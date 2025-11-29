#ifndef AUDIO_OUTPUT_H
#define AUDIO_OUTPUT_H

#include "builddef.h"
#ifdef USE_FFMPEG

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Define API macro for visibility export
#ifndef __MINGW32__
#define API __attribute__((visibility("default")))
#else
#define API __declspec(dllexport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct audio_output audio_output;

// sample_fmt is AVSampleFormat from libavutil, but we use int to avoid include issues
API audio_output* audio_output_init(int sample_rate, int channels, int sample_fmt);
API void audio_output_start(audio_output* ao);
API void audio_output_pause(audio_output* ao);
API void audio_output_resume(audio_output* ao);
API int audio_output_write(audio_output* ao, const uint8_t* data, size_t len);
API double audio_output_get_clock(audio_output* ao);
API void audio_output_set_pts(audio_output* ao, uint64_t pts, double time_base);
API void audio_output_flush(audio_output* ao);
API void audio_output_destroy(audio_output* ao);
API audio_output* audio_output_get_global(void);
// Check if audio buffer needs more data (returns true if buffer is less than 50% full)
API bool audio_output_needs_data(audio_output* ao);
API int audio_output_get_sample_rate(audio_output* ao);
API int audio_output_get_channels(audio_output* ao);

#ifdef __cplusplus
}
#endif

#endif // USE_FFMPEG
#endif // AUDIO_OUTPUT_H

