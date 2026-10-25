#pragma once

// SDL3-backed sound manager - replaces the original's hand-rolled per-platform mixer.
// Music decoding uses minimp3 (see CMakeLists.txt).

// shared
#include "constants.h"
#include "gamesignal.h"
#include "timer.h"

// SDL
#include <SDL3/SDL_audio.h>

#include <array>
#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class SoundManager
{
public:
   //! the instance is never destroyed
   static SoundManager& getInstance();

   SoundManager();
   ~SoundManager();

   void fadeOut(float fade_out_time);
   void restartPlayListAfterFadeOut(int delay);

   void startPlaylist();
   void playNextTrack();
   void playPreviousTrack();

   float getVolumeMusic() const;
   float getVolumeSfx() const;
   void setVolumeMusic(float volume);
   void setVolumeSfx(float volume);

   // F2/F3, shown by the HUD's mute icons; the configured volumes stay as they are
   bool isMusicMuted() const;
   bool isSfxMuted() const;
   void toggleMuteMusic();
   void toggleMuteSfx();

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
   void playSoundError();
   void playSoundInfo();

   void playSoundMouseOver(const std::string& page, const std::string& item);
   void playSoundMouseClick(const std::string& page);
   void playSoundTick();

   // ticks on _music_timer; auto-advances finished tracks and drives the fade-out ramp.
   void updateMusic();

   // matches the original's SoundManager::musicPlaying(artist, album, track) signal -
   // MusicPlayerDrawable connects to this to show its slide-in "now playing" notification.
   Signal<const std::string&, const std::string&, const std::string&> trackChangedSignal;

protected:
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
      SampleError,
      SampleInfo,
      SampleCount
   };

   struct StreamDeleter
   {
      void operator()(SDL_AudioStream* stream) const;
   };

   using Stream = std::unique_ptr<SDL_AudioStream, StreamDeleter>;

   struct FreeDeleter
   {
      void operator()(void* buffer) const
      {
         std::free(buffer);
      }
   };

   // one track's PCM data as minimp3 decoded it, empty if the decode failed or was cancelled
   struct DecodedTrack
   {
      std::size_t _index = 0;
      std::unique_ptr<int16_t, FreeDeleter> _buffer;
      std::size_t _sample_count = 0;
      SDL_AudioSpec _spec{};
   };

   // decodes the playlist's track index on a worker thread (synchronously on a browser build
   // without threads); the worker gives up early once cancel is set
   void requestDecode(std::size_t index);

   // runs on the worker, touches nothing but its arguments
   static std::unique_ptr<DecodedTrack> decodeTrack(
      std::size_t index,
      const std::filesystem::path& path,
      std::shared_ptr<std::atomic_bool> cancel
   );

   // collects a finished decode, starts the requested track once its PCM data is there and
   // prefetches the following track when the playing one nears its end
   void updateDecode();

   // queues a decoded track on the music stream and announces it; main thread only
   void startTrack(DecodedTrack& decoded);

   void cancelPendingTrack();

   struct Sample
   {
      std::unique_ptr<Uint8, decltype(&SDL_free)> buffer{nullptr, &SDL_free};
      Uint32 length = 0;
      SDL_AudioSpec spec{};
   };

   void initializeSamples();
   void loadSample(SampleId id, const std::string& filename);

   // picks the next of a small round-robin pool of mixed-together channels (matches the
   // original's own fixed-channel-count SamplePlayer, referenced by its getChannelCount()) and
   // feeds it this sample's PCM data - cutting off whatever that channel was still playing, same
   // trade-off any simple fixed-channel sfx mixer makes.
   void play(SampleId id);

   // asks for the track to be played; it starts (and trackChangedSignal fires) once it has been
   // decoded off the main thread, the current track keeps playing until then. A later request
   // supersedes an earlier one that hasn't started yet.
   void playTrack(std::size_t index);

   float getMusicGain() const;
   float getSfxGain() const;

   static constexpr int channel_count = 8;

   SDL_AudioDeviceID _device = 0;
   std::array<Stream, channel_count> _channels;
   int _next_channel = 0;
   std::array<Sample, SampleCount> _samples{};

   // skips the click sound on the very first pageChanged (initial page load, not a real click).
   bool _mouse_click_initialized = false;

   float _volume_music = 1.0f;
   float _volume_sfx = 1.0f;
   bool _music_muted = false;
   bool _sfx_muted = false;

   Stream _music_stream;
   std::vector<std::filesystem::path> _playlist;
   std::size_t _track_index = 0;
   Timer _music_timer;

   bool _fading = false;
   float _fade_start_volume = 1.0f;
   float _fade_duration_ms = 1000.0f;
   float _fade_elapsed_ms = 0.0f;

   // set once a fade-out has silenced the music; updateMusic() then starts no further track
   // until the playlist is restarted (restartPlayListAfterFadeOut, next/previous track)
   bool _music_halted = false;

   // the track playTrack() asked for that hasn't started yet
   std::optional<std::size_t> _pending_track;

   // at most one decode runs at a time; _decoded holds a finished one (requested or prefetched)
   std::future<std::unique_ptr<DecodedTrack>> _decode_future;
   std::size_t _decode_index = 0;
   std::shared_ptr<std::atomic_bool> _decode_cancel;
   std::unique_ptr<DecodedTrack> _decoded;

   // input format of the playing track, sizes the prefetch threshold
   SDL_AudioSpec _music_spec{};
};
