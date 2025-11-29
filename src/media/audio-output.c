#include "builddef.h"
#ifdef USE_FFMPEG

#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#include <libavutil/samplefmt.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#include "lib/internal.h"

// Define API macro for visibility export
#ifndef __MINGW32__
#define API __attribute__((visibility("default")))
#else
#define API __declspec(dllexport)
#endif

#define AUDIO_BUFFER_SIZE 4096

// SDL2 types - defined locally to avoid including SDL2/SDL.h (which would cause SDL2 to be linked)
typedef uint32_t SDL_AudioDeviceID;
typedef struct SDL_AudioSpec {
  int freq;
  uint16_t format;
  uint8_t channels;
  uint8_t silence;
  uint16_t samples;
  uint16_t padding;
  uint32_t size;
  void (*callback)(void* userdata, uint8_t* stream, int len);
  void* userdata;
} SDL_AudioSpec;

#define SDL_INIT_AUDIO 0x00000010
#define SDL_AUDIO_ALLOW_FREQUENCY_CHANGE 0x00000001
#define SDL_AUDIO_ALLOW_CHANNELS_CHANGE 0x00000002
#define AUDIO_S16SYS 0x8010

// SDL2 function pointers - loaded dynamically to avoid loading SDL2 at library load time
typedef struct {
  void* handle;
  int (*SDL_Init)(uint32_t);
  void (*SDL_QuitSubSystem)(uint32_t);
  const char* (*SDL_GetError)(void);
  SDL_AudioDeviceID (*SDL_OpenAudioDevice)(const char*, int, const SDL_AudioSpec*, SDL_AudioSpec*, int);
  void (*SDL_PauseAudioDevice)(SDL_AudioDeviceID, int);
  void (*SDL_CloseAudioDevice)(SDL_AudioDeviceID);
  int (*SDL_SetHint)(const char*, const char*);
} sdl2_functions;

static sdl2_functions sdl2 = {0};

typedef struct audio_output {
  SDL_AudioDeviceID device_id;
  SDL_AudioSpec want_spec;
  SDL_AudioSpec have_spec;
  uint8_t* buffer;
  size_t buffer_size;
  size_t buffer_pos;
  size_t buffer_used;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  bool playing;
  bool paused;
  double audio_clock;  // Current audio position in seconds
  uint64_t audio_pts;  // Audio PTS in timebase units
  int sample_rate;
  int channels;
  int sample_fmt;  // AVSampleFormat (using int to avoid include issues)
} audio_output;

static audio_output* g_audio = NULL;
static bool sdl_initialized = false;

static void audio_callback(void* userdata, uint8_t* stream, int len) {
  static int callback_count = 0;
  callback_count++;
  audio_output* ao = (audio_output*)userdata;
  if (!ao || !ao->playing) {
    memset(stream, 0, len);
    if(callback_count <= 10){
      FILE* logfile = fopen("/tmp/ncplayer_audio.log", "a");
      if(logfile){
        fprintf(logfile, "audio_callback: Not playing (call %d, ao=%p, playing=%d)\n", callback_count, ao, ao ? ao->playing : 0);
        fclose(logfile);
      }
    }
    return;
  }

  if(callback_count <= 10 || callback_count % 1000 == 0){
    FILE* logfile = fopen("/tmp/ncplayer_audio.log", "a");
    if(logfile){
      fprintf(logfile, "audio_callback: Call %d, len=%d, buffer_used=%zu\n", callback_count, len, ao->buffer_used);
      fclose(logfile);
    }
  }

  pthread_mutex_lock(&ao->mutex);

  size_t bytes_to_write = len;
  size_t bytes_available = ao->buffer_used;

  if (bytes_available == 0) {
    // Buffer underrun - fill with silence
    memset(stream, 0, len);
    pthread_mutex_unlock(&ao->mutex);
    return;
  }

  if (bytes_to_write > bytes_available) {
    bytes_to_write = bytes_available;
  }

  memcpy(stream, ao->buffer + ao->buffer_pos, bytes_to_write);
  ao->buffer_pos += bytes_to_write;
  ao->buffer_used -= bytes_to_write;

  // Keep buffer data contiguous at the front to simplify writes
  if (ao->buffer_used > 0 && ao->buffer_pos > 0) {
    memmove(ao->buffer, ao->buffer + ao->buffer_pos, ao->buffer_used);
  }
  ao->buffer_pos = 0;

  // Update audio clock based on bytes written
  if (ao->sample_rate > 0 && ao->channels > 0) {
    double samples_written = bytes_to_write / (ao->channels * sizeof(int16_t));
    ao->audio_clock += samples_written / ao->sample_rate;
  }

  pthread_cond_signal(&ao->cond);
  pthread_mutex_unlock(&ao->mutex);

  // Fill remaining with silence if needed
  if (bytes_to_write < (size_t)len) {
    memset(stream + bytes_to_write, 0, len - bytes_to_write);
  }
}

// Load SDL2 dynamically to avoid loading it at program startup
static int load_sdl2(void) {
  if (sdl2.handle) {
    return 0; // Already loaded
  }

  sdl2.handle = dlopen("libSDL2-2.0.so.0", RTLD_LAZY);
  if (!sdl2.handle) {
    sdl2.handle = dlopen("libSDL2.so", RTLD_LAZY);
  }
  if (!sdl2.handle) {
    logerror("Failed to load SDL2: %s", dlerror());
    return -1;
  }

  // Load SDL2 functions
  sdl2.SDL_SetHint = (int (*)(const char*, const char*))dlsym(sdl2.handle, "SDL_SetHint");
  sdl2.SDL_Init = (int (*)(uint32_t))dlsym(sdl2.handle, "SDL_Init");
  sdl2.SDL_GetError = (const char* (*)(void))dlsym(sdl2.handle, "SDL_GetError");
  sdl2.SDL_OpenAudioDevice = (SDL_AudioDeviceID (*)(const char*, int, const SDL_AudioSpec*, SDL_AudioSpec*, int))dlsym(sdl2.handle, "SDL_OpenAudioDevice");
  sdl2.SDL_PauseAudioDevice = (void (*)(SDL_AudioDeviceID, int))dlsym(sdl2.handle, "SDL_PauseAudioDevice");
  sdl2.SDL_CloseAudioDevice = (void (*)(SDL_AudioDeviceID))dlsym(sdl2.handle, "SDL_CloseAudioDevice");
  sdl2.SDL_QuitSubSystem = (void (*)(uint32_t))dlsym(sdl2.handle, "SDL_QuitSubSystem");

  if (!sdl2.SDL_Init || !sdl2.SDL_GetError || !sdl2.SDL_OpenAudioDevice ||
      !sdl2.SDL_PauseAudioDevice || !sdl2.SDL_CloseAudioDevice || !sdl2.SDL_QuitSubSystem) {
    logerror("Failed to load SDL2 functions: %s", dlerror());
    dlclose(sdl2.handle);
    sdl2.handle = NULL;
    return -1;
  }

  return 0;
}

API audio_output* audio_output_init(int sample_rate, int channels, int sample_fmt) {
  // Load SDL2 dynamically (only when audio is actually needed)
  if (load_sdl2() < 0) {
    return NULL;
  }

  // Initialize SDL2 audio subsystem only once
  // Use SDL_INIT_AUDIO only (not SDL_INIT_EVERYTHING) to avoid interfering with terminal
  if (!sdl_initialized) {
    // Don't let SDL2 install signal handlers that might interfere with terminal
    if (sdl2.SDL_SetHint) {
      sdl2.SDL_SetHint("SDL_HINT_NO_SIGNAL_HANDLERS", "1");
    }
    if (sdl2.SDL_Init(SDL_INIT_AUDIO) < 0) {
      logerror("SDL audio init failed: %s", sdl2.SDL_GetError());
      return NULL;
    }
    sdl_initialized = true;
  }

  audio_output* ao = calloc(1, sizeof(audio_output));
  if (!ao) {
    return NULL;
  }

  ao->sample_rate = sample_rate;
  ao->channels = channels;
  ao->sample_fmt = sample_fmt;

  memset(&ao->want_spec, 0, sizeof(ao->want_spec));
  ao->want_spec.freq = sample_rate;
  ao->want_spec.format = AUDIO_S16SYS;  // Signed 16-bit samples, system byte order
  ao->want_spec.channels = channels;
  ao->want_spec.samples = AUDIO_BUFFER_SIZE;
  ao->want_spec.callback = audio_callback;
  ao->want_spec.userdata = ao;

  ao->device_id = sdl2.SDL_OpenAudioDevice(NULL, 0, &ao->want_spec, &ao->have_spec,
                                      SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
  if (ao->device_id == 0) {
    logerror("SDL_OpenAudioDevice failed: %s", sdl2.SDL_GetError());
    free(ao);
    return NULL;
  }

  // Update to actual specs
  ao->sample_rate = ao->have_spec.freq;
  ao->channels = ao->have_spec.channels;

  ao->buffer_size = sample_rate * channels * sizeof(int16_t) * 2; // 2 seconds buffer
  ao->buffer = malloc(ao->buffer_size);
  if (!ao->buffer) {
    sdl2.SDL_CloseAudioDevice(ao->device_id);
    free(ao);
    return NULL;
  }

  if (pthread_mutex_init(&ao->mutex, NULL) != 0) {
    free(ao->buffer);
    sdl2.SDL_CloseAudioDevice(ao->device_id);
    free(ao);
    return NULL;
  }

  if (pthread_cond_init(&ao->cond, NULL) != 0) {
    pthread_mutex_destroy(&ao->mutex);
    free(ao->buffer);
    sdl2.SDL_CloseAudioDevice(ao->device_id);
    free(ao);
    return NULL;
  }

  ao->playing = false;
  ao->paused = false;
  ao->buffer_pos = 0;
  ao->buffer_used = 0;
  ao->audio_clock = 0.0;

  g_audio = ao;
  return ao;
}

API void audio_output_start(audio_output* ao) {
  if (!ao) return;
  ao->playing = true;
  ao->paused = false;
  if(sdl2.SDL_PauseAudioDevice){
    sdl2.SDL_PauseAudioDevice(ao->device_id, 0);
    FILE* logfile = fopen("/tmp/ncplayer_audio.log", "a");
    if(logfile){
      fprintf(logfile, "audio_output_start: SDL_PauseAudioDevice called, device_id=%u\n", ao->device_id);
      fclose(logfile);
    }
  }
}

API void audio_output_pause(audio_output* ao) {
  if (!ao) return;
  ao->paused = true;
  sdl2.SDL_PauseAudioDevice(ao->device_id, 1);
}

API void audio_output_resume(audio_output* ao) {
  if (!ao) return;
  ao->paused = false;
  sdl2.SDL_PauseAudioDevice(ao->device_id, 0);
}

API int audio_output_write(audio_output* ao, const uint8_t* data, size_t len) {
  if (!ao || !data) return -1;

  pthread_mutex_lock(&ao->mutex);

  // Wait if buffer is too full
  size_t available_space = ao->buffer_size - ao->buffer_used;
  while (len > available_space) {
    pthread_cond_wait(&ao->cond, &ao->mutex);
    if (!ao->playing) {
      pthread_mutex_unlock(&ao->mutex);
      return -1;
    }
    available_space = ao->buffer_size - ao->buffer_used;
  }

  // Ensure data is contiguous at buffer start before writing
  if (ao->buffer_pos > 0 && ao->buffer_used > 0) {
    memmove(ao->buffer, ao->buffer + ao->buffer_pos, ao->buffer_used);
    ao->buffer_pos = 0;
  }

  memcpy(ao->buffer + ao->buffer_used, data, len);
  ao->buffer_used += len;

  pthread_mutex_unlock(&ao->mutex);
  return 0;
}

API double audio_output_get_clock(audio_output* ao) {
  if (!ao) return 0.0;
  pthread_mutex_lock(&ao->mutex);
  double clock = ao->audio_clock;
  pthread_mutex_unlock(&ao->mutex);
  return clock;
}

API void audio_output_set_pts(audio_output* ao, uint64_t pts, double time_base) {
  if (!ao) return;
  pthread_mutex_lock(&ao->mutex);
  ao->audio_pts = pts;
  ao->audio_clock = pts * time_base;
  pthread_mutex_unlock(&ao->mutex);
}

API void audio_output_flush(audio_output* ao) {
  if (!ao) return;
  pthread_mutex_lock(&ao->mutex);
  ao->buffer_pos = 0;
  ao->buffer_used = 0;
  ao->audio_clock = 0.0;
  pthread_mutex_unlock(&ao->mutex);
}

API void audio_output_destroy(audio_output* ao) {
  if (!ao) return;

  ao->playing = false;
  pthread_cond_broadcast(&ao->cond);

  sdl2.SDL_CloseAudioDevice(ao->device_id);
  pthread_mutex_destroy(&ao->mutex);
  pthread_cond_destroy(&ao->cond);
  free(ao->buffer);
  free(ao);

  if (g_audio == ao) {
    g_audio = NULL;
  }

  // Don't quit SDL subsystem here - it might be used by other instances
  // Only quit if this was the last instance
  if (g_audio == NULL && sdl2.handle) {
    sdl2.SDL_QuitSubSystem(SDL_INIT_AUDIO);
    sdl_initialized = false;
    dlclose(sdl2.handle);
    memset(&sdl2, 0, sizeof(sdl2));
  }
}

API audio_output* audio_output_get_global(void) {
  return g_audio;
}

API bool audio_output_needs_data(audio_output* ao) {
  if (!ao) return false;
  pthread_mutex_lock(&ao->mutex);
  // Need more data if buffer is less than 50% full
  bool needs_data = (ao->buffer_used < ao->buffer_size / 2);
  pthread_mutex_unlock(&ao->mutex);
  return needs_data;
}

API int audio_output_get_sample_rate(audio_output* ao) {
  if (!ao) return 0;
  pthread_mutex_lock(&ao->mutex);
  int rate = ao->sample_rate;
  pthread_mutex_unlock(&ao->mutex);
  return rate;
}

API int audio_output_get_channels(audio_output* ao) {
  if (!ao) return 0;
  pthread_mutex_lock(&ao->mutex);
  int ch = ao->channels;
  pthread_mutex_unlock(&ao->mutex);
  return ch;
}

#endif // USE_FFMPEG

