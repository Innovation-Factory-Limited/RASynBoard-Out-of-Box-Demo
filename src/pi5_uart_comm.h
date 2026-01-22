/*
 * pi5_uart_comm.h
 *
 * UART communication module for transferring audio data to Raspberry Pi 5
 * Uses UART9 (P109/P110) at 115200 baud
 *
 * Protocol:
 * - Pi5 sends 0xA5 (PI5_CMD_READY) when ready to receive
 * - RASynBoard sends 0x5A (PI5_CMD_ACK) to acknowledge
 * - RASynBoard sends 0xD0 (PI5_CMD_DATA_START) to start transfer
 * - RASynBoard sends metadata header (ASCII)
 * - RASynBoard sends audio data in 512-byte chunks
 * - RASynBoard sends 0xD1 (PI5_CMD_DATA_END) to end transfer
 *
 * Created for: Low-Power Sound Detection System
 */

#ifndef PI5_UART_COMM_H_
#define PI5_UART_COMM_H_

#include <stdint.h>
#include <stdbool.h>
#include "circular_audio_buffer.h"

// Protocol command definitions
#define PI5_CMD_READY       0xA5  // Pi5 → RASynBoard: I'm ready to receive
#define PI5_CMD_ACK         0x5A  // RASynBoard → Pi5: Acknowledged
#define PI5_CMD_DATA_START  0xD0  // Start of audio data transfer
#define PI5_CMD_DATA_END    0xD1  // End of audio data transfer

/**
 * @brief Initialize UART9 for Pi5 communication
 *
 * Configures UART9 (P109/P110) at 115200 baud, 8N1
 * Starts receiving for Pi5 ready signal
 */
void pi5_uart_init(void);

/**
 * @brief Check if Pi5 has signaled ready
 *
 * @return true if Pi5 sent ready signal (0xA5), false otherwise
 */
bool pi5_uart_check_ready(void);

/**
 * @brief Send audio buffer to Pi5
 *
 * Sends complete audio buffer contents to Pi5 via UART.
 * Protocol: START → METADATA → AUDIO_DATA → END
 *
 * @param buffer Pointer to circular audio buffer
 * @return 0 on success, negative error code on failure
 */
int pi5_uart_send_audio_buffer(circular_audio_buffer_t *buffer);

#endif /* PI5_UART_COMM_H_ */
