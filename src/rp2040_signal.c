/*
 * rp2040_signal.c
 *
 * GPIO signaling module to wake the RP2040 power management HAT
 * which in turn powers on the Raspberry Pi 5
 *
 * Created for: Low-Power Sound Detection System
 */

#include "rp2040_signal.h"
#include "bsp_pin_cfg.h"
#include <stdio.h>

// GPIO pin for RP2040 wake signal
// P002 (Port 0, Pin 2) - available and unused in default configuration
#define RP2040_WAKE_PIN  BSP_IO_PORT_00_PIN_02

void rp2040_signal_init(void)
{
    // Pin is configured via FSP configurator as GPIO output
    // Set initial state to LOW
    R_BSP_PinWrite(RP2040_WAKE_PIN, BSP_IO_LEVEL_LOW);

    printf("RP2040 signal initialized on P002\n");
}

void rp2040_signal_wake(void)
{
    printf("Signaling RP2040 to wake Pi5...\n");

    // Send 100ms HIGH pulse
    // RP2040 will detect the rising edge and power on the Pi5
    R_BSP_PinWrite(RP2040_WAKE_PIN, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    R_BSP_PinWrite(RP2040_WAKE_PIN, BSP_IO_LEVEL_LOW);

    printf("RP2040 wake signal sent (100ms pulse)\n");
}
