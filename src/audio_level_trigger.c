/*
 * audio_level_trigger.c
 *
 * Simple audio level trigger - detects any loud sound above threshold
 * Extracts small audio chunk from NDP120 and checks amplitude
 */

#include "audio_level_trigger.h"
#include "syntiant_platform.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>

static uint16_t g_threshold = AUDIO_THRESHOLD_DEFAULT;
static uint16_t g_current_level = 0;
static TickType_t g_last_trigger_time = 0;
static bool g_initialized = false;

// Small buffer for audio level check
static uint8_t g_level_check_buffer[512];

void audio_level_trigger_init(void)
{
    g_threshold = AUDIO_THRESHOLD_DEFAULT;
    g_current_level = 0;
    g_last_trigger_time = 0;
    g_initialized = true;

    printf("Audio level trigger initialized (threshold: %u)\n", g_threshold);
}

void audio_level_set_threshold(uint16_t threshold)
{
    g_threshold = threshold;
    printf("Audio threshold set to: %u\n", g_threshold);
}

uint16_t audio_level_get_current(void)
{
    return g_current_level;
}

/**
 * @brief Calculate peak amplitude from PCM audio samples
 */
static uint16_t calculate_peak_amplitude(int16_t *samples, uint32_t num_samples)
{
    uint16_t peak = 0;

    for (uint32_t i = 0; i < num_samples; i++) {
        int16_t sample = samples[i];
        uint16_t abs_sample = (sample < 0) ? (uint16_t)(-sample) : (uint16_t)sample;

        if (abs_sample > peak) {
            peak = abs_sample;
        }
    }

    return peak;
}

/**
 * @brief Callback for audio extraction (just for level check)
 */
static void level_check_callback(uint32_t extract_size, uint8_t *audio_data, void *arg)
{
    uint16_t *peak_out = (uint16_t *)arg;

    if (extract_size > 0 && audio_data != NULL) {
        uint32_t data_size;
        int audio_type = ndp_core2_platform_tiny_src_type(audio_data, &data_size);

        if (audio_type == NDP_CORE2_FLOW_SRC_TYPE_PCM0 && data_size > 0) {
            // Calculate peak from PCM samples (16-bit signed)
            uint32_t num_samples = data_size / 2;
            *peak_out = calculate_peak_amplitude((int16_t *)audio_data, num_samples);
        }
    }
}

bool audio_level_check_trigger(void)
{
    if (!g_initialized) {
        return false;
    }

    // Check cooldown period
    TickType_t now = xTaskGetTickCount();
    if ((now - g_last_trigger_time) < pdMS_TO_TICKS(AUDIO_COOLDOWN_MS)) {
        return false;  // Still in cooldown
    }

    // Extract a small audio chunk and check level
    uint16_t peak = 0;
    uint32_t sample_size = sizeof(g_level_check_buffer);

    int s = ndp_core2_platform_tiny_notify_extract_data(
        g_level_check_buffer,
        sample_size,
        level_check_callback,
        &peak
    );

    if (s == NDP_CORE2_ERROR_DATA_REREAD) {
        // No new data available yet
        return false;
    }

    if (s != 0) {
        // Error extracting audio
        return false;
    }

    g_current_level = peak;

    // Check if above threshold
    if (peak > g_threshold) {
        printf("\n>>> SOUND DETECTED! Level: %u (threshold: %u) <<<\n", peak, g_threshold);
        g_last_trigger_time = now;
        return true;
    }

    return false;
}
