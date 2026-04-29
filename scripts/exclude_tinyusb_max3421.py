"""
Phase 10.1 hotfix — Path E

Two pre-build adjustments to installed lib_deps trees in `.pio/libdeps/<env>/`:

(1) Adafruit TinyUSB Library — write a `library.json` that excludes the entire
    tinyusb-core (`class/`, `common/`, `device/`, `host/`, `osal/`,
    `portable/`, `tusb.c`) from the lib build. Arduino-ESP32 already ships a
    prebuilt `libarduino_tinyusb.a` that defines all of those symbols;
    compiling them again from the Adafruit lib produces wholesale
    duplicate-symbol link errors (the v1.0 BLOCKER reported only the
    `hcd_max3421_*` subset, but the actual collision spans device + host +
    class + portable). Only the Adafruit Arduino-glue layer under `arduino/`
    is compiled.

(2) BLE-MIDI — patch `src/hardware/BLEMIDI_ESP32_NimBLE.h` to mark the
    out-of-class `begin(...)` definition as `inline`. The library defines
    that body in a header without `inline`, so any TU including the header
    emits a strong definition and multi-TU linkage fails with an ODR
    violation. The fix on upstream master changes the API (templates the
    class), which would require src/ source changes; pinning to that fix
    commit is therefore not viable. A pre-build `inline` injection on the
    installed copy is purely a build-tree adjustment — it does not modify
    files under src/, does not fork upstream, and does not lib_ignore.

Both adjustments are idempotent — safe to run on every build.

See: .planning/phases/10.1-lib-deps-conflict-hotfix/10.1-01-PLAN.md (Task 2,
Path E) and 10.1-01-BLOCKER.md for why Paths A/B/C did not work.
"""
import json
import os

Import("env")  # noqa: F821  (provided by SCons / PlatformIO)

LIB_NAME = "Adafruit TinyUSB Library"
BLE_MIDI_LIB = "BLE-MIDI"
BLE_MIDI_HEADER = "src/hardware/BLEMIDI_ESP32_NimBLE.h"
BLE_MIDI_NEEDLE = "bool BLEMIDI_ESP32_NimBLE::begin("
BLE_MIDI_REPLACEMENT = "inline bool BLEMIDI_ESP32_NimBLE::begin("
EXPECTED = {
    "name": "Adafruit TinyUSB Library",
    "version": "3.3.0",
    "build": {
        # On ESP32, the Arduino-ESP32 framework already ships a prebuilt
        # libarduino_tinyusb.a containing the entire TinyUSB core (device + host
        # + class + portable + osal). The Adafruit TinyUSB library is only
        # needed for its Arduino-glue layer under `arduino/`; compiling its
        # bundled tinyusb sources causes wholesale multi-definition link errors
        # against the prebuilt lib (see 10.1-CONTEXT.md D10.1.2 — the v1.0
        # blocker described `hcd_max3421_*` only, but the actual collision spans
        # device/, host/, class/*_host, and portable/analog/max3421/).
        #
        # Strategy: exclude every tinyusb-core directory; keep only `arduino/`
        # and the top-level Adafruit_TinyUSB.h/cpp shim. The `arduino/ports/esp32/`
        # subtree provides the ESP32-specific glue that calls into the prebuilt
        # libarduino_tinyusb.a.
        "srcFilter": [
            "+<*>",
            "-<class/>",
            "-<common/>",
            "-<device/>",
            "-<host/>",
            "-<osal/>",
            "-<portable/>",
            "-<tusb.c>",
        ],
    },
}


def _patch_libdep(libdeps_root: str) -> None:
    lib_dir = os.path.join(libdeps_root, LIB_NAME)
    if not os.path.isdir(lib_dir):
        return
    target = os.path.join(lib_dir, "library.json")
    try:
        if os.path.isfile(target):
            with open(target, "r", encoding="utf-8") as fh:
                current = json.load(fh)
            if current == EXPECTED:
                return
        with open(target, "w", encoding="utf-8") as fh:
            json.dump(EXPECTED, fh, indent=2)
        print(
            "[exclude_tinyusb_max3421] wrote {} excluding portable/analog/max3421/".format(
                target
            )
        )
    except OSError as exc:
        print("[exclude_tinyusb_max3421] WARNING: could not patch {}: {}".format(target, exc))


def _patch_blemidi(libdeps_root: str) -> None:
    header = os.path.join(libdeps_root, BLE_MIDI_LIB, BLE_MIDI_HEADER)
    if not os.path.isfile(header):
        return
    try:
        with open(header, "r", encoding="utf-8") as fh:
            content = fh.read()
        if BLE_MIDI_REPLACEMENT in content:
            return  # already patched
        if BLE_MIDI_NEEDLE not in content:
            print(
                "[exclude_tinyusb_max3421] WARNING: BLE-MIDI header layout changed; "
                "skipping inline patch ({})".format(header)
            )
            return
        patched = content.replace(BLE_MIDI_NEEDLE, BLE_MIDI_REPLACEMENT, 1)
        with open(header, "w", encoding="utf-8") as fh:
            fh.write(patched)
        print(
            "[exclude_tinyusb_max3421] patched BLE-MIDI {}: marked begin() inline".format(
                header
            )
        )
    except OSError as exc:
        print("[exclude_tinyusb_max3421] WARNING: could not patch {}: {}".format(header, exc))


libdeps_dir = env.subst("$PROJECT_LIBDEPS_DIR")  # noqa: F821
env_name = env.subst("$PIOENV")  # noqa: F821
_patch_libdep(os.path.join(libdeps_dir, env_name))
_patch_blemidi(os.path.join(libdeps_dir, env_name))
