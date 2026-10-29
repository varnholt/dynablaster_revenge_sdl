# The colour schemes of the story's worlds: five colours each that go together (the way kuler.adobe.com sets them),
# a world's every colour taken from them or a shade of one of them, so its ground, props and blocks belong together.
# Mostly neighbouring hues for the world's body, one complementary accent that sets it off.
#   python palettes.py <sheet.png>      draws them side by side to judge them
import sys

PALETTES = {
    1: ("meadow, a sunny morning", ["#4E6B3A", "#8FA65A", "#D9CFB4", "#A7C0D4", "#C7774F"],
        ["moss in the shade", "sunlit grass", "limestone", "morning haze", "clay, flowers"]),
    2: ("rocky mountains, dusty and warm", ["#8C4A2F", "#C98B57", "#E6CFA6", "#6F7A8C", "#A8183A"],
        ["rust rock", "sandstone", "dust", "slate cobbles", "ruby"]),
    3: ("river in the mist", ["#2F5D73", "#5E8FA3", "#A9C4C9", "#6E7F57", "#D8CDB0"],
        ["deep water", "river", "mist, granite", "reeds, moss", "sand bank"]),
    4: ("forest gloom", ["#1F3A2B", "#3F6B3A", "#8AA85A", "#5A4030", "#C24A3A"],
        ["deep pine", "leaves", "sunfleck", "bark", "toadstool"]),
    5: ("lava cave", ["#1C1A1C", "#4A3A36", "#8C3B22", "#E0662A", "#F5C15A"],
        ["basalt", "ash", "clay", "lava", "ember"]),
    6: ("the keep by torchlight", ["#3B2A20", "#8C6B4A", "#CDB38C", "#8E1F1F", "#D9A441"],
        ["oak", "sandstone in shade", "sandstone", "banner red", "gold"]),
    7: ("the machine hall", ["#2A2C30", "#5C6066", "#9A9FA6", "#D9A21B", "#C4562A"],
        ["iron", "steel", "deck plate", "hazard yellow", "furnace"]),
    8: ("the final hall", ["#0E1A22", "#1F3B47", "#2E8C96", "#9BA6B4", "#E0503C"],
        ["the dark", "dark teal", "teal light", "panel grey", "alarm red"]),
}


def color(world, index, shade=1.0):
    """Colour index of the world's palette as rgb 0..1 (sRGB, like every colour the builders take), shade times as
    light (above 1 towards white)."""
    hex_value = PALETTES[world][1][index].lstrip("#")
    rgb = [int(hex_value[i:i + 2], 16) / 255 for i in (0, 2, 4)]
    if shade <= 1.0:
        return tuple(c * shade for c in rgb)
    return tuple(c + (1 - c) * (shade - 1) for c in rgb)


def _oklab(rgb):
    """sRGB 0..1 (any shape ending in 3) to OKLab."""
    import numpy as np
    c = np.clip(np.asarray(rgb, np.float64), 0, 1)
    c = np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)
    lms = c @ np.array([[0.4122214708, 0.2119034982, 0.0883024619], [0.5363325363, 0.6806995451, 0.2817188376],
                        [0.0514459929, 0.1073969566, 0.6299787005]])
    lms = np.cbrt(lms)
    return lms @ np.array([[0.2104542553, 1.9779984951, 0.0259040371], [0.7936177850, -2.4285922050, 0.7827717662],
                           [-0.0040720468, 0.4505937099, -0.8086757660]])


def _srgb(lab):
    import numpy as np
    lms = np.asarray(lab) @ np.array([[1.0, 1.0, 1.0], [0.3963377774, -0.1055613458, -0.0894841775],
                                      [0.2158037573, -0.0638541728, -1.2914855480]])
    lms = lms ** 3
    c = lms @ np.array([[4.0767416621, -1.2684380046, -0.0041960863], [-3.3077115913, 2.6097574011, -0.7034186147],
                        [0.2309699292, -0.3413193478, 1.7076127010]])
    c = np.clip(c, 0, 1)
    return np.where(c <= 0.0031308, c * 12.92, 1.055 * c ** (1 / 2.4) - 0.055)


def snap(world, rgb):
    """rgb (sRGB 0..1, a colour or an image) as shades of the world's palette: each colour takes the hue and the
    colourfulness of the palette colour nearest to it in those (OKLab), its own lightness stays. So shading, wear and
    variation painted into a texture survive, while every hue in the world is one of its five."""
    import numpy as np
    scalar = np.ndim(rgb) == 1
    lab = _oklab(np.atleast_2d(np.asarray(rgb, np.float64)) if scalar else rgb)
    palette = _oklab([color(world, i) for i in range(5)])
    # nearest by hue and colourfulness, a touch by lightness to settle greys; more and a brown went with a dark green
    chroma = lab[..., None, 1:] - palette[:, 1:]
    distance = np.sqrt((chroma ** 2).sum(-1)) + 0.04 * np.abs(lab[..., None, 0] - palette[:, 0])
    nearest = palette[np.argmin(distance, axis=-1)]
    out = _srgb(np.concatenate([lab[..., :1], nearest[..., 1:]], axis=-1))
    return tuple(float(c) for c in out[0]) if scalar else out


def sheet(path):
    from PIL import Image, ImageDraw, ImageFont
    swatch, label = 150, 400
    image = Image.new("RGB", (label + 5 * swatch, len(PALETTES) * swatch), (24, 24, 26))
    draw = ImageDraw.Draw(image)
    try:
        title, small = ImageFont.truetype("arialbd.ttf", 22), ImageFont.truetype("arial.ttf", 15)
    except OSError:
        title = small = ImageFont.load_default()
    for row, (world, (name, colors, roles)) in enumerate(sorted(PALETTES.items())):
        y = row * swatch
        draw.text((16, y + 50), f"{world}  {name}", fill=(230, 230, 230), font=title)
        for i, (hex_value, role) in enumerate(zip(colors, roles)):
            x = label + i * swatch
            draw.rectangle((x, y, x + swatch - 1, y + swatch - 1), fill=hex_value)
            text = (20, 20, 20) if sum(int(hex_value[k:k + 2], 16) for k in (1, 3, 5)) > 380 else (235, 235, 235)
            draw.text((x + 8, y + swatch - 44), role, fill=text, font=small)
            draw.text((x + 8, y + swatch - 24), hex_value, fill=text, font=small)
    image.save(path)


if __name__ == "__main__":
    sheet(sys.argv[1] if len(sys.argv) > 1 else "palettes.png")
