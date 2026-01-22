/*
 * audio_level_trigger.h
 *
 * Simple audio level trigger - detects any loud sound above threshold
 * No ML model required - just checks audio amplitude
 */

#ifndef AUDIO_LEVEL_TRIGGER_H_
#define AUDIO_LEVEL_TRIGGER_H_

#include <stdint.h>
#include <stdbool.h>

// Default threshold (adjustable via config.ini in future)
#define AUDIO_THRESHOLD_DEFAULT     2000    // Raw PCM amplitude (0-32767 for 16-bit)
#define AUDIO_COOLDOWN_MS           5000    // Minimum time between triggers (5 sec)

/**
 * @brief Initialize audio level trigger
 */
void audio_level_trigger_init(void);

/**
 * @brief Check audio level and trigger if above threshold
 *
 * Call this periodically to monitor audio levels.
 * Returns true if sound detected and trigger should fire.
 *
 * @return true if loud sound detected, false otherwise
 */
bool audio_level_check_trigger(void);

/**
 * @brief Set the amplitude threshold
 * @param threshold Raw PCM amplitude threshold (0-32767)
 */
void audio_level_set_threshold(uint16_t threshold);

/**
 * @brief Get current audio level (for debugging)
 * @return Current peak amplitude
 */
uint16_t audio_level_get_current(void);

#endif /* AUDIO_LEVEL_TRIGGER_H_ */
