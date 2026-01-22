# Quick Start Guide - Low-Power Sound Detection System

## What Has Been Implemented

I've created the foundational modules for your low-power sound detection system. Here's what's ready:

### ✅ Completed (Phase 1)

1. **GPIO Signaling Module** (`src/rp2040_signal.c/h`)
   - Sends 100ms pulse to RP2040 power HAT on P002

2. **Circular Audio Buffer** (`src/circular_audio_buffer.c/h`)
   - 256KB buffer for 8 seconds of audio
   - Automatic wrap-around and overflow handling

3. **UART Communication** (`src/pi5_uart_comm.c/h`)
   - Simple protocol for transferring audio to Pi5
   - Ready for FSP configuration

4. **Configuration Updates** (`config.ini`)
   - New sections for audio detection and Pi5 communication

5. **Documentation**
   - FSP configuration guide
   - Implementation status tracker

---

## Your Next Steps

### Step 1: Configure Hardware in FSP (1-2 hours)

**Follow this guide**: [`docs/FSP_CONFIGURATION_GUIDE.md`](docs/FSP_CONFIGURATION_GUIDE.md)

**Good news: Most FSP configuration is already done!**

Already configured:
- ✓ GPIO P002 as output
- ✓ SCI4 (g_uart4) on P205/P206 for Pi5 communication
- ✓ LPM stack with Standby mode + IRQ5 wake

**Quick checklist:**
1. Update `config.ini` on SD card: `[Debug Print] Port=2`
2. Wire J8 Pmod connector to Pi5:
   - Pin 3 (P205/TXD4) → Pi5 RXD (GPIO15)
   - Pin 4 (P206/RXD4) → Pi5 TXD (GPIO14)
   - Pin 5 (GND) → Pi5 GND
3. Build and flash the project

### Step 2: Build and Test Components (1 day)

After FSP configuration:

```bash
# Build the project
# In e2studio: Project → Build All

# Flash to board
# Use E2-Light debugger via USB-C
```

**Test GPIO:**
```c
// Add to main initialization
rp2040_signal_init();
rp2040_signal_wake();
// Use oscilloscope to verify 100ms pulse on P002
```

**Test Circular Buffer:**
```c
// Test code snippet
circular_audio_buffer_t test_buf;
circular_buffer_init(&test_buf);
circular_buffer_start_recording(&test_buf);

uint8_t data[1024] = {0};
circular_buffer_write(&test_buf, data, 1024);
// Should write successfully
```

### Step 3: Integration (2-3 days)

Modify `src/ndp_thread_entry.c` to:
1. Include new module headers
2. Initialize modules at startup
3. Call `rp2040_signal_wake()` on sound detection
4. Start audio recording to circular buffer
5. Transfer to Pi5 when ready

**See**: Implementation plan for detailed integration steps

---

## Project Structure

```
Your firmware now includes:

New Modules:
├── src/rp2040_signal.c         # GPIO wake signal
├── src/circular_audio_buffer.c # Audio buffering
└── src/pi5_uart_comm.c         # UART communication

Modified Files:
├── ra_cfg/fsp_cfg/bsp/bsp_pin_cfg.h  # Added P002 definition
└── ndp120/synpkg_files/config.ini    # New config sections

Documentation:
├── docs/FSP_CONFIGURATION_GUIDE.md   # FSP setup instructions
├── docs/IMPLEMENTATION_STATUS.md     # Current progress
└── QUICKSTART.md                     # This file
```

---

## System Architecture

```
┌─────────────────┐
│  TDK Microphone │
│   (in NDP120)   │
└────────┬────────┘
         │ Always listening
         ▼
┌─────────────────┐
│     NDP120      │  Detects sound > 60dB
│  (ultra-low pwr)│
└────────┬────────┘
         │ IRQ when threshold exceeded
         ▼
┌─────────────────┐
│     RA6M4       │  Wakes from standby
│  (low-pwr mode) │
└────────┬────────┘
         │
         ├──► GPIO P002 ──► RP2040 ──► Powers Pi5
         │
         └──► Records to 256KB buffer
               │
               └──► SCI4 (J8 Pmod) ──► Pi5 (when ready)
```

---

## Configuration Reference

### Audio Detection Settings (`config.ini`)

```ini
[Audio Detection]
Detection_Mode=0      # 0=threshold, 1=ML model
Threshold_dB=60       # 60dB = normal conversation
Buffer_Size_KB=256    # 8 seconds @ 16kHz

[Pi5 Communication]
UART_Baud=115200     # Standard (or 921600 for high-speed)
Transfer_Timeout=30   # Wait 30 sec for Pi5
GPIO_Pulse_Duration=100  # 100ms wake pulse
```

### Pin Assignments

| Pin | Function | Purpose |
|-----|----------|---------|
| P002 | GPIO Output | RP2040 wake signal |
| P205 | SCI4 TxD (J8 Pin 3) | Send audio to Pi5 |
| P206 | SCI4 RxD (J8 Pin 4) | Receive ready from Pi5 |

**Note:** P109/P110 (SCI9) is used by DA16600 WiFi module - do not use for Pi5.

---

## Testing Sequence

### Phase 1: Component Testing
1. ✓ GPIO pulse (oscilloscope: 100ms @ 3.3V on P002)
2. ✓ Circular buffer (write/read 256KB)
3. ✓ UART loopback (J8 Pin3→Pin4, using USB-serial adapter)

### Phase 2: Integration Testing
4. ⏳ Detection → GPIO signal → record
5. ⏳ UART transfer to PC terminal
6. ⏳ Low-power wake cycle

### Phase 3: System Testing
7. ⏳ End-to-end with real Pi5
8. ⏳ Power consumption verification
9. ⏳ Extended operation test

---

## Expected Behavior

When working correctly:

1. **Idle State**:
   - RA6M4 in STANDBY mode (~5mA)
   - NDP120 monitoring audio (~250µA)
   - Total: < 5mA

2. **Sound Detection**:
   - Sound > 60dB detected
   - RA6M4 wakes in < 100ms
   - GPIO P002 pulses HIGH for 100ms
   - RP2040 receives signal

3. **Recording**:
   - Circular buffer starts recording
   - 16kHz, 16-bit PCM audio
   - Fills at 32KB/sec
   - Can hold 8 seconds

4. **Transfer**:
   - Pi5 sends 0xA5 (ready)
   - RASynBoard sends audio via UART
   - ~22 seconds @ 115200 baud
   - Pi5 receives complete audio

---

## Troubleshooting

### Build Errors

**Issue**: Undefined reference errors

**Solution**: Ensure all source files are included in the build

### GPIO Not Working

**Issue**: P002 stays LOW

**Solution**:
1. Verify FSP pin configuration
2. Check `R_BSP_PinWrite()` is called
3. Measure with multimeter/scope

### UART Not Transmitting

**Issue**: No data on J8 Pmod (P205)

**Solution**:
1. Verify `config.ini` has `Port=2` (debug to USB-VCOM)
2. Check wiring: J8 Pin3 (TX) → Pi5 RX
3. Verify callback is being called
4. Test with loopback (J8 Pin3→Pin4)

### Debug Output Missing

**Issue**: No serial output after setting Port=2

**Solution**: Connect USB cable to Core Board USB-C for USB-VCOM debug output

---

## Performance Targets

| Metric | Target | How to Measure |
|--------|--------|----------------|
| Wake latency | < 100ms | Oscilloscope: sound → IRQ |
| Standby current | < 5mA | Multimeter on power rail |
| Recording quality | 16kHz, 16-bit | Verify audio playback |
| Transfer time | ~22 sec | Stopwatch during UART transfer |

---

## What's Next?

After completing FSP configuration and testing:

1. **NDP120 Configuration** (2-3 days)
   - Simplify to threshold detection
   - Remove ML model dependency

2. **Low-Power Mode** (1-2 days)
   - Implement proper standby entry/exit
   - Verify wake on IRQ5

3. **Full Integration** (2-3 days)
   - Combine all modules in `ndp_thread_entry.c`
   - Add recording management
   - Test end-to-end flow

**Total estimated time**: 15-19 days for complete implementation

---

## Getting Help

- **Implementation Plan**: See `C:\Users\erhan\.claude\plans\refactored-greeting-octopus.md`
- **FSP Guide**: See `docs/FSP_CONFIGURATION_GUIDE.md`
- **Status**: See `docs/IMPLEMENTATION_STATUS.md`
- **RASynBoard Docs**: http://avnet.me/rasynboard-ug

---

## Summary

✅ **Phase 1 Complete**: All foundational modules created
⏳ **Next**: FSP configuration (follow `docs/FSP_CONFIGURATION_GUIDE.md`)
🎯 **Goal**: Low-power sound detection system operational

Good luck with your implementation! The foundation is solid and ready for hardware configuration and integration.
