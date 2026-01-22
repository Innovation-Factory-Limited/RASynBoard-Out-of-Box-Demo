# FSP Configuration Guide for Low-Power Sound Detection System

This guide provides step-by-step instructions for configuring the Renesas FSP (Flexible Software Package) for the low-power sound detection system.

## Overview

The following hardware resources need to be configured:
1. **GPIO P002** - Output signal to RP2040 power management HAT
2. **UART9 (SCI9)** - Communication with Raspberry Pi 5
3. **LPM (Low Power Mode)** - Standby mode with IRQ wake capability

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

## Part 2: Configure UART9 for Pi5 Communication

### Step 1: Add UART9 Stack

1. In the FSP Configuration perspective, click on the **Stacks** tab
2. Click **New Stack** button
3. Select **Connectivity** → **UART (r_sci_uart)**
4. Click **Add**

### Step 2: Configure UART9 Instance

1. Select the newly added UART stack
2. In the **Properties** panel, configure:

#### Common Settings
- **Name**: `g_uart9`
- **Channel**: `9` (SCI9)
- **Callback**: `pi5_uart_callback`
- **Transmit/Receive Interrupt Priority**: `Priority 3` (or appropriate)

#### Communication Settings
- **Baud Rate**: `115200`
- **Data Bits**: `8 bits`
- **Parity**: `No Parity`
- **Stop Bits**: `1 bit`
- **Flow Control**: `None`

### Step 3: Configure UART9 Pins

1. Return to the **Pins** tab
2. Locate **P109** and configure:
   - **Symbolic Name**: `UART9_TXD` (or leave as default TxD9)
   - **Mode**: `Peripheral mode (TxD9)`
   - **Function**: `TxD9_SCK9_MISO9_SCL9`

3. Locate **P110** and configure:
   - **Symbolic Name**: `UART9_RXD` (or leave as default RxD9)
   - **Mode**: `Peripheral mode (RxD9)`
   - **Function**: `RxD9_MISO9_SDA9`

### Step 4: Verify UART Configuration

After configuration, you should see in `ra_gen/hal_data.h`:

```c
extern const uart_instance_t g_uart9;
extern uart_ctrl_t g_uart9_ctrl;
extern const uart_cfg_t g_uart9_cfg;
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

## Part 5: Uncomment UART Code in pi5_uart_comm.c

After FSP configuration is complete, you need to enable the UART code:

### Step 1: Open pi5_uart_comm.c

Located in: `src/pi5_uart_comm.c`

### Step 2: Uncomment UART Code

Find and uncomment the following sections:

#### In `pi5_uart_init()`:
```c
// Uncomment this block:
err = R_SCI_UART_Open(&g_uart9_ctrl, &g_uart9_cfg);
if (FSP_SUCCESS != err) {
    printf("ERROR: UART9 open failed: %d\n", err);
    return;
}

// Start receiving for Pi5 ready signal
R_SCI_UART_Read(&g_uart9_ctrl, rx_buffer, 1);
```

#### In `pi5_uart_check_ready()`:
```c
// Uncomment this block:
R_SCI_UART_Write(&g_uart9_ctrl, &ack, 1);
while (!uart_tx_complete) {
    vTaskDelay(1);
}

// And:
R_SCI_UART_Read(&g_uart9_ctrl, rx_buffer, 1);
```

#### In `pi5_uart_send_audio_buffer()`:
Uncomment all `R_SCI_UART_Write()` calls (there are multiple instances)

---

## Part 6: Build and Verify

### Step 1: Build Project

1. Click **Project** → **Build All** (or Ctrl+B)
2. Verify no compilation errors
3. Check for any warnings related to GPIO or UART

### Step 2: Verify Pin Assignments

Use the **Pin List** view in FSP to verify:

| Pin | Function | Symbolic Name | Direction |
|-----|----------|--------------|-----------|
| P002 | GPIO | RP2040_SIGNAL | Output (Low) |
| P109 | TxD9 | UART9_TXD | Output |
| P110 | RxD9 | UART9_RXD | Input |

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

### Issue: UART9 not available in channel selection

**Solution**: Verify that SCI9 is not already allocated to another peripheral. Check the **Resource Usage** tab in FSP.

### Issue: Pin configuration conflicts

**Solution**: Ensure P002, P109, P110 are not already assigned. Remove any conflicting assignments in the Pins tab.

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

- [ ] P002 configured as GPIO output (RP2040_SIGNAL)
- [ ] UART9 configured with P109/P110, 115200 baud
- [ ] LPM configured with Standby mode
- [ ] IRQ5 enabled as wake source
- [ ] FSP code generated successfully
- [ ] Project builds without errors
- [ ] UART code uncommented in pi5_uart_comm.c
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
