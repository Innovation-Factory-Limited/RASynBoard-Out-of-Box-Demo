/*
 * pi5_uart_comm.h
 *
 * UART communication module for transferring audio data to Raspberry Pi 5
 * Uses SCI4 (P205/P206) at 115200 baud via J8 Pmod connector
 *
 * Hardware Connection (J8 Pmod on IO Board):
 *   Pin 3 (P205/TXD4) -> Pi5 RXD
 *   Pin 4 (P206/RXD4) -> Pi5 TXD
 *   Pin 5 (GND)       -> Pi5 GND
 *
 * Note: Debug console must be moved to USB-VCOM (config.ini: Port=2)
 *       to free SCI4 for Pi5 communication.
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
#include "hal_data.h"              // FSP generated header - includes uart_callback_args_t
#include "circular_audio_buffer.h"

// Protocol command definitions
#define PI5_CMD_READY       0xA5  // Pi5 → RASynBoard: I'm ready to receive
#define PI5_CMD_ACK         0x5A  // RASynBoard → Pi5: Acknowledged
#define PI5_CMD_DATA_START  0xD0  // Start of audio data transfer
#define PI5_CMD_DATA_END    0xD1  // End of audio data transfer

/**
 * @brief Initialize SCI4 UART for Pi5 communication
 *
 * Uses existing g_uart4 (SCI4) on P205/P206 at 115200 baud, 8N1
 * Accessible via J8 Pmod connector pins 3 (TX) and 4 (RX)
 *
 * IMPORTANT: Set config.ini [Debug Print] Port=2 to use USB-VCOM for debug,
 * freeing SCI4 for Pi5 communication.
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

/**
 * @brief UART callback for Pi5 communication events
 *
 * Handles TX complete and RX complete events.
 * Note: This may need to be set in FSP configurator if not using
 * the existing console_callback.
 */
void pi5_uart_callback(uart_callback_args_t *p_args);

#endif /* PI5_UART_COMM_H_ */
