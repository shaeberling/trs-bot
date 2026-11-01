

https://www.adafruit.com/product/413 (out of stock)

https://www.digikey.com/en/products/detail/adafruit-industries-llc/413/5819057

Large push-pull solenoid
12 V, 10 mm throw, 6 Newton starting force, ~12 Ω coil
Adafruit says "about 250mA", but 12 V / 12 Ω is ~1 A, so plan for 1 A.
40 mm body, 144 g, 4x M3 threaded mounting holes.
put a small rubber bumper or felt pad on each plunger tip

---

Cheaper alternative: generic JF-0530B solenoid, 2-pack, $12.88

https://www.amazon.com/dp/B0FFGJVV8N

12 V, 850 mA (~14 Ω), 5 N, 10 mm stroke, push-pull, return spring.
30x16x15 mm body, 6x58 mm plunger, 2.5 mm mounting holes.
Smaller and lighter than the #413, which helps for neighboring keys.
No duty-cycle rating, so assume 50% or worse and use peak-and-hold.
The listing also says 300 mA in one place, so measure the coil resistance.

Lawrence bought a pair Oct 31.

---

ULN2803A

https://www.digikey.com/en/products/detail/stmicroelectronics/ULN2803A/599591

Bipolar (BJT) Transistor Array 8 NPN Darlington 50V 500mA 2.25W Through Hole 18-DIP

Not enough for these solenoids: ~1 A each is over the 500 mA channel limit, and
~1 W per active channel quickly reaches the 2.25 W package limit.

---

Logic-level N-MOSFET per solenoid (e.g. AO3400 or IRLZ44N), plus a flyback
diode (e.g. 1N4001 or 1N5819) across each coil. Replaces the ULN2803A.

For the Pico test, a ready-made board instead of a bare MOSFET:
dual MOSFET trigger switch module, 6-pack, $5.49
https://www.amazon.com/dp/B0FMJH3DML
DC 5-36 V load, 3.3-20 V trigger (so the Pico's 3.3 V GPIO works), 0-20 kHz
PWM, screw terminals. Unclear whether it has a flyback diode, so add one. (The
MOSFET's body diode doesn't count: it's reverse-biased by the turn-off spike.)
Low-side switch. A reviewer measured the trigger switching at about 3 V, which
is close to the Pico's 3.3 V, so check that OUT+/OUT- drops to near 0 V when on.

---

Flyback diodes: 1N5819 Schottky, 1 A 40 V, 100-pack, $7.99
https://www.amazon.com/dp/B079KG1TN2
One across each solenoid, cathode (stripe) to +12 V.

---

12 V 5 A power supply, ALITOVE, $12.99
https://www.amazon.com/dp/B01GEA8PQA
5.5x2.5 mm plug, comes with a female barrel-to-screw-terminal adapter.
Two solenoids need ~2 A; 5 A leaves room for more keys later.

---

3D printed case adaptor

Adjustable solenoid height because keyboards vary and solenoid power force drops
off sharply with distance.

---

Adafruit FT232H breakout (#2264)
https://www.adafruit.com/product/2264
Adafruit FT232H Breakout - General Purpose USB to GPIO, SPI, I2C - USB C & Stemma QT

---

Numato Lab 8-channel USB GPIO module
https://numato.com/product/8-channel-usb-gpio-module-with-analog-inputs
8 Channel USB GPIO Module With Analog Inputs
Shows up as serial port.

---

MCP2221A breakout (Adafruit #4471)
It works the same way as the FT232H but has only four GPIO pins, one short of what you need.

---

Controller board:

    Pico / Pico 2: USB Full-Speed (12 Mbps), 1 ms frames.
    ESP32-S3: USB Full-Speed (12 Mbps), 1 ms frames.
    Teensy 4.0: USB High-Speed (480 Mbps), 125 µs microframes.

