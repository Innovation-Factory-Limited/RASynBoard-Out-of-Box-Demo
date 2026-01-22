# Integration Guide - Adding Sound Detection to ndp_thread_entry.c

This guide shows you exactly how to integrate the sound detection modules into the main NDP thread.

## Overview

We need to modify `src/ndp_thread_entry.c` to:
1. Include new module headers
2. Add global state variables
3. Initialize modules at startup
4. Trigger detection flow on sound events
5. Manage audio recording and transfer

---

## Step 1: Add Include Statements

**Location**: Top of `src/ndp_thread_entry.c` (after line 15)

**Add these includes**:
```c
#include "iotc_thread_entry.h"

// ===== NEW: Sound Detection System Modules =====
#include "rp2040_signal.h"
#include "circular_audio_buffer.h"
#include "pi5_uart_comm.h"
// ================================================
```

---

## Step 2: Add Global Variables

**Location**: After the existing typedefs (after line ~62)

**Add these globals**:
```c
// Structure to hold inference data and inference counts
static inferenceData_t inferenceData[SYNTIANT_NDP120_MAX_NNETWORKS][SYNTIANT_NDP120_MAX_CLASSES];

// ===== NEW: Sound Detection System State =====
static circular_audio_buffer_t g_audio_buffer;
static bool g_pi5_ready = false;
static bool g_recording_active = false;
// ==============================================
```

---

## Step 3: Add Audio Extraction Callback

**Location**: Add this new function before `ndp_thread_entry()` (around line 290)

```c
/**
 * @brief Callback for audio extraction to circular buffer
 *
 * Called by NDP120 platform when audio data is ready.
 * Writes PCM audio data to the circular buffer.
 */
void audio_buffer_callback(uint32_t extract_size, uint8_t *audio_data, void *arg)
{
    circular_audio_buffer_t *buffer = (circular_audio_buffer_t *)arg;

    if (extract_size > 0) {
        uint32_t data_size;
        int audio_type = ndp_core2_platform_tiny_src_type(audio_data, &data_size);

        if (audio_type == NDP_CORE2_FLOW_SRC_TYPE_PCM0) {
            // Write PCM audio to circular buffer
            uint32_t written = circular_buffer_write(buffer, audio_data, data_size);

            if (buffer->overflow) {
                printf("W"); // Warning: buffer overflow
            }
        }
    }
}
```

---

## Step 4: Add Recording Management Function

**Add this function after the callback** (before `ndp_thread_entry()`):

```c
/**
 * @brief Manage audio recording and transfer to Pi5
 *
 * Extracts audio from NDP120 to circular buffer until Pi5 signals ready,
 * then transfers the buffered audio via UART.
 */
void manage_audio_recording(void)
{
    int s;
    uint8_t *data_ptr = NULL;
    uint32_t sample_size, audio_chunk_size;

    printf("=== Starting audio recording manager ===\n");

    // Allocate extraction buffer
    data_ptr = pvPortMalloc(2048);
    if (!data_ptr) {
        printf("ERROR: Failed to allocate audio extraction buffer\n");
        return;
    }

    // Get audio chunk configuration
    s = ndp_core2_platform_tiny_get_audio_chunk_size(&audio_chunk_size, &sample_size);
    if (s) {
        printf("ERROR: Get audio chunk size failed: %d\n", s);
        vPortFree(data_ptr);
        return;
    }

    printf("Audio chunk size: %lu, sample size: %lu\n", audio_chunk_size, sample_size);

    // Record until Pi5 signals ready or timeout
    uint32_t timeout_ticks = pdMS_TO_TICKS(30000UL);  // 30 second timeout
    TickType_t start_time = xTaskGetTickCount();

    while (g_recording_active && !g_pi5_ready) {
        // Extract audio to circular buffer
        s = ndp_core2_platform_tiny_notify_extract_data(data_ptr,
                                                         sample_size,
                                                         audio_buffer_callback,
                                                         &g_audio_buffer);

        if (s == NDP_CORE2_ERROR_DATA_REREAD) {
            vTaskDelay(pdMS_TO_TICKS(1UL));
            continue;
        }

        if (s) {
            printf("ERROR: Audio extraction failed: %d\n", s);
            break;
        }

        // Check if Pi5 is ready
        if (pi5_uart_check_ready()) {
            g_pi5_ready = true;
            printf("\n>>> Pi5 ready signal received! <<<\n");
            break;
        }

        // Check timeout
        if ((xTaskGetTickCount() - start_time) > timeout_ticks) {
            printf("\nWARNING: Timeout waiting for Pi5 (30 sec)\n");
            break;
        }

        // Show buffer fill level periodically
        static uint32_t last_print = 0;
        if ((xTaskGetTickCount() - last_print) > pdMS_TO_TICKS(1000UL)) {
            printf("Buffer: %lu / %lu bytes\n",
                   circular_buffer_available(&g_audio_buffer),
                   AUDIO_BUFFER_SIZE);
            last_print = xTaskGetTickCount();
        }

        vTaskDelay(pdMS_TO_TICKS(10UL));
    }

    // Stop recording
    circular_buffer_stop_recording(&g_audio_buffer);
    printf("Recording stopped. Buffer contains: %lu bytes\n",
           circular_buffer_available(&g_audio_buffer));

    // Transfer if Pi5 is ready
    if (g_pi5_ready) {
        pi5_uart_send_audio_buffer(&g_audio_buffer);
    } else {
        printf("Pi5 not ready - audio not transferred\n");
    }

    // Clean up
    vPortFree(data_ptr);
    g_recording_active = false;
    g_pi5_ready = false;

    // Turn off recording LED
    uint32_t led_event = LED_EVENT_NONE;
    xQueueSend(g_led_queue, (void *)&led_event, 0U);

    printf("=== Audio recording cycle complete ===\n\n");
}
```

---

## Step 5: Initialize Modules in ndp_thread_entry()

**Location**: Find the line after `ndp_irq_enable();` (around line 417)

**Add initialization**:
```c
    /* Enable NDP IRQ */
    ndp_irq_enable();

    // ===== NEW: Initialize Sound Detection Modules =====
    printf("\n=== Initializing Sound Detection System ===\n");
    circular_buffer_init(&g_audio_buffer);
    rp2040_signal_init();
    pi5_uart_init();
    printf("Sound detection system ready\n\n");
    // ===================================================

    /* Start USB thread to enable CDC serial communication and MSC mass storage function */
    start_usb_pcdc_thread();
```

---

## Step 6: Modify Sound Detection Handler

**Location**: Find the `if( evbits & EVENT_BIT_VOICE )` section (around line 432)

**Replace the existing handler with**:

```c
        if( evbits & EVENT_BIT_VOICE )
        {
            xSemaphoreTake(g_ndp_mutex,portMAX_DELAY);
            ndp_core2_platform_tiny_poll(&notifications, 1, &fatal_error);
            if (fatal_error) {
                printf("\nNDP Fatal Error!!!\n\n");
            }

            ret = ndp_core2_platform_tiny_match_process(&ndp_nn_idx, &ndp_class_idx, &sec_val, NULL);
            if (!ret) {
                printf("\nNDP MATCH!!! -- [%d:%d]:%s %s sec-val\n\n",
                    ndp_nn_idx, ndp_class_idx, labels_per_network[ndp_nn_idx][ndp_class_idx],
                    (sec_val>0)?"with":"without");

                // ===== NEW: Sound Detection Flow =====
                if (!g_recording_active) {
                    printf("\n>>> SOUND DETECTED - Starting wake sequence <<<\n");

                    // Step 1: Signal RP2040 to wake Pi5
                    rp2040_signal_wake();

                    // Step 2: Start circular buffer recording
                    circular_buffer_start_recording(&g_audio_buffer);
                    g_recording_active = true;

                    // Step 3: Turn on recording LED (green)
                    uint32_t led_event = LED_COLOR_GREEN;
                    xQueueSend(g_led_queue, (void *)&led_event, 0U);

                    // Step 4: Start audio recording task
                    printf("Starting audio recording to buffer...\n");

                    // Option A: Run in separate task (recommended)
                    xTaskCreate((TaskFunction_t)manage_audio_recording,
                                "AudioRec",
                                2048,       // Stack size
                                NULL,       // Parameters
                                3,          // Priority
                                NULL);      // Task handle

                    // Option B: Run inline (simpler but blocks)
                    // manage_audio_recording();
                } else {
                    printf("Recording already active - ignoring detection\n");
                }
                // ======================================

                // Original code for non-recording flows
                // (Keep this for compatibility with existing features)
                /*
                switch (ndp_class_idx) {
                    case 0:
                    case 1:
                    // ... rest of original switch cases ...
                }
                */
            }
            xSemaphoreGive(g_ndp_mutex);

            // Original semaphore give (keep this)
            xSemaphoreGive(g_binary_semaphore);
            memcpy(&last_stat, &current_stat, sizeof(blink_msg_t));
        }
```

**Note**: You can keep the original `switch(ndp_class_idx)` cases if you want to support both the sound detection system AND the original keyword detection features simultaneously.

---

## Step 7: Optional - Add Manual Test Trigger

For testing without waiting for actual sound detection, add this in the main loop:

```c
    // ===== TESTING: Manual trigger with button press =====
    // Add this inside the while(1) loop for testing
    // Remove or comment out in production

    #ifdef ENABLE_MANUAL_TEST_TRIGGER
    if (some_test_condition) {  // e.g., button press
        printf("\n>>> MANUAL TEST TRIGGER <<<\n");
        rp2040_signal_wake();
        circular_buffer_start_recording(&g_audio_buffer);
        g_recording_active = true;
        manage_audio_recording();
    }
    #endif
    // ====================================================
```

---

## Complete Integration Checklist

Before testing:

- [x] FSP configuration complete (GPIO P002, SCI4, LPM) - **Already done**
- [x] UART code functional in `pi5_uart_comm.c` - **Uses existing g_uart4**
- [ ] config.ini updated: `[Debug Print] Port=2` (USB-VCOM for debug)
- [ ] Hardware wired: J8 Pin3→Pi5 RX, J8 Pin4→Pi5 TX, J8 Pin5→GND
- [x] Includes added to `ndp_thread_entry.c` - **Done**
- [x] Global variables added - **Done**
- [x] Callback functions added - **Done**
- [x] Module initialization added - **Done**
- [x] Detection handler modified - **Done**
- [ ] Project builds without errors

---

## Testing the Integration

### Test 1: Verify Initialization
Flash the firmware and check serial output (via USB-VCOM):
```
=== Initializing Sound Detection System ===
RP2040 signal initialized on P002
Pi5 UART initialized @ 115200 baud
Hardware connection via J8 Pmod:
  Pin 3 (P205/TXD4) -> Pi5 RXD
  Pin 4 (P206/RXD4) -> Pi5 TXD
  Pin 5 (GND)       -> Pi5 GND
Sound detection system ready
```

### Test 2: Trigger Detection
Make a loud sound (clap, whistle) near the microphone:
```
NDP MATCH!!! -- [0:0]:NN0:ok-syntiant with sec-val

>>> SOUND DETECTED - Starting wake sequence <<<
Signaling RP2040 to wake Pi5...
RP2040 wake signal sent (100ms pulse)
Starting audio recording to buffer...
```

### Test 3: Verify GPIO Pulse
Use oscilloscope on P002:
- Should see 100ms HIGH pulse (3.3V)
- Immediately after sound detection

### Test 4: Verify Recording
Check serial output:
```
=== Starting audio recording manager ===
Audio chunk size: 512, sample size: 512
Buffer: 2048 / 262144 bytes
Buffer: 4096 / 262144 bytes
...
```

### Test 5: Simulate Pi5 Ready
Send `0xA5` via J8 Pmod UART (P205/P206) from PC terminal or Pi5:
```
Pi5 ready signal received (0xA5)
Sent ACK to Pi5 (0x5A)

>>> Pi5 ready signal received! <<<
Recording stopped. Buffer contains: 32768 bytes
=== Starting audio transfer to Pi5 ===
...
=== Transfer complete: 32768 bytes ===
```

---

## Troubleshooting

### Issue: Modules not initializing
**Symptom**: No "Sound detection system ready" message

**Solution**:
- Verify config.ini has `Port=2` to redirect debug to USB-VCOM
- Check USB cable connected to Core Board for debug output
- Look for error messages in serial output

### Issue: No audio data in buffer
**Symptom**: "Buffer contains: 0 bytes"

**Solution**:
- Verify PDM feature is enabled: `get_event_watch_mode() & WATCH_TYPE_AUDIO`
- Check NDP120 is properly initialized
- Verify audio extraction callback is being called

### Issue: Recording never stops
**Symptom**: Buffer fills but no transfer

**Solution**:
- Check Pi5 UART connection (TX→RX, RX→TX)
- Verify Pi5 sends 0xA5 ready signal
- Check 30-second timeout isn't too short

### Issue: Compilation errors
**Symptom**: Undefined references

**Solution**:
- Verify all new `.c` files are in the build
- Check FSP-generated files are included
- Clean and rebuild project

---

## Performance Monitoring

Add this to periodic debug output:

```c
// Inside the while(g_recording_active) loop
printf("Heap free: %lu bytes\n", xPortGetFreeHeapSize());
printf("Buffer: %lu / %lu (%.1f%%)\n",
       circular_buffer_available(&g_audio_buffer),
       AUDIO_BUFFER_SIZE,
       (float)circular_buffer_available(&g_audio_buffer) / AUDIO_BUFFER_SIZE * 100.0f);
```

**Expected values:**
- Heap free: > 100KB (depends on system)
- Buffer fill: Should grow steadily at 32KB/sec
- No "W" warnings (overflow)

---

## Next Steps After Integration

1. Test with real RP2040 power management HAT
2. Verify Pi5 actually boots from GPIO signal
3. Test full end-to-end flow with Pi5
4. Implement low-power mode (Phase 5)
5. Optimize power consumption
6. Test battery life

---

## Summary

After these changes, your system will:
1. ✅ Detect sound via NDP120
2. ✅ Send GPIO wake pulse to RP2040
3. ✅ Record audio to 256KB circular buffer
4. ✅ Wait for Pi5 ready signal
5. ✅ Transfer audio via UART to Pi5

The integration is modular and can be tested incrementally!
