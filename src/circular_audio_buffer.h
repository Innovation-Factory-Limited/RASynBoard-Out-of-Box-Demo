/*
 * circular_audio_buffer.h
 *
 * Circular audio buffer for storing recorded audio samples
 * while waiting for Pi5 to boot and become ready
 *
 * Buffer size: 256KB = 8 seconds @ 16kHz 16-bit PCM
 *
 * Created for: Low-Power Sound Detection System
 */

#ifndef CIRCULAR_AUDIO_BUFFER_H_
#define CIRCULAR_AUDIO_BUFFER_H_

#include <stdint.h>
#include <stdbool.h>

// Audio buffer configuration
#define AUDIO_BUFFER_SIZE (256 * 1024)  // 256KB = 8 seconds @ 16kHz 16-bit
#define AUDIO_SAMPLE_RATE 16000
#define AUDIO_BYTES_PER_SAMPLE 2

/**
 * @brief Circular audio buffer structure
 *
 * Manages a circular buffer for audio data with write and read pointers.
 * Handles wrap-around automatically and tracks buffer fullness.
 */
typedef struct {
    uint8_t buffer[AUDIO_BUFFER_SIZE];  // Audio data buffer
    uint32_t write_index;                // Write position (0 to AUDIO_BUFFER_SIZE-1)
    uint32_t read_index;                 // Read position (0 to AUDIO_BUFFER_SIZE-1)
    uint32_t count;                      // Number of bytes currently in buffer
    bool recording;                      // Recording active flag
    bool overflow;                       // Overflow occurred flag
} circular_audio_buffer_t;

/**
 * @brief Initialize circular audio buffer
 *
 * @param cb Pointer to circular audio buffer structure
 */
void circular_buffer_init(circular_audio_buffer_t *cb);

/**
 * @brief Write audio data to circular buffer
 *
 * @param cb Pointer to circular audio buffer structure
 * @param data Pointer to audio data to write
 * @param len Number of bytes to write
 * @return Number of bytes actually written
 */
uint32_t circular_buffer_write(circular_audio_buffer_t *cb, const uint8_t *data, uint32_t len);

/**
 * @brief Read audio data from circular buffer
 *
 * @param cb Pointer to circular audio buffer structure
 * @param data Pointer to destination buffer
 * @param len Number of bytes to read
 * @return Number of bytes actually read
 */
uint32_t circular_buffer_read(circular_audio_buffer_t *cb, uint8_t *data, uint32_t len);

/**
 * @brief Get number of bytes available in buffer
 *
 * @param cb Pointer to circular audio buffer structure
 * @return Number of bytes available to read
 */
uint32_t circular_buffer_available(circular_audio_buffer_t *cb);

/**
 * @brief Start recording to circular buffer
 *
 * @param cb Pointer to circular audio buffer structure
 */
void circular_buffer_start_recording(circular_audio_buffer_t *cb);

/**
 * @brief Stop recording to circular buffer
 *
 * @param cb Pointer to circular audio buffer structure
 */
void circular_buffer_stop_recording(circular_audio_buffer_t *cb);

/**
 * @brief Check if recording is active
 *
 * @param cb Pointer to circular audio buffer structure
 * @return true if recording, false otherwise
 */
bool circular_buffer_is_recording(circular_audio_buffer_t *cb);

#endif /* CIRCULAR_AUDIO_BUFFER_H_ */
