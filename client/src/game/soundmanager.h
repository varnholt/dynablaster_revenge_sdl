#pragma once

// SDL3-backed sound manager - replaces the original's hand-rolled per-platform mixer.
// Music decoding uses minimp3 (see CMakeLists.txt).

// shared
#include "constants.h"
#include "signal.h"
#include "timer.h"

// SDL
#include <SDL3/SDL_audio.h>

#include <array>
#include <filesystem>
#include <string>
#include <vector>

class SoundManager
{
public:
   static SoundManager* getInstance();

   void fadeOut(float fade_out_time);
   void restartPlayListAfterFadeOut(int delay);

   void startPlaylist();
   void playNextTrack();

   float getVolumeMusic() const;
   float getVolumeSfx() const;
   void setVolumeMusic(float volume);
   void setVolumeSfx(float volume);

   void playSoundKilled();
   void playSoundBomb();
   void playSoundStart();
   void playSoundExtra();
   void playSoundTime(int time);
   void playSoundPlayerJoined();
   void playSoundPlayerLeft();
   void playSoundKick();
   void playSoundHurryUp();
   void playSoundMessageSent();
   void playSoundMessageReceived();
   void playSoundBombBounce();
   void playSoundBoxShake();
   void playSoundExtraRevealed();
   void playSkullSound(Constants::SkullType skull_type);
   void playSoundGameWin();
   void playSoundGameDraw();

   void playSoundMouseOver(const std::string& page, const std::string& item);
   void playSoundMouseClick(const std::string& page);
   void playSoundTick();

   // ticks on _music_timer; auto-advances finished tracks and drives the fade-out ramp.
   void updateMusic();

   // matches the original's SoundManager::musicPlaying(artist, album, track) signal -
   // MusicPlayerDrawable connects to this to show its slide-in "now playing" notification.
   Signal<const std::string&, const std::string&, const std::string&> trackChangedSignal;

protected:
   SoundManager();
   ~SoundManager();

   enum SampleId
   {
      SampleBomb,
      SampleExtra,
      SampleKilled,
      SampleStart,
      SampleCountdown1,
      SampleCountdown2,
      SampleCountdown3,
      SamplePlayerJoined,
      SamplePlayerLeft,
      SampleKick,
      SampleHurryUp,
      SampleMessageSent,
      SampleMessageReceived,
      SampleBombBounce,
      SampleBoxShake,
      SampleExtraRevealed,
      SampleExtraMushroom,
      SampleExtraInvisible,
      SampleExtraInvulnerable,
      SampleMouseOver,
      SampleMouseClick,
      SampleGameWin,
      SampleGameDraw,
      SampleCount
   };

   struct Sample
   {
      Uint8* buffer = nullptr;
      Uint32 length = 0;
      SDL_AudioSpec spec{};
   };

   void initializeSamples();
   void loadSample(SampleId id, const char* filename);

   // picks the next of a small round-robin pool of mixed-together channels (matches the
   // original's own fixed-channel-count SamplePlayer, referenced by its getChannelCount()) and
   // feeds it this sample's PCM data - cutting off whatever that channel was still playing, same
   // trade-off any simple fixed-channel sfx mixer makes.
   void play(SampleId id);

   // decodes the whole track up front via minimp3 and queues it in one go.
   void playTrack(std::size_t index);

   static constexpr int channel_count = 8;

   SDL_AudioDeviceID _device = 0;
   std::array<SDL_AudioStream*, channel_count> _channels{};
   int _next_channel = 0;
   std::array<Sample, SampleCount> _samples{};

   // skips the click sound on the very first pageChanged (initial page load, not a real click).
   bool _mouse_click_initialized = false;

   float _volume_music = 1.0f;
   float _volume_sfx = 1.0f;

   SDL_AudioStream* _music_stream = nullptr;
   std::vector<std::filesystem::path> _playlist;
   std::size_t _track_index = 0;
   Timer _music_timer;

   bool _fading = false;
   float _fade_start_volume = 1.0f;
   float _fade_duration_ms = 1000.0f;
   float _fade_elapsed_ms = 0.0f;

   static SoundManager* sInstance;
};
