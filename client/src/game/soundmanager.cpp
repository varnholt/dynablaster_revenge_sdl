#include "soundmanager.h"

#include "gamesettings.h"
#include "tools/singleton.h"

#include "logging.h"

#include <SDL3/SDL.h>

#define MINIMP3_IMPLEMENTATION
#include "minimp3_ex.h"

#include "stringutils.h"

#include <algorithm>

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
}  // namespace

SoundManager::SoundManager()
{
   if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
   {
      qWarning("SoundManager: SDL_InitSubSystem(SDL_INIT_AUDIO) failed: %s", SDL_GetError());
      return;
   }

   _device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
   if (_device == 0)
   {
      qWarning("SoundManager: SDL_OpenAudioDevice failed: %s", SDL_GetError());
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
      qWarning("SoundManager: failed to load %s: %s", filename.c_str(), SDL_GetError());
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

   Timer::singleShot(
      delay,
      [this]()
      {
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
      qWarning("SoundManager: no music found in data/music");
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

void SoundManager::playTrack(std::size_t index)
{
   if (!_music_stream || _playlist.empty())
      return;

   const std::string path = _playlist[index % _playlist.size()].string();

   mp3dec_t decoder;
   mp3dec_file_info_t info{};
   if (mp3dec_load(&decoder, path.c_str(), &info, nullptr, nullptr) != 0 || !info.buffer)
   {
      qWarning("SoundManager: failed to decode %s", path.c_str());
      return;
   }

   SDL_AudioSpec spec{};
   spec.format = SDL_AUDIO_S16;
   spec.channels = info.channels;
   spec.freq = info.hz;

   SDL_ClearAudioStream(_music_stream.get());
   SDL_SetAudioStreamFormat(_music_stream.get(), &spec, nullptr);
   SDL_PutAudioStreamData(_music_stream.get(), info.buffer, static_cast<int>(info.samples * sizeof(mp3d_sample_t)));

   free(info.buffer);

   std::string artist;
   std::string track;
   splitTrackFilename(_playlist[index % _playlist.size()].stem().string(), artist, track);
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
      }

      return;
   }

   if (!_playlist.empty() && SDL_GetAudioStreamQueued(_music_stream.get()) == 0)
   {
      _track_index = (_track_index + 1) % _playlist.size();
      playTrack(_track_index);
   }
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
