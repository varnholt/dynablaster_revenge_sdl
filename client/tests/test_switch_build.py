"""Validate an already-built Switch NRO inside the devkitPro container."""
import hashlib
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest

CLIENT = Path(__file__).resolve().parents[1]
BUILD = CLIENT / 'build-switch' / 'gcc14'
NRO = BUILD / 'dynablaster_revenge.nro'
ELF = BUILD / 'dynablaster_revenge.elf'


class SwitchBuild(unittest.TestCase):
    def test_numeric_network_addresses(self):
        # Run the numeric formatter on the container host to check IPv4/IPv6,
        # network byte order and bounds without requiring console services.
        source = r'''
        #include "switchnetcompat.h"
        #include <assert.h>
        #include <string.h>
        int main(void) {
            char host[80], service[16];
            const int flags = NI_NUMERICHOST | NI_NUMERICSERV;
            struct sockaddr_in v4 = {0};
            v4.sin_family = AF_INET;
            v4.sin_port = htons(2137);
            assert(inet_pton(AF_INET, "127.0.0.1", &v4.sin_addr) == 1);
            assert(getnameinfo((struct sockaddr*)&v4, sizeof(v4), host, sizeof(host), service, sizeof(service), flags) == 0);
            assert(strcmp(host, "127.0.0.1") == 0 && strcmp(service, "2137") == 0);
            assert(getnameinfo((struct sockaddr*)&v4, sizeof(v4), host, 3, 0, 0, flags) == EAI_OVERFLOW);
            assert(getnameinfo((struct sockaddr*)&v4, sizeof(v4), 0, 0, service, 2, flags) == EAI_OVERFLOW);
            assert(getnameinfo((struct sockaddr*)&v4, 1, host, sizeof(host), 0, 0, flags) == EAI_FAMILY);
            struct sockaddr_in6 v6 = {0};
            v6.sin6_family = AF_INET6;
            v6.sin6_port = htons(65535);
            v6.sin6_scope_id = 4;
            assert(inet_pton(AF_INET6, "ff02::1", &v6.sin6_addr) == 1);
            assert(getnameinfo((struct sockaddr*)&v6, sizeof(v6), host, sizeof(host), service, sizeof(service), flags) == 0);
            assert(strcmp(host, "ff02::1%4") == 0 && strcmp(service, "65535") == 0);
            assert(getnameinfo((struct sockaddr*)&v6, sizeof(v6), 0, 0, service, sizeof(service), NI_NUMERICSERV) == 0);
            assert(strcmp(service, "65535") == 0);
            return 0;
        }
        '''
        with tempfile.TemporaryDirectory() as temporary:
            code = Path(temporary) / 'network.c'
            program = Path(temporary) / 'network'
            code.write_text(source)
            subprocess.run(['cc', '-std=c11', '-D_DEFAULT_SOURCE',
                            '-I', str(CLIENT / 'src/platform'), str(code), '-o', str(program)], check=True)
            subprocess.run([str(program)], check=True)

    def test_nro_and_embedded_romfs(self):
        with NRO.open('rb') as stream:
            header = stream.read(0x80)
            self.assertEqual(header[0x10:0x14], b'NRO0')
            declared_size = struct.unpack_from('<I', header, 0x18)[0]
            self.assertGreater(declared_size, 0)
            stream.seek(declared_size)
            assets = stream.read(56)
            self.assertEqual(assets[:4], b'ASET')
            nacp_offset, nacp_size = struct.unpack_from('<QQ', assets, 24)
            romfs_offset, romfs_size = struct.unpack_from('<QQ', assets, 40)
            self.assertEqual(nacp_size, 0x4000)
            self.assertGreater(romfs_size, 0)
            self.assertLessEqual(declared_size + romfs_offset + romfs_size, NRO.stat().st_size)
            stream.seek(declared_size + nacp_offset)
            self.assertTrue(stream.read(512).startswith(b'Dynablaster Revenge\0'))

            romfs_start = declared_size + romfs_offset
            stream.seek(romfs_start)
            romfs = struct.unpack('<10Q', stream.read(80))
            self.assertEqual(romfs[0], 80)
            stream.seek(romfs_start + romfs[3])
            directories = stream.read(romfs[4])
            stream.seek(romfs_start + romfs[7])
            files = stream.read(romfs[8])
            embedded = {}

            def walk(directory_offset, parent):
                _, _, child_directory, child_file, _, name_length = struct.unpack_from('<6I', directories, directory_offset)
                name = directories[directory_offset + 24:directory_offset + 24 + name_length].decode()
                path = parent / name
                while child_file != 0xffffffff:
                    _, sibling, offset, size, _, length = struct.unpack_from('<IIQQII', files, child_file)
                    filename = files[child_file + 32:child_file + 32 + length].decode()
                    embedded[path / filename] = (offset, size)
                    child_file = sibling
                while child_directory != 0xffffffff:
                    walk(child_directory, path)
                    child_directory = struct.unpack_from('<I', directories, child_directory + 4)[0]

            walk(0, Path())
            sources = {Path('data') / file.relative_to(CLIENT / 'data'): file
                       for file in (CLIENT / 'data').rglob('*') if file.is_file()}
            self.assertEqual(set(embedded), set(sources), 'RomFS file set differs from source assets')
            for path, source in sources.items():
                offset, size = embedded[path]
                self.assertEqual(size, source.stat().st_size, str(path))
                stream.seek(romfs_start + romfs[9] + offset)
                self.assertEqual(hashlib.sha256(stream.read(size)).digest(),
                                 hashlib.sha256(source.read_bytes()).digest(), str(path))

    def test_aarch64_elf(self):
        with ELF.open('rb') as stream:
            header = stream.read(20)
        self.assertEqual(header[:6], b'\x7fELF\x02\x01')
        self.assertEqual(struct.unpack_from('<HH', header, 16), (3, 0xb7))

    def test_real_platform_backends_linked(self):
        result = subprocess.run(['/opt/devkitpro/devkitA64/bin/aarch64-none-elf-nm',
                                 '--defined-only', str(ELF)], check=True, capture_output=True, text=True)
        symbols = {line.split()[-1] for line in result.stdout.splitlines() if line.split()}
        for symbol in ['SWITCH_VideoInit', 'SWITCH_CreateWindow', 'SWITCH_GLES_CreateContext',
                       'SWITCH_PumpEvents', 'SWITCHAUD_OpenDevice', 'padUpdate',
                       'socketInitialize', 'romfsMountSelf', 'swkbdShow']:
            self.assertTrue(symbol in symbols, f'Missing linked platform symbol: {symbol}')


if __name__ == '__main__':
    unittest.main(verbosity=2)
