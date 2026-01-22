/*
 * rp2040_signal.h
 *
 * GPIO signaling module to wake the RP2040 power management HAT
 * which in turn powers on the Raspberry Pi 5
 *
 * Created for: Low-Power Sound Detection System
 */

#ifndef RP2040_SIGNAL_H_
#define RP2040_SIGNAL_H_

#include "bsp_api.h"

/**
 * @brief Initialize RP2040 signal GPIO
 *
 * Configures GPIO P002 as output with initial LOW state.
 * This function should be called once during system initialization.
 */
void rp2040_signal_init(void);

/**
 * @brief Send wake signal to RP2040
 *
 * Sends a 100ms HIGH pulse on GPIO P002 to wake the RP2040
 * power management HAT. The RP2040 will detect the rising edge
 * and power on the Raspberry Pi 5.
 */
void rp2040_signal_wake(void);

#endif /* RP2040_SIGNAL_H_ */
