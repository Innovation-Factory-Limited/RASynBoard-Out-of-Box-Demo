# FSP Configuration Guide for Low-Power Sound Detection System

This guide provides step-by-step instructions for configuring the Renesas FSP (Flexible Software Package) for the low-power sound detection system.

## Overview

The following hardware resources need to be configured:
1. **GPIO P002** - Output signal to RP2040 power management HAT
2. **SCI4 (UART4)** - Communication with Raspberry Pi 5 (already configured in FSP)
3. **LPM (Low Power Mode)** - Standby mode with IRQ wake capability

**Note:** SCI4 (P205/P206) is already configured as `g_uart4` in FSP for debug console.
We repurpose it for Pi5 communication by redirecting debug to USB-VCOM.

## Prerequisites

- Renesas e2studio IDE installed
- FSP configurator available in e2studio
- RASynBoard project open in e2studio

---

## Part 1: Configure GPIO P002 for RP2040 Signaling

### Step 1: Open Pin Configuration

1. In e2studio, navigate to the **Project Explorer**
2. Double-click `configuration.xml` in the project root
3. Wait for the FSP Configuration perspective to open
4. Click on the **Pins** tab at the bottom

### Step 2: Configure P002

1. In the pin diagram or pin list, locate **P002** (Port 0, Pin 2)
2. Set the following properties:
   - **Symbolic Name**: `RP2040_SIGNAL`
   - **Comment**: `RP2040 wake signal`
   - **Mode**: `Output mode (Initial Low)`
   - **Direction**: `Output`
   - **Initial state**: `Low`

3. The pin configuration should look like this:
   ```
   Pin: P002
   Symbolic Name: RP2040_SIGNAL
   Mode: Output mode (Initial Low)
   ```

### Step 3: Verify Pin Definition

The FSP configurator will automatically add the following to `ra_cfg/fsp_cfg/bsp/bsp_pin_cfg.h`:

```c
#define RP2040_SIGNAL (BSP_IO_PORT_00_PIN_02)
```

**Note**: This should already be added manually in the source code, but FSP will regenerate it.

---

## Part 2: Pi5 Communication via SCI4 (Already Configured)

**IMPORTANT:** SCI4 is already configured in FSP as `g_uart4`. No additional FSP configuration is needed!

### Existing Configuration

SCI4 is pre-configured for debug console with the following settings:
- **Name**: `g_uart4`
- **Channel**: `4` (SCI4)
- **Pins**: P205 (TXD4), P206 (RXD4)
- **Baud Rate**: `115200`
- **Callback**: `console_callback`

### Repurposing SCI4 for Pi5 Communication

To use SCI4 for Pi5 communication instead of debug console:

1. **Redirect debug output to USB-VCOM**:
   Edit `config.ini` on the SD card:
   ```ini
   [Debug Print]
   Port=2    # 1=UART, 2=USB-VCOM
   ```

2. **Hardware Connection via J8 Pmod Connector**:
   | J8 Pin | Signal | Connect To |
   |--------|--------|------------|
   | Pin 3 | P205 (TXD4) | Pi5 RXD (GPIO15) |
   | Pin 4 | P206 (RXD4) | Pi5 TXD (GPIO14) |
   | Pin 5 | GND | Pi5 GND |

### Why Not UART9 (SCI9)?

SCI9 (P109/P110) is already used by the DA16600 WiFi module for AT commands.
The existing `g_uart3` instance uses channel 9 with `rm_atcmd_uart_callback`.

### Verify Existing UART4 Configuration

The existing configuration in `ra_gen/hal_data.h` shows:

```c
extern const uart_instance_t g_uart4;
extern sci_uart_instance_ctrl_t g_uart4_ctrl;
extern const uart_cfg_t g_uart4_cfg;
```

---

## Part 3: Configure Low Power Mode (LPM)

### Step 1: Add LPM Stack

1. In the **Stacks** tab, click **New Stack**
2. Select **System** → **Low Power Modes (r_lpm)**
3. Click **Add**

### Step 2: Configure LPM Settings

1. Select the LPM stack
2. Configure the following properties:

#### General Settings
- **Name**: `g_lpm0`
- **Low Power Mode**: `Standby mode`
- **Snooze Request Source**: `None` (not using snooze)

#### Wake Source Configuration
- **IRQ5**: ✓ **Enabled** (NDP120 interrupt)
- **IRQ13**: ✓ **Enabled** (optional, for user button wake)

#### Output Port Settings
- **Output port enable**: `Disabled` (to reduce power consumption)

### Step 3: Verify Wake Sources

The NDP120 interrupt should already be configured on **IRQ05**. Verify this in the **Interrupts** tab:

1. Look for **ICU IRQ5**
2. Verify it's connected to the NDP120 interrupt pin (should already be configured)

---

## Part 4: Generate FSP Code

### Step 1: Save Configuration

1. Save the `configuration.xml` file (Ctrl+S)
2. Review any warnings or errors in the **Problems** tab

### Step 2: Generate Code

1. Right-click on the project in Project Explorer
2. Select **Generate Project Content**
3. Wait for code generation to complete

### Step 3: Verify Generated Files

Check that the following files have been updated:

1. **`ra_gen/pin_data.c`** - Should include P002, P109, P110 configurations
2. **`ra_gen/hal_data.c`** - Should include UART9 and LPM configurations
3. **`ra_gen/hal_data.h`** - Should include extern declarations

---

## Part 5: UART Code Status

The Pi5 UART communication code in `src/pi5_uart_comm.c` is now **fully enabled** and uses the existing `g_uart4` (SCI4) instance.

### No Code Changes Required

The UART code has been updated to:
- Use `g_uart4_ctrl` and `g_uart4_cfg` (existing FSP configuration)
- Handle the case where UART4 may already be open (shared with console)
- Include proper timeout handling for transmit operations

### Key Functions

- `pi5_uart_init()` - Opens/reuses UART4, starts receiving
- `pi5_uart_check_ready()` - Checks for Pi5 ready signal (0xA5)
- `pi5_uart_send_audio_buffer()` - Sends audio data to Pi5

### Configuration Requirement

**Important:** Update `config.ini` on the SD card:
```ini
[Debug Print]
Port=2    # Redirect debug to USB-VCOM, freeing SCI4 for Pi5
```

---

## Part 6: Build and Verify

### Step 1: Build Project

1. Click **Project** → **Build All** (or Ctrl+B)
2. Verify no compilation errors
3. Check for any warnings related to GPIO or UART

### Step 2: Verify Pin Assignments

Use the **Pin List** view in FSP to verify:

| Pin | Function | Usage | Direction |
|-----|----------|-------|-----------|
| P002 | GPIO | RP2040_SIGNAL | Output (Low) |
| P205 | TxD4 | Pi5 Communication (via J8) | Output |
| P206 | RxD4 | Pi5 Communication (via J8) | Input |

**Note:** P109/P110 (SCI9) are used by DA16600 WiFi - do not modify.

### Step 3: Test GPIO Output

After flashing the firmware, you can test the GPIO with an oscilloscope or multimeter:

```c
// Test code (add to initialization)
rp2040_signal_init();
rp2040_signal_wake();  // Should generate 100ms pulse on P002
```

Expected: 100ms HIGH pulse (3.3V) on P002

---

## Troubleshooting

### Issue: No data on Pi5 UART

**Solutions**:
1. Verify `config.ini` has `Port=2` to free SCI4 from debug console
2. Check wiring: J8 Pin3 (TX) → Pi5 RX, J8 Pin4 (RX) → Pi5 TX
3. Verify Pi5 UART is configured for 115200 baud, 8N1
4. Check ground connection between boards

### Issue: Debug output missing after changing Port=2

**Solution**: Connect USB cable to Core Board USB-C connector for USB-VCOM debug output.

### Issue: Pin configuration conflicts

**Solution**: P002 should be configured as GPIO output. P205/P206 are already configured for SCI4.

### Issue: Compilation errors after code generation

**Solution**:
- Clean the project (Project → Clean)
- Rebuild (Project → Build All)
- Verify all FSP-generated files are included in the build

### Issue: Low Power Mode not waking on NDP120 interrupt

**Solution**:
- Verify IRQ5 is enabled as wake source in LPM configuration
- Check that NDP120 interrupt is properly connected to IRQ5
- Test wake functionality with a simpler interrupt source first

---

## Additional Configuration Notes

### Power Consumption Optimization

For minimum power consumption in standby mode:

1. In LPM configuration, ensure:
   - **Output port enable**: Disabled
   - **Deep Software Standby**: Enabled (if not using wake on IRQ)

2. Consider disabling unused peripherals in standby

### UART Baud Rate Adjustment

To increase transfer speed (optional):

1. In UART9 configuration, change **Baud Rate** to `921600`
2. Update `config.ini`: `[Pi5 Communication]->UART_Baud=921600`
3. Ensure Pi5 UART is also configured for 921600 baud

---

## Summary Checklist

Before proceeding with integration:

- [x] P002 configured as GPIO output (RP2040_SIGNAL) - **Already done**
- [x] SCI4 (g_uart4) available on P205/P206 - **Already configured in FSP**
- [x] LPM configured with Standby mode - **Already configured (g_lpm0, g_lpm1)**
- [x] IRQ5 enabled as wake source - **Already configured**
- [ ] config.ini updated with `Port=2` for USB-VCOM debug
- [ ] Project builds without errors
- [ ] Hardware wiring: J8 Pin3→Pi5 RX, J8 Pin4→Pi5 TX, J8 Pin5→GND
- [ ] GPIO test successful (optional)

---

## Next Steps

After completing FSP configuration:

1. Proceed to **Phase 5: Integration** in the main implementation plan
2. Modify `src/ndp_thread_entry.c` to integrate all modules
3. Test the complete detection → signal → record → transfer flow

For questions or issues, refer to:
- [Renesas FSP Documentation](https://www.renesas.com/us/en/software-tool/flexible-software-package-fsp)
- [RASynBoard Development Guide](http://avnet.me/rasynboard-ug)
- Main implementation plan: `C:\Users\erhan\.claude\plans\refactored-greeting-octopus.md`
