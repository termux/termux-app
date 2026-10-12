#ifndef VORBISLIB_H
#define VORBISLIB_H
#include <stddef.h>
#include <stdint.h>
#if defined(_WIN32) && defined(VORBISLIB_SHARED)
# ifdef VORBISLIB_BUILD
#  define VL_API __declspec(dllexport)
# else
#  define VL_API __declspec(dllimport)
# endif
#elif defined(__GNUC__)
# define VL_API __attribute__((visibility("default")))
#else
# define VL_API
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef enum vl_status {
    VL_OK = 0, VL_INVALID_ARGUMENT = 1, VL_INVALID_DATA = 2,
    VL_UNSUPPORTED = 3, VL_OUT_OF_MEMORY = 4, VL_CODEC_ERROR = 5
} vl_status;
typedef enum vl_codebooks { VL_AOTUV_603 = 0, VL_LEGACY_CODEBOOKS = 1 } vl_codebooks;
typedef struct vl_buffer { uint8_t *data; size_t size; } vl_buffer;
typedef struct vl_audio {
    float *samples; /* interleaved; Wwise channel order; caller frees with vl_free */
    uint32_t frames;
    uint32_t sample_rate;
    uint32_t channels;
} vl_audio;

/* Thread-local diagnostic; valid until the next operation on this thread. */
VL_API const char *vl_last_error(void);
VL_API const char *vl_version(void);
VL_API void vl_free(void *allocation);

/* PCM -> modern Wwise custom Vorbis WEM (0xFFFF, fmt size 0x42).
 * Encoder supports mono/stereo, 8000..48000 Hz, quality -0.1..1.0.
 * Some rate/quality combinations may have codebooks absent from Wwise's table;
 * those return VL_UNSUPPORTED. PCM must be finite and within [-1,1].
 * Output is allocated only on success; initialize output to zero and free any
 * previous allocation before reusing it. No Wwise engine/desktop process needed.
 */
VL_API vl_status vl_encode(const float *pcm, uint32_t frames,
    uint32_t channels, uint32_t sample_rate, float quality, vl_buffer *output);

/* Complete WAV/OGG file bytes -> modern Wwise custom Vorbis WEM.
 * Mono/stereo, 8000..48000 Hz; no resampling or downmixing.
 * max_frames bounds decoded PCM allocation and must be nonzero.
 * WAV: little-endian RIFF, PCM 8/16/24/32-bit, IEEE float 32/64-bit,
 * including WAVE_FORMAT_EXTENSIBLE PCM/float. Float WAV must be finite
 * and within [-1,1]. OGG: one complete Ogg Vorbis logical stream; decoded
 * overshoot is clipped to [-1,1] before lossy re-encoding. Chaining,
 * multiplexing, Opus, damaged/truncated streams are rejected.
 * Source metadata (loops, tags, cues) is not retained. Ownership and
 * failure semantics are the same as vl_encode.
 */
VL_API vl_status vl_wav_to_wem(const uint8_t *wav, size_t size,
    float quality, uint32_t max_frames, vl_buffer *output);
VL_API vl_status vl_ogg_to_wem(const uint8_t *ogg, size_t size,
    float quality, uint32_t max_frames, vl_buffer *output);

/* Detect WAV or Ogg from file bytes, independent of filename extension. */
VL_API vl_status vl_audio_to_wem(const uint8_t *audio, size_t size,
    float quality, uint32_t max_frames, vl_buffer *output);

/* Complete, little-endian modern WEM -> PCM. Supports packed external codebooks,
 * stripped setup and modified audio packets. No banks, prefetch fragments,
 * Opus, ADPCM, RIFX or old header-triad WEMs. Pass codebook set explicitly.
 * max_frames is a required allocation limit, not a truncation request.
 */
VL_API vl_status vl_decode(const uint8_t *wem, size_t size,
    vl_codebooks codebooks, uint32_t max_frames, vl_audio *output);

/* Rebuild a standard Ogg Vorbis stream without re-encoding audio.
 * Recomputes granules and trims to the WEM sample count. */
VL_API vl_status vl_wem_to_ogg(const uint8_t *wem, size_t size,
    vl_codebooks codebooks, vl_buffer *output);

#ifdef __cplusplus
}
#endif
#endif
