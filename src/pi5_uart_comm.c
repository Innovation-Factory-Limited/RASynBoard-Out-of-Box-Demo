/*
 * pi5_uart_comm.c
 *
 * UART communication module for transferring audio data to Raspberry Pi 5
 * Uses SCI4 (P205/P206) via J8 Pmod connector
 *
 * Hardware Connection:
 *   J8 Pin 3 (P205/TXD4) -> Pi5 RXD
 *   J8 Pin 4 (P206/RXD4) -> Pi5 TXD
 *   J8 Pin 5 (GND)       -> Pi5 GND
 *
 * Created for: Low-Power Sound Detection System
 */

#include "pi5_uart_comm.h"
#include "hal_data.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

// Use existing g_uart4 (SCI4) which is already configured in FSP
// This UART is normally used for debug console, but we repurpose it for Pi5
// when debug is redirected to USB-VCOM (config.ini: Port=2)

static volatile bool uart_tx_complete = true;  // Start true for first transmission
static volatile bool uart_rx_ready = false;
static volatile bool uart_initialized = false;
static uint8_t rx_buffer[16];

/**
 * @brief UART callback for transmit and receive events
 *
 * This callback is invoked by the UART driver for TX/RX events.
 * Note: If using the existing console_callback, this function may not be called.
 * In that case, use polling mode for RX operations.
 */
void pi5_uart_callback(uart_callback_args_t *p_args)
{
    if (p_args->event == UART_EVENT_TX_COMPLETE) {
        uart_tx_complete = true;
    }
    else if (p_args->event == UART_EVENT_RX_COMPLETE) {
        uart_rx_ready = true;
    }
    else if (p_args->event == UART_EVENT_RX_CHAR) {
        // Single character received (useful for ready signal)
        uart_rx_ready = true;
    }
}

void pi5_uart_init(void)
{
    fsp_err_t err;

    printf("Initializing Pi5 UART communication...\n");
    printf("Using SCI4 (g_uart4) on P205/P206 via J8 Pmod connector\n");

    // Check if UART4 is already open (it might be used by console)
    // We'll try to open it - if it fails, it may already be open
    err = R_SCI_UART_Open(&g_uart4_ctrl, &g_uart4_cfg);
    if (FSP_SUCCESS == err) {
        printf("UART4 opened successfully\n");
    } else if (FSP_ERR_ALREADY_OPEN == err) {
        printf("UART4 already open (shared with console)\n");
        // This is OK - we can still use it
    } else {
        printf("WARNING: UART4 open returned: %d\n", err);
        printf("Continuing anyway - UART may still work\n");
    }

    uart_initialized = true;
    uart_tx_complete = true;
    uart_rx_ready = false;

    // Start receiving for Pi5 ready signal
    err = R_SCI_UART_Read(&g_uart4_ctrl, rx_buffer, 1);
    if (FSP_SUCCESS != err && FSP_ERR_IN_USE != err) {
        printf("Note: UART read setup returned: %d (may use polling)\n", err);
    }

    printf("Pi5 UART initialized @ 115200 baud\n");
    printf("Hardware connection via J8 Pmod:\n");
    printf("  Pin 3 (P205/TXD4) -> Pi5 RXD\n");
    printf("  Pin 4 (P206/RXD4) -> Pi5 TXD\n");
    printf("  Pin 5 (GND)       -> Pi5 GND\n");
    printf("\n");
    printf("IMPORTANT: Ensure config.ini has [Debug Print] Port=2\n");
    printf("to redirect debug output to USB-VCOM, freeing SCI4 for Pi5.\n");

    // ===== Send Hello World test message over UART =====
    // This helps verify the UART TX is working - check with serial terminal on Pi5
    const char *hello_msg = "\r\n=== RASynBoard Pi5 UART Ready ===\r\n"
                            "Hello from RASynBoard!\r\n"
                            "Waiting for Pi5 ready signal (0xA5)...\r\n\r\n";

    uart_tx_complete = false;
    err = R_SCI_UART_Write(&g_uart4_ctrl, (uint8_t*)hello_msg, strlen(hello_msg));
    if (FSP_SUCCESS == err) {
        // Wait for transmission with timeout
        uint32_t timeout = 2000;
        while (!uart_tx_complete && timeout > 0) {
            vTaskDelay(pdMS_TO_TICKS(1));
            timeout--;
        }
        if (timeout > 0) {
            printf(">>> Sent Hello World test message over Pi5 UART <<<\n");
        } else {
            printf("WARNING: Hello message TX timeout\n");
        }
    } else {
        printf("WARNING: Failed to send Hello message: %d\n", err);
    }
}

bool pi5_uart_check_ready(void)
{
    if (!uart_initialized) {
        return false;
    }

    // Check if we received data via callback
    if (uart_rx_ready) {
        uart_rx_ready = false;

        // Check if it's the ready signal
        if (rx_buffer[0] == PI5_CMD_READY) {
            printf("Pi5 ready signal received (0x%02X)\n", PI5_CMD_READY);

            // Send ACK
            uint8_t ack = PI5_CMD_ACK;
            uart_tx_complete = false;

            fsp_err_t err = R_SCI_UART_Write(&g_uart4_ctrl, &ack, 1);
            if (FSP_SUCCESS == err) {
                // Wait for transmission to complete (with timeout)
                uint32_t timeout = 1000;
                while (!uart_tx_complete && timeout > 0) {
                    vTaskDelay(pdMS_TO_TICKS(1));
                    timeout--;
                }
            }

            printf("Sent ACK to Pi5 (0x%02X)\n", PI5_CMD_ACK);

            // Restart receive for next communication
            R_SCI_UART_Read(&g_uart4_ctrl, rx_buffer, 1);

            return true;
        }

        // Restart receive for next byte
        R_SCI_UART_Read(&g_uart4_ctrl, rx_buffer, 1);
    }

    return false;
}

/**
 * @brief Wait for UART TX to complete with timeout
 */
static bool wait_for_tx_complete(uint32_t timeout_ms)
{
    uint32_t timeout = timeout_ms;
    while (!uart_tx_complete && timeout > 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        timeout--;
    }
    return uart_tx_complete;
}

/**
 * @brief Send data over UART with retry
 */
static fsp_err_t uart_send_data(const uint8_t *data, uint32_t length)
{
    uart_tx_complete = false;
    fsp_err_t err = R_SCI_UART_Write(&g_uart4_ctrl, data, length);

    if (FSP_SUCCESS == err) {
        if (!wait_for_tx_complete(5000)) {  // 5 second timeout
            printf("WARNING: UART TX timeout\n");
            return FSP_ERR_TIMEOUT;
        }
    }

    return err;
}

int pi5_uart_send_audio_buffer(circular_audio_buffer_t *buffer)
{
    uint8_t chunk[512];
    uint32_t total_sent = 0;
    uint32_t available;
    fsp_err_t err;

    if (!uart_initialized) {
        printf("ERROR: UART not initialized\n");
        return -1;
    }

    available = circular_buffer_available(buffer);

    printf("\n=== Starting audio transfer to Pi5 ===\n");
    printf("Total bytes to transfer: %lu\n", (unsigned long)available);

    // Send start command
    uint8_t cmd = PI5_CMD_DATA_START;
    err = uart_send_data(&cmd, 1);
    if (FSP_SUCCESS != err) {
        printf("ERROR: Failed to send DATA_START: %d\n", err);
        return -2;
    }
    printf("Sent DATA_START command (0x%02X)\n", PI5_CMD_DATA_START);

    // Send metadata header (ASCII format for easy debugging)
    char metadata[64];
    snprintf(metadata, sizeof(metadata), "AUDIO:%lu,%lu\n",
             (unsigned long)AUDIO_SAMPLE_RATE, (unsigned long)available);

    err = uart_send_data((uint8_t*)metadata, strlen(metadata));
    if (FSP_SUCCESS != err) {
        printf("ERROR: Failed to send metadata: %d\n", err);
        return -3;
    }
    printf("Sent metadata: %s", metadata);

    // Send audio data in 512-byte chunks
    printf("Transferring audio data");
    while (available > 0) {
        uint32_t to_send = (available < sizeof(chunk)) ? available : sizeof(chunk);
        uint32_t read = circular_buffer_read(buffer, chunk, to_send);

        if (read > 0) {
            err = uart_send_data(chunk, read);
            if (FSP_SUCCESS != err) {
                printf("\nERROR: Failed to send chunk: %d\n", err);
                return -4;
            }

            total_sent += read;
            available -= read;

            // Progress indicator every 10KB
            if (total_sent % 10240 == 0) {
                printf(".");
            }
        } else {
            break;  // No more data
        }
    }

    // Send end command
    cmd = PI5_CMD_DATA_END;
    err = uart_send_data(&cmd, 1);
    if (FSP_SUCCESS != err) {
        printf("\nERROR: Failed to send DATA_END: %d\n", err);
        return -5;
    }

    printf("\nSent DATA_END command (0x%02X)\n", PI5_CMD_DATA_END);
    printf("=== Transfer complete: %lu bytes ===\n\n", (unsigned long)total_sent);

    return 0;
}
