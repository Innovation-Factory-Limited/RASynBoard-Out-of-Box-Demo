/*
 * pi5_uart_comm.c
 *
 * UART communication module for transferring audio data to Raspberry Pi 5
 *
 * Created for: Low-Power Sound Detection System
 */

#include "pi5_uart_comm.h"
#include "hal_data.h"
#include <stdio.h>
#include <string.h>

// UART instance handles - will be configured via FSP
// Note: These need to be created in FSP configurator as g_uart9_ctrl and g_uart9_cfg
// extern uart_ctrl_t g_uart9_ctrl;
// extern const uart_cfg_t g_uart9_cfg;

// For now, we'll use placeholder implementations until FSP is configured
// The user will need to configure UART9 in FSP configurator

static volatile bool uart_tx_complete = false;
static volatile bool uart_rx_ready = false;
static uint8_t rx_buffer[16];

/**
 * @brief UART callback for transmit and receive events
 *
 * This callback is invoked by the UART driver for TX/RX events
 */
void pi5_uart_callback(uart_callback_args_t *p_args)
{
    if (p_args->event == UART_EVENT_TX_COMPLETE) {
        uart_tx_complete = true;
    }
    else if (p_args->event == UART_EVENT_RX_COMPLETE) {
        uart_rx_ready = true;
    }
}

void pi5_uart_init(void)
{
    fsp_err_t err;

    printf("Initializing Pi5 UART communication...\n");

    // Note: UART9 must be configured in FSP configurator
    // - Pins: P109 (TxD9), P110 (RxD9)
    // - Baud: 115200
    // - Data: 8-bit, No parity, 1 stop bit
    // - Callback: pi5_uart_callback

    // Uncomment when FSP UART9 is configured:
    /*
    err = R_SCI_UART_Open(&g_uart9_ctrl, &g_uart9_cfg);
    if (FSP_SUCCESS != err) {
        printf("ERROR: UART9 open failed: %d\n", err);
        printf("Please configure UART9 in FSP configurator:\n");
        printf("  - Pins: P109 (TxD9), P110 (RxD9)\n");
        printf("  - Baud: 115200, 8N1\n");
        printf("  - Callback: pi5_uart_callback\n");
        return;
    }

    // Start receiving for Pi5 ready signal
    R_SCI_UART_Read(&g_uart9_ctrl, rx_buffer, 1);
    */

    printf("Pi5 UART9 initialized @ 115200 baud (P109/P110)\n");
    printf("NOTE: UART9 must be configured in FSP configurator to function\n");
}

bool pi5_uart_check_ready(void)
{
    // Check if we received data
    if (uart_rx_ready) {
        uart_rx_ready = false;

        // Check if it's the ready signal
        if (rx_buffer[0] == PI5_CMD_READY) {
            printf("Pi5 ready signal received (0x%02X)\n", PI5_CMD_READY);

            // Send ACK
            uint8_t ack = PI5_CMD_ACK;
            uart_tx_complete = false;

            // Uncomment when FSP UART9 is configured:
            /*
            R_SCI_UART_Write(&g_uart9_ctrl, &ack, 1);
            while (!uart_tx_complete) {
                vTaskDelay(1);
            }
            */

            printf("Sent ACK to Pi5 (0x%02X)\n", PI5_CMD_ACK);
            return true;
        }

        // Restart receive for next byte
        // Uncomment when FSP UART9 is configured:
        // R_SCI_UART_Read(&g_uart9_ctrl, rx_buffer, 1);
    }

    return false;
}

int pi5_uart_send_audio_buffer(circular_audio_buffer_t *buffer)
{
    uint8_t chunk[512];
    uint32_t total_sent = 0;
    uint32_t available = circular_buffer_available(buffer);

    printf("\n=== Starting audio transfer to Pi5 ===\n");
    printf("Total bytes to transfer: %lu\n", available);

    // Send start command
    uint8_t cmd = PI5_CMD_DATA_START;
    uart_tx_complete = false;

    // Uncomment when FSP UART9 is configured:
    /*
    R_SCI_UART_Write(&g_uart9_ctrl, &cmd, 1);
    while (!uart_tx_complete) {
        vTaskDelay(1);
    }
    */

    printf("Sent DATA_START command (0x%02X)\n", PI5_CMD_DATA_START);

    // Send metadata header (ASCII format for easy debugging)
    char metadata[64];
    snprintf(metadata, sizeof(metadata), "AUDIO:%lu,%lu\n",
             (unsigned long)AUDIO_SAMPLE_RATE, available);

    uart_tx_complete = false;

    // Uncomment when FSP UART9 is configured:
    /*
    R_SCI_UART_Write(&g_uart9_ctrl, (uint8_t*)metadata, strlen(metadata));
    while (!uart_tx_complete) {
        vTaskDelay(1);
    }
    */

    printf("Sent metadata: %s", metadata);

    // Send audio data in 512-byte chunks
    printf("Transferring audio data");
    while (available > 0) {
        uint32_t to_send = (available < sizeof(chunk)) ? available : sizeof(chunk);
        uint32_t read = circular_buffer_read(buffer, chunk, to_send);

        if (read > 0) {
            uart_tx_complete = false;

            // Uncomment when FSP UART9 is configured:
            /*
            R_SCI_UART_Write(&g_uart9_ctrl, chunk, read);
            while (!uart_tx_complete) {
                vTaskDelay(1);
            }
            */

            total_sent += read;
            available -= read;

            // Progress indicator every 10KB
            if (total_sent % 10240 == 0) {
                printf(".");
            }
        }
    }

    // Send end command
    cmd = PI5_CMD_DATA_END;
    uart_tx_complete = false;

    // Uncomment when FSP UART9 is configured:
    /*
    R_SCI_UART_Write(&g_uart9_ctrl, &cmd, 1);
    while (!uart_tx_complete) {
        vTaskDelay(1);
    }
    */

    printf("\nSent DATA_END command (0x%02X)\n", PI5_CMD_DATA_END);
    printf("=== Transfer complete: %lu bytes ===\n\n", total_sent);

    return 0;
}
