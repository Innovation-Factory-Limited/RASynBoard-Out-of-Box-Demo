/*
 * circular_audio_buffer.c
 *
 * Circular audio buffer implementation for storing recorded audio samples
 * while waiting for Pi5 to boot and become ready
 *
 * Created for: Low-Power Sound Detection System
 */

#include "circular_audio_buffer.h"
#include <string.h>

void circular_buffer_init(circular_audio_buffer_t *cb)
{
    cb->write_index = 0;
    cb->read_index = 0;
    cb->count = 0;
    cb->recording = false;
    cb->overflow = false;

    // Clear buffer memory
    memset(cb->buffer, 0, AUDIO_BUFFER_SIZE);
}

uint32_t circular_buffer_write(circular_audio_buffer_t *cb, const uint8_t *data, uint32_t len)
{
    // Don't write if not recording
    if (!cb->recording) {
        return 0;
    }

    // Calculate available space
    uint32_t space_available = AUDIO_BUFFER_SIZE - cb->count;
    uint32_t to_write = (len < space_available) ? len : space_available;

    // Check for overflow condition
    if (to_write < len) {
        cb->overflow = true;  // Signal that we're dropping data
    }

    // Write data with wrap-around handling
    for (uint32_t i = 0; i < to_write; i++) {
        cb->buffer[cb->write_index] = data[i];
        cb->write_index = (cb->write_index + 1) % AUDIO_BUFFER_SIZE;
    }

    // Update byte count
    cb->count += to_write;

    return to_write;
}

uint32_t circular_buffer_read(circular_audio_buffer_t *cb, uint8_t *data, uint32_t len)
{
    // Calculate available data to read
    uint32_t to_read = (len < cb->count) ? len : cb->count;

    // Read data with wrap-around handling
    for (uint32_t i = 0; i < to_read; i++) {
        data[i] = cb->buffer[cb->read_index];
        cb->read_index = (cb->read_index + 1) % AUDIO_BUFFER_SIZE;
    }

    // Update byte count
    cb->count -= to_read;

    return to_read;
}

uint32_t circular_buffer_available(circular_audio_buffer_t *cb)
{
    return cb->count;
}

void circular_buffer_start_recording(circular_audio_buffer_t *cb)
{
    cb->recording = true;
    cb->overflow = false;
}

void circular_buffer_stop_recording(circular_audio_buffer_t *cb)
{
    cb->recording = false;
}

bool circular_buffer_is_recording(circular_audio_buffer_t *cb)
{
    return cb->recording;
}
