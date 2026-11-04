# Programming the Pico in C

How to set up a command-line C/C++ toolchain for the Raspberry Pi Pico 2 W
(RP2350), which drives the solenoids. The [solenoid test](solenoid-test.md)
has the wiring; this page covers building and flashing firmware for it.

The toolchain is the official **Pico SDK** plus the **Arm GNU Toolchain**
(`arm-none-eabi-gcc`). The SDK's build is CMake, but CMake can generate plain
Makefiles, so the day-to-day loop is `make`. Raspberry Pi's VS Code extension
wraps the same tools and isn't needed.

## Already set up

On the main dev machine the SDK (2.3.1) is at `~/others/pico/pico-sdk`, and
`~/.zshenv` sets `PICO_SDK_PATH` to it. The Arm toolchain, CMake and Ninja are
installed. Skip to [A minimal project](#3-a-minimal-project).

The rest of this page is for setting up a new machine.

## 1. Install the tools

### macOS

The host compiler and git come from the Xcode command-line tools, if they
aren't there already:

```bash
xcode-select --install
```

Then the Arm toolchain, build tools, and picotool:

```bash
brew install --cask gcc-arm-embedded
```

```bash
brew install cmake ninja libusb picotool
```

Use the **cask** (`gcc-arm-embedded`), which is Arm's official build. Homebrew
also has a plain `arm-none-eabi-gcc` formula, but it has no C library
(newlib), so nothing that includes `<stdio.h>` will link. To check which one
you have:

```bash
arm-none-eabi-gcc -print-file-name=libc.a
```

That should print a full path ending in `arm-none-eabi/lib/libc.a`. A bare
`libc.a` means there's no newlib.

### Debian / Ubuntu

```bash
sudo apt install build-essential cmake ninja-build git libusb-1.0-0-dev pkg-config gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
```

Distro toolchains can lag. If yours is too old, download the
`arm-none-eabi` build for your platform from Arm's
[GNU toolchain downloads](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
and put its `bin` directory on your `PATH`.

### picotool

picotool loads firmware over USB and inspects binaries. If the build doesn't
find an installed one it can use, the SDK downloads and builds its own on the
first build. That's slower but works, as long as libusb is installed.

## 2. Get the SDK

Pick a directory; `~/pico` is a common choice.

```bash
git clone https://github.com/raspberrypi/pico-sdk.git ~/pico/pico-sdk
```

```bash
cd ~/pico/pico-sdk && git submodule update --init
```

Don't skip the submodules: they include TinyUSB (needed for USB serial) and
the CYW43 Wi‑Fi driver (needed on the W boards, even just for the LED). The
Pico 2 W needs SDK 2.1.0 or later. The `master` branch is always the latest
release, so to update an existing checkout:

```bash
cd $PICO_SDK_PATH && git pull && git submodule update --init
```

Then add this to `~/.zshenv` (or `~/.bashrc`), using your path:

```bash
export PICO_SDK_PATH=~/pico/pico-sdk
```

## 3. A minimal project

This fires the solenoid on GP15 ten times with the same 40 ms pulses as the
MicroPython test, and blinks the onboard LED along with it.

`CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.13)
set(PICO_BOARD pico2_w)            # must come before the SDK import
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(solenoid C CXX ASM)
pico_sdk_init()

add_executable(solenoid main.c)
target_link_libraries(solenoid pico_stdlib pico_cyw43_arch_none)
pico_enable_stdio_usb(solenoid 1)  # printf over USB serial
pico_enable_stdio_uart(solenoid 0)
pico_add_extra_outputs(solenoid)   # makes the .uf2
```

`main.c`:

```c
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#define SOLENOID_PIN 15

int main(void) {
    stdio_init_all();
    cyw43_arch_init();               // the W's LED is on the Wi-Fi chip
    gpio_init(SOLENOID_PIN);
    gpio_set_dir(SOLENOID_PIN, GPIO_OUT);
    gpio_put(SOLENOID_PIN, 0);

    sleep_ms(2000);                  // time to open the serial port
    for (int i = 0; i < 10; i++) {
        printf("press %d\n", i);
        gpio_put(SOLENOID_PIN, 1);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(40);
        gpio_put(SOLENOID_PIN, 0);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(500);
    }
    printf("done\n");
    while (true) {
        sleep_ms(1000);
    }
}
```

On the W boards the onboard LED is wired to the Wi‑Fi chip, not to a GPIO, so
blink examples that use `PICO_DEFAULT_LED_PIN` don't work. Use
`cyw43_arch_gpio_put` as above. A plain Pico 2 (no W) would use
`PICO_BOARD pico2` and could drop `pico_cyw43_arch_none`.

Build:

```bash
cmake -S . -B build -G "Unix Makefiles" && make -C build -j
cmake -S . -B build -G Ninja && ninja -C build
```

After the first configure, only the `make` (or `ninja`) step is needed; it
re-runs CMake when `CMakeLists.txt` changes.

## 4. Flash and watch the output

- **First time:** hold BOOTSEL while plugging in USB. A drive called `RP2350`
  mounts. Copy `build/solenoid.uf2` onto it and the board reboots into the
  program.
- **After that:** while a program built with USB stdio is running, picotool can
  reboot the board into BOOTSEL by itself, so there's no button to press:

  ```bash
  picotool load -fx build/solenoid.uf2
  ```

  If a program has crashed or doesn't use USB stdio, fall back to BOOTSEL.
- **Serial output:** the board shows up as `/dev/tty.usbmodem*` on macOS
  (`/dev/ttyACM*` on Linux). Use `screen /dev/tty.usbmodemXXXX` (quit with
  Ctrl-A K), or `tio` (`brew install tio`), which reconnects by itself when the
  board reboots.

## Notes

- **Debugging:** the Raspberry Pi Debug Probe (~$12) plus Raspberry Pi's
  OpenOCD fork gives GDB over SWD. It isn't needed for the solenoid test.
- **RP2350 erratum E9:** an input pin with the internal pull-down enabled can
  get stuck high. Outputs such as TRIG aren't affected. For a button or sensor
  input, use an external pull-down or the internal pull-up.
- **Timing:** for peak-and-hold, use the SDK's hardware PWM (`hardware/pwm.h`)
  and timer alarms (`pico/time.h`). They give exact, jitter-free pulses, which
  is the main reason to use C here instead of MicroPython.
