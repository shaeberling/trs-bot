# Solenoid test rig

The first hardware test: a Raspberry Pi Pico fires one or two solenoids
through MOSFET boards. The goals are to confirm the solenoid presses a Model I
key reliably, to tune the pulse length and holding duty, and later to measure
the round-trip latency with the camera.

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="images/solenoid-circuit-dark.svg">
    <img src="images/solenoid-circuit.svg" alt="Circuit diagram: the Pico's GP15 and GND go to the MOSFET board's TRIG and GND; a 12 V supply feeds VIN+ and VIN−; OUT+ and OUT− go to the solenoid, with a 1N5819 diode across it, stripe toward +12 V; the solenoid's plunger presses a TRS-80 key" width="800">
  </picture>
</p>

## Position and force

The JF-0530B solenoid has 10 mm of travel. When powered, it pulls its plunger
into the coil, which pushes the rod out the other end. The force depends on how
much of the air gap inside the coil is left to close. At 12 V, from the
datasheet:

| Gap left | Force |
|---|---|
| 10 mm (at rest) | almost nothing |
| 5 mm | ~0.8 N |
| 1 mm | ~2.5 N |
| 0 mm (closed: plunger in, rod fully extended) | ~5 N |

A Model I key probably needs under 1 N (to be measured), so for the first
5 mm or so of its travel the solenoid can barely push it. That's why the first
test only worked with the rod starting a few millimeters above the key: it has
to reach the key late in the stroke, where the force is high.

We should measure these things on real Model I keyboards, both Alps and Hi-Tek:

- Force needed to push the key, at the activation point and at the bottom. (It
  probably isn't constant across the push.)
- Distance from unpushed to start-of-activation.
- Distance from start-of-activation to fully pushed down.
- Height of the keycap above whatever the rig rests on, which sets the mount
  geometry.

To find the activation point, run `10 PRINT @0, PEEK(14400);: GOTO 10`. It
shows keyboard row 6 (`0x3840`), where SPACE is bit 7 (128). Then lower the key
in measured steps (feeler gauges, shims, or calipers) and note where the value
changes.

We might want to hold keys down for a long time (say, for thrust
or steering), which at full power would overheat the solenoid (see the next
section). We can avoid this by using PWM to lower the power. That also lowers
the force, but much less once the air gap is closed: the steel is saturated,
so at zero gap a tenth of the power still gives about 4 N instead of 6 N. So
we use full power while pushing the key, until the rod is fully extended and
the air gap is closed, then drop to a holding duty. The full-power phase has to
last until the plunger has actually closed. If it drops too early, the plunger
stalls with a gap left, where the holding force is weak. So we should also
measure how long the plunger takes to close.

Since the rod has to extend fully, and the height might be too fussy to
calibrate on a real machine at a show, we can allow some slop by adding a
spring at the end of the rod. It screws onto the rod's M3(?) thread, where the
rubber cap goes now. The spring has to be:

- Strong enough: where the key bottoms out (or at least passes activation),
  the spring has to push harder than the key's spring, so more than about 1 N.
- Weak enough: in the last millimeter or two of the stroke, the key's spring
  plus this spring have to stay under what the solenoid delivers there (about
  2.5 N at 1 mm). So about 1–2 N when fully compressed.

How far the spring can compress is how much calibration slop we get.

## Power and duty cycle

The datasheet's force curves are for steady DC at 1.8, 3.6, 7.2 and 18 W.
Each has a maximum duty cycle: the fraction of time the coil may be on,
averaged over seconds to minutes. Power × duty cycle is 1.8 W for every line,
which is the heat the coil can get rid of continuously. The datasheet also
says not to leave it on for more than about 2 seconds at a time.

From the 12 V supply, fast PWM (10–20 kHz) acts like a lower DC voltage. The
coil's inductance smooths the current to about duty × 12 V / 13.7 Ω, so power
is about duty² × 10.5 W. Slow PWM (around 1 kHz) lets the current ripple and
makes the coil whine.

| Datasheet line | Max duty cycle | DC on our 13.7 Ω coil | PWM duty from 12 V |
|---|---|---|---|
| 1.8 W | 100% | 5.0 V | ~41% |
| 3.6 W | 50% | 7.0 V | ~59% |
| 7.2 W | 25% | 9.9 V | ~83% |
| 10.5 W (12 V DC) | ~17% (interpolated) | 12 V | 100% |
| 18 W | 10% | 15.7 V | not possible from 12 V |

So keep the average under about 1.8 W:

- Holding at ~40% PWM (~1.7 W) can last indefinitely.
- Full-power taps can take up about 17% of the time, e.g. three 50 ms taps per
  second, sustained.

## Parts

Details and links are in [PARTS.md](../PARTS.md).

| Part | Qty |
|---|---|
| Raspberry Pi Pico | 1 |
| JF-0530B solenoid, 12 V ~0.85 A | 2 |
| Dual MOSFET trigger switch board (D4184) | 1 per solenoid |
| 1N5819 Schottky diode | 1 per solenoid |
| 12 V 5 A power supply, with barrel-to-screw-terminal adapter | 1 |

## Wiring

For each solenoid:

1. 12 V supply + and − to the board's **VIN+** and **VIN−**.
2. Solenoid to **OUT+** and **OUT−**. Its polarity doesn't matter.
3. Diode across the solenoid, **stripe to OUT+** (+12 V). Backwards, it shorts
   the supply as soon as the board turns on.
4. Pico GPIO to **TRIG**: GP15 for the first solenoid, GP14 for the second.
5. Pico **GND** to the board's **GND**. Without a shared ground the trigger
   doesn't work.

The board is a low-side switch: OUT+ is always at +12 V, and TRIG high connects
OUT− to ground. The MOSFET's built-in body diode doesn't protect it from the
turn-off spike, which is why the external diode is needed.

## First test (MicroPython)

Start with short pulses:

```python
from machine import Pin
import time

sol = Pin(15, Pin.OUT, value=0)
for _ in range(10):
    sol.on()
    time.sleep_ms(40)
    sol.off()
    time.sleep_ms(500)
```

Then try peak-and-hold, which keeps a held key from overheating the solenoid:

```python
from machine import Pin, PWM
import time

pwm = PWM(Pin(15), freq=10_000, duty_u16=0)
pwm.duty_u16(65535)   # full power until the plunger has closed
time.sleep_ms(20)     # measure how long closing actually takes
pwm.duty_u16(26000)   # ~40% (~1.7 W) to hold; tune down until the key slips
time.sleep(2)
pwm.duty_u16(0)
```

## Things to check

- **Nothing fires at boot.** The Pico's GPIOs start with a weak pull-down. If
  the solenoid twitches at power-up or reset, add a 10 kΩ resistor from TRIG to
  GND.
- **The MOSFET turns fully on.** A reviewer measured the board's trigger
  switching at about 3 V, close to the Pico's 3.3 V. With the solenoid held
  on, the voltage across the solenoid should be nearly the full 12 V, and the
  board should stay cool. If not, put a 5 V buffer (e.g. 74AHCT125) between the
  Pico and TRIG.
- **The key registers.** The game has to see the key for at least one scan, so
  30–50 ms is the floor. Find the shortest pulse that registers every time on
  both Alps and Hi-Tek keyboards.
- **Heat.** Held at full power, the solenoid dissipates about 10 W. Feel it
  after a few seconds of holding, with and without peak-and-hold.
- **Release time.** With a plain diode, the coil current fades slowly, so the
  plunger lets go a few milliseconds late. If that shows up in the latency
  measurements, a ~18 V Zener in series with the 1N5819 speeds it up.
