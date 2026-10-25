#include "soundmanager.h"

#include "gamesettings.h"
#include "tools/singleton.h"

#include "log.h"

#include <SDL3/SDL.h>

#define MINIMP3_IMPLEMENTATION
#include "minimp3_ex.h"

#include "stringutils.h"

#include <algorithm>
#include <chrono>
#include <format>

namespace
{
// this port's mp3 playlist names each track "NN-artist-track_words.mp3" (no embedded ID3 tags,
// unlike the original's OGG Vorbis comments) - split that on the fly instead of adding a whole
// tag-parsing dependency for metadata the filename already carries.
void splitTrackFilename(const std::string& stem, std::string& artist, std::string& track)
{
   const std::vector<std::string> parts = StringUtils::split(stem, '-');
   artist = parts.size() > 1 ? parts[1] : stem;
   track.clear();
   for (std::size_t i = 2; i < parts.size(); i++)
   {
      if (i > 2)
         track += " ";
      track += parts[i];
   }
   if (track.empty())
      track = artist;

   // the game's menu font only has uppercase glyphs (matches e.g. the login nick field showing
   // a lowercase-stored "player1" as "PLAYER1") - no case-folding needed here, just readability.
   std::replace(track.begin(), track.end(), '_', ' ');
}

// the following track is decoded this long before the playing one runs out
constexpr int prefetch_seconds = 5;
}  // namespace

SoundManager::SoundManager()
{
   if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
   {
      Log::warning(std::format("SoundManager: SDL_InitSubSystem(SDL_INIT_AUDIO) failed: {}", SDL_GetError()));
      return;
   }

   _device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
   if (_device == 0)
   {
      Log::warning(std::format("SoundManager: SDL_OpenAudioDevice failed: {}", SDL_GetError()));
      return;
   }

   std::array<SDL_AudioStream*, channel_count> channels{};
   for (size_t i = 0; i < channels.size(); i++)
   {
      _channels[i].reset(SDL_CreateAudioStream(nullptr, nullptr));
      channels[i] = _channels[i].get();
   }

   SDL_BindAudioStreams(_device, channels.data(), static_cast<int>(channels.size()));

   _music_stream.reset(SDL_CreateAudioStream(nullptr, nullptr));
   SDL_BindAudioStream(_device, _music_stream.get());

   SDL_ResumeAudioDevice(_device);

   initializeSamples();

   _volume_music = GameSettings::getInstance().getAudioSettings().getVolumeMusic();
   _volume_sfx = GameSettings::getInstance().getAudioSettings().getVolumeSfx();
   SDL_SetAudioStreamGain(_music_stream.get(), getMusicGain());

   _music_timer.timeoutSignal.connect([this]() { updateMusic(); });
   _music_timer.start(50);
}

SoundManager::~SoundManager()
{
   if (_decode_cancel)
      _decode_cancel->store(true);

   if (_decode_future.valid())
      _decode_future.wait();

   // the streams go before the device
   for (auto& channel : _channels)
   {
      channel.reset();
   }

   _music_stream.reset();

   for (auto& sample : _samples)
   {
      sample.buffer.reset();
   }

   if (_device)
      SDL_CloseAudioDevice(_device);
}

SoundManager& SoundManager::getInstance()
{
   return Singleton<SoundManager>::Instance();
}

void SoundManager::initializeSamples()
{
   loadSample(SampleBomb, "data/sfx/bomb.wav");
   loadSample(SampleExtra, "data/sfx/extra.wav");
   loadSample(SampleKilled, "data/sfx/killed.wav");
   loadSample(SampleStart, "data/sfx/start.wav");
   loadSample(SampleCountdown1, "data/sfx/countdown_1.wav");
   loadSample(SampleCountdown2, "data/sfx/countdown_2.wav");
   loadSample(SampleCountdown3, "data/sfx/countdown_3.wav");
   loadSample(SamplePlayerJoined, "data/sfx/player_joined.wav");
   loadSample(SamplePlayerLeft, "data/sfx/player_left.wav");
   loadSample(SampleKick, "data/sfx/kick.wav");
   loadSample(SampleHurryUp, "data/sfx/hurry_up.wav");
   loadSample(SampleMessageSent, "data/sfx/message_sent.wav");
   loadSample(SampleMessageReceived, "data/sfx/message_received.wav");
   loadSample(SampleBombBounce, "data/sfx/bounce.wav");
   loadSample(SampleBoxShake, "data/sfx/shake.wav");
   loadSample(SampleExtraRevealed, "data/sfx/extra_revealed.wav");
   loadSample(SampleExtraMushroom, "data/sfx/extra_mushroom.wav");
   loadSample(SampleExtraInvisible, "data/sfx/extra_invisible.wav");
   loadSample(SampleExtraInvulnerable, "data/sfx/extra_invulnerable.wav");
   loadSample(SampleMouseOver, "data/sfx/mouse_over.wav");
   loadSample(SampleMouseClick, "data/sfx/mouse_click.wav");
   loadSample(SampleGameWin, "data/sfx/win.wav");
   loadSample(SampleGameDraw, "data/sfx/draw.wav");
   loadSample(SampleError, "data/sfx/error.wav");
   loadSample(SampleInfo, "data/sfx/info.wav");
}

void SoundManager::StreamDeleter::operator()(SDL_AudioStream* stream) const
{
   SDL_UnbindAudioStream(stream);
   SDL_DestroyAudioStream(stream);
}

void SoundManager::loadSample(SampleId id, const std::string& filename)
{
   Sample& sample = _samples[id];
   Uint8* buffer = nullptr;
   Uint32 length = 0;

   if (!SDL_LoadWAV(filename.c_str(), &sample.spec, &buffer, &length))
   {
      Log::warning(std::format("SoundManager: failed to load {}: {}", filename, SDL_GetError()));
      return;
   }

   sample.buffer.reset(buffer);
   sample.length = length;
}

void SoundManager::play(SampleId id)
{
   if (_device == 0)
      return;

   const Sample& sample = _samples[id];
   if (!sample.buffer)
      return;

   const Stream& channel = _channels[_next_channel];
   _next_channel = (_next_channel + 1) % channel_count;

   SDL_ClearAudioStream(channel.get());
   SDL_SetAudioStreamGain(channel.get(), getSfxGain());
   SDL_SetAudioStreamFormat(channel.get(), &sample.spec, nullptr);
   SDL_PutAudioStreamData(channel.get(), sample.buffer.get(), static_cast<int>(sample.length));
}

void SoundManager::fadeOut(float fade_out_time)
{
   if (_fading || !_music_stream)
      return;

   _fading = true;
   _fade_start_volume = SDL_GetAudioStreamGain(_music_stream.get());
   _fade_duration_ms = fade_out_time;
   _fade_elapsed_ms = 0.0f;
}

void SoundManager::restartPlayListAfterFadeOut(int delay)
{
   if (_music_stream)
      SDL_ClearAudioStream(_music_stream.get());

   _music_halted = true;
   cancelPendingTrack();

   Timer::singleShot(
      delay,
      [this]()
      {
         _fading = false;
         SDL_SetAudioStreamGain(_music_stream.get(), getMusicGain());
         playNextTrack();
      }
   );
}

float SoundManager::getVolumeMusic() const
{
   return _volume_music;
}

float SoundManager::getVolumeSfx() const
{
   return _volume_sfx;
}

void SoundManager::setVolumeMusic(float volume)
{
   _volume_music = volume;

   if (_music_stream && !_fading)
      SDL_SetAudioStreamGain(_music_stream.get(), getMusicGain());
}

void SoundManager::setVolumeSfx(float volume)
{
   _volume_sfx = volume;
}

bool SoundManager::isMusicMuted() const
{
   return _music_muted;
}

bool SoundManager::isSfxMuted() const
{
   return _sfx_muted;
}

void SoundManager::toggleMuteMusic()
{
   _music_muted = !_music_muted;

   if (_music_stream && !_fading)
      SDL_SetAudioStreamGain(_music_stream.get(), getMusicGain());
}

void SoundManager::toggleMuteSfx()
{
   _sfx_muted = !_sfx_muted;
}

float SoundManager::getMusicGain() const
{
   return _music_muted ? 0.0f : _volume_music;
}

float SoundManager::getSfxGain() const
{
   return _sfx_muted ? 0.0f : _volume_sfx;
}

void SoundManager::startPlaylist()
{
   if (_playlist.empty())
   {
      for (const auto& entry : std::filesystem::directory_iterator("data/music"))
      {
         if (entry.path().extension() == ".mp3")
            _playlist.push_back(entry.path());
      }

      std::sort(_playlist.begin(), _playlist.end());
   }

   if (_playlist.empty())
   {
      Log::warning("SoundManager: no music found in data/music");
      return;
   }

   _track_index = 0;
   playTrack(_track_index);
}

void SoundManager::playNextTrack()
{
   if (_playlist.empty())
   {
      startPlaylist();
      return;
   }

   _track_index = (_track_index + 1) % _playlist.size();
   playTrack(_track_index);
}

void SoundManager::playPreviousTrack()
{
   if (_playlist.empty())
   {
      startPlaylist();
      return;
   }

   _track_index = (_track_index + _playlist.size() - 1) % _playlist.size();
   playTrack(_track_index);
}

void SoundManager::playTrack(std::size_t index)
{
   if (!_music_stream || _playlist.empty())
      return;

   _pending_track = index % _playlist.size();
   updateDecode();
}

void SoundManager::cancelPendingTrack()
{
   // a decode still running for it carries on and is kept for when the track is asked for again
   _pending_track.reset();
}

std::unique_ptr<SoundManager::DecodedTrack> SoundManager::decodeTrack(
   std::size_t index,
   const std::filesystem::path& path,
   std::shared_ptr<std::atomic_bool> cancel
)
{
   const auto progress = [](void* user_data, size_t, uint64_t, mp3dec_frame_info_t*) -> int
   { return static_cast<std::atomic_bool*>(user_data)->load() ? MP3D_E_USER : 0; };

   mp3dec_t decoder;
   mp3dec_file_info_t info{};
   const int result = mp3dec_load(&decoder, path.string().c_str(), &info, progress, cancel.get());
   std::unique_ptr<int16_t, FreeDeleter> buffer(info.buffer);

   if (cancel->load())
      return nullptr;

   auto decoded = std::make_unique<DecodedTrack>();
   decoded->_index = index;

   if (result == 0 && buffer)
   {
      decoded->_buffer = std::move(buffer);
      decoded->_sample_count = info.samples;
      decoded->_spec.format = SDL_AUDIO_S16;
      decoded->_spec.channels = info.channels;
      decoded->_spec.freq = info.hz;
   }

   return decoded;
}

void SoundManager::requestDecode(std::size_t index)
{
   _decode_index = index;
   _decode_cancel = std::make_shared<std::atomic_bool>(false);

#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
   std::promise<std::unique_ptr<DecodedTrack>> promise;
   promise.set_value(decodeTrack(index, _playlist[index], _decode_cancel));
   _decode_future = promise.get_future();
#else
   _decode_future = std::async(std::launch::async, &SoundManager::decodeTrack, index, _playlist[index], _decode_cancel);
#endif
}

void SoundManager::updateDecode()
{
   // a second pass picks up a decode that finished right away (browser build without threads)
   for (auto pass = 0; pass < 2; pass++)
   {
      if (_decode_future.valid() && _decode_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
      {
         if (auto decoded = _decode_future.get())
            _decoded = std::move(decoded);

         _decode_cancel.reset();
      }

      if (_pending_track)
      {
         const auto index = *_pending_track;

         if (_decoded && _decoded->_index == index)
         {
            _pending_track.reset();
            const auto decoded = std::move(_decoded);

            if (decoded->_buffer)
               startTrack(*decoded);
            else
               Log::warning(std::format("SoundManager: failed to decode {}", _playlist[index].string()));
         }
         else if (!_decode_future.valid())
         {
            requestDecode(index);
            continue;
         }
         else if (_decode_index != index)
         {
            // superseded, the requested track is decoded once this one has given up
            _decode_cancel->store(true);
         }

         return;
      }

      break;
   }

   if (_music_halted || _decode_future.valid() || _playlist.empty() || _music_spec.freq == 0)
      return;

   const auto next = (_track_index + 1) % _playlist.size();
   if (_decoded && _decoded->_index == next)
      return;

   const auto bytes_per_second = _music_spec.freq * _music_spec.channels * static_cast<int>(sizeof(int16_t));
   if (SDL_GetAudioStreamQueued(_music_stream.get()) < bytes_per_second * prefetch_seconds)
      requestDecode(next);
}

void SoundManager::startTrack(DecodedTrack& decoded)
{
   _music_halted = false;
   _music_spec = decoded._spec;

   SDL_ClearAudioStream(_music_stream.get());
   SDL_SetAudioStreamFormat(_music_stream.get(), &decoded._spec, nullptr);
   SDL_PutAudioStreamData(
      _music_stream.get(), decoded._buffer.get(), static_cast<int>(decoded._sample_count * sizeof(int16_t))
   );

   // the whole track is queued, flushing hands the resampler's held-back tail to the device as well -
   // without it a few bytes stay queued forever and updateMusic() never notices the track ended
   SDL_FlushAudioStream(_music_stream.get());

   std::string artist;
   std::string track;
   splitTrackFilename(_playlist[decoded._index].stem().string(), artist, track);
   trackChangedSignal(artist, "Dynablaster Revenge", track);
}

void SoundManager::updateMusic()
{
   if (!_music_stream)
      return;

   if (_fading)
   {
      _fade_elapsed_ms += 50.0f;
      const float factor = (std::max)(1.0f - _fade_elapsed_ms / _fade_duration_ms, 0.0f);
      SDL_SetAudioStreamGain(_music_stream.get(), factor * _fade_start_volume);

      if (factor <= 0.0f)
      {
         SDL_ClearAudioStream(_music_stream.get());
         _fading = false;
         _music_halted = true;
         cancelPendingTrack();
      }
   }
   else if (!_music_halted && !_pending_track && !_playlist.empty() && SDL_GetAudioStreamQueued(_music_stream.get()) == 0)
   {
      _track_index = (_track_index + 1) % _playlist.size();
      playTrack(_track_index);
      return;
   }

   updateDecode();
}

void SoundManager::playSoundKilled()
{
   play(SampleKilled);
}

void SoundManager::playSoundBomb()
{
   play(SampleBomb);
}

void SoundManager::playSoundGameWin()
{
   play(SampleGameWin);
}

void SoundManager::playSoundGameDraw()
{
   play(SampleGameDraw);
}

void SoundManager::playSoundError()
{
   play(SampleError);
}

void SoundManager::playSoundInfo()
{
   play(SampleInfo);
}

void SoundManager::playSoundStart()
{
   play(SampleStart);
}

void SoundManager::playSoundExtra()
{
   play(SampleExtra);
}

void SoundManager::playSoundTime(int time)
{
   if (time == 1)
      play(SampleCountdown1);
   else if (time == 2)
      play(SampleCountdown2);
   else if (time == 3)
      play(SampleCountdown3);
}

void SoundManager::playSoundPlayerJoined()
{
   play(SamplePlayerJoined);
}

void SoundManager::playSoundPlayerLeft()
{
   play(SamplePlayerLeft);
}

void SoundManager::playSoundKick()
{
   play(SampleKick);
}

void SoundManager::playSoundHurryUp()
{
   play(SampleHurryUp);
}

void SoundManager::playSoundMessageSent()
{
   play(SampleMessageSent);
}

void SoundManager::playSoundMessageReceived()
{
   play(SampleMessageReceived);
}

void SoundManager::playSoundBombBounce()
{
   play(SampleBombBounce);
}

void SoundManager::playSoundBoxShake()
{
   play(SampleBoxShake);
}

void SoundManager::playSoundExtraRevealed()
{
   play(SampleExtraRevealed);
}

void SoundManager::playSkullSound(Constants::SkullType skull_type)
{
   switch (skull_type)
   {
      case Constants::SkullMushroom:
         play(SampleExtraMushroom);
         break;

      case Constants::SkullInvisible:
         play(SampleExtraInvisible);
         break;

      case Constants::SkullInvincible:
         play(SampleExtraInvulnerable);
         break;

      default:
         break;
   }
}

void SoundManager::playSoundMouseOver(const std::string& /*page*/, const std::string& item)
{
   if (item.starts_with("button"))
      play(SampleMouseOver);
}

void SoundManager::playSoundMouseClick(const std::string& /*page*/)
{
   if (_mouse_click_initialized)
      play(SampleMouseClick);
   else
      _mouse_click_initialized = true;
}

void SoundManager::playSoundTick()
{
   play(SampleMouseOver);
}
