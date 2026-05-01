# T32 Key Discovery Firmware

Single-purpose PlatformIO firmware that discovers the empirical wiring
permutation between matrix key index (0..31, top-left → bottom-right) and
electrical mux channel (`mux*16 + ch`) on a T32 board. Flash it, follow the
LED prompts, and copy the printed `k3dot0Keys[32]` array into
`src/variant_t32.hpp`.

This tool is fully standalone — it does NOT modify the repo, share a build
environment with the main firmware, or pull in TinyUSB/BLE-MIDI/MIDI/JSON
dependencies. FastLED is the only library.

## Build & flash

```sh
cd tools/key_discovery
pio run -t upload
```

The board flashes the same `unwn_s3` board definition the main firmware uses
(ESP32-S3). Make sure no other PlatformIO build is uploading to the same
board at the same time.

## Monitor

```sh
pio device monitor
```

Baud is 115200 with the `esp32_exception_decoder` filter. After upload the
firmware prints a banner, captures a baseline ADC scan (~500 ms), then enters
the per-key discovery flow.

## Operator workflow

1. After upload, **release MODE** (the boot button on `unwn_s3`). If MODE was
   held at boot, the firmware logs `MODE held at boot — restarting` and calls
   `ESP.restart()` so the bootloader does not stay armed.
2. The firmware lights LED 0 red. Press the **physical key directly under
   that LED**.
3. The pressed key's mux channel is recorded. The LED turns green and Serial
   prints a line like `key 0 -> mux=0 ch=3 (electrical=3)`. Release the key.
4. The firmware advances to LED 1 (red) and the cycle repeats for all 32
   keys.
5. **Made a mistake?** Press MODE during a key-wait to re-arm that index —
   the current LED stays red and the firmware re-listens. (No need to
   restart the whole flow.)
6. After all 32 keys are mapped the firmware prints a copy-paste-ready block:

   ```cpp
   inline constexpr uint8_t k3dot0Keys[32] = {
       v0, v1, v2, v3, v4, v5, v6, v7,
       v8, v9, v10, v11, v12, v13, v14, v15,
       v16, v17, v18, v19, v20, v21, v22, v23,
       v24, v25, v26, v27, v28, v29, v30, v31,
   };
   ```

   All 32 LEDs then blink slow green forever to indicate completion.

## Pasting the result

Copy the printed `inline constexpr uint8_t k3dot0Keys[32] = { ... };` block
from Serial and paste it over the existing `k3dot0Keys[32]` array inside
`namespace detail` in `src/variant_t32.hpp`.

## Caveats

- `src/variant_t32.hpp` contains `static_assert` blocks that validate the
  `k3dot0Keys[]` permutation against each `kMuxes[].keyMapping` array at
  compile time. If the new `k3dot0Keys[]` does not match the existing
  `keyMapping` arrays, **those arrays must be updated together** with the
  new permutation — the build will refuse to link otherwise. Treat this as
  a feature: it forces the two encodings to stay consistent.
- `pinout::t32::COM2 = GPIO17` is a placeholder pending T32 schematic
  verification (Plan 12.03.5.A). It maps to ESP32-S3 ADC2_CH6. ADC2 conflicts
  with WiFi on ESP32 classic, but on ESP32-S3 ADC2 is usable when WiFi is
  off — and this firmware never starts WiFi, so the conflict is irrelevant
  here. Confirm GPIO17 against the T32 schematic before trusting Mux 1
  readings.
- The `MODE` button on `unwn_s3` is GPIO0, the same line the bootloader uses
  for download mode. Holding it at boot puts the chip into download mode
  *before* this firmware runs; once running, our firmware reads it as an
  active-low input with internal pull-up.
