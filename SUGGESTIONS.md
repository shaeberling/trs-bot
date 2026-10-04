# Suggestions

From a review of the project in October 2026, when the repo was only `README.md`
and `PARTS.md`. They're suggestions, not decisions.

## Overall

The plan is doable and would make a great demo. The riskiest part isn't the ML.
It's getting reliable vision and timing on an unfamiliar Model I in a
convention hall. Work on those early.

## 1. Have the camera rebuild the screen as characters

Don't train on raw pixels. Have the camera stage output the 64×16 character
grid, which is the same 1 KB that sits in video RAM at `0x3C00`. Codes 128–191
are 2×3 block graphics and can be decoded exactly. Only text characters need a
classifier.

Why:
- The game-playing policy trains on the emulator's exact video RAM, so the only
  differences from the real machine are camera errors and timing.
- The camera stage can be built and tested against a real Model I without any
  ML in place.
- The policy's input is tiny (1,024 bytes per frame, plus a few past frames).

## 2. Use the keyboard matrix layout

Enter, Clear, Break, Up, Down, Left, Right and Space are all in row 6
(`0x3840`; confirmed in `~/mine/trs80/packages/trs80-emulator/src/Keyboard.ts`).
The Model I matrix has no diodes, so some combinations of 3+ keys ghost, but
keys within one row never do. Arrows plus fire can be pressed together safely.

Games usually scan the matrix directly instead of using the ROM, so a press has
to last longer than one scan by the game. Aim for at least 30–50 ms.

## 3. Manage solenoid heat

The Adafruit #413 (12 V, ~250 mA, ~12 Ω, 10 mm throw, 6 N) is rated for a **50%
duty cycle**. Steering means holding keys down, so use peak-and-hold drive:
full power for about 20 ms to pull the plunger in, then drop to a much lower
PWM duty to hold it.

ULN2803A notes:
- Tie COM (pin 10) to +12 V so the built-in flyback diodes work.
- Each channel handles 500 mA, but the package is limited to ~2.25 W in total.
  At ~1 V Vce(sat) and 250 mA, that's ~0.25 W per active channel. Several keys
  held at once get warm, and peak-and-hold helps here too.

## 4. Consider a Raspberry Pi Pico for the key driver

Use a Pico instead of the FT232H, Numato or MCP2221A. A microcontroller gives:
- precise pulse timing that the host OS can't disturb
- peak-and-hold PWM per channel
- USB CDC serial to the laptop
- timestamped presses, useful for measuring latency

You already have Pico experience (`~/mine/pico-rtos-test`). It also has plenty
of GPIO, so you aren't stuck at "one short".

## 5. Set up the camera for a CRT

- Lock the exposure to a multiple of the refresh period: 1/60 s or 1/30 s on a
  60 Hz (US) machine, 1/50 s on a 50 Hz (UK/EU) machine. Otherwise rolling bars
  and dark bands appear.
- Turn off auto exposure, auto white balance and auto focus.
- For geometry, click the four screen corners once at setup, then refine with a
  mesh or thin-plate spline to correct for CRT curvature. This is simple and
  robust at a show.

## 6. Measure the full loop delay early

The loop is camera exposure, USB transfer, decoding, inference, serial to the
driver, and solenoid travel. Expect roughly 50–150 ms. To measure it, have a
solenoid press a key that changes the screen, then time how long until the
camera sees the change.

Then simulate that delay during training, with random jitter, random press
durations, and the occasional misread cell. This is the main thing that breaks
when moving from the emulator to the real machine.

## 7. Training approach

- The emulator can save and restore its state, so in simulation you can search
  ahead (Go-Explore style, or plain tree search over key choices) to generate
  expert play. Then train a fast reactive policy to copy it, and fine-tune
  with RL if needed. This will probably beat plain DQN, which
  `~/mine/defense-command-ai` suggests was hard to get working.
- A hand-written bot per game that reads the reconstructed screen is a solid
  fallback for the demo, and a good baseline to compare against.

## 8. "Any Model I" varies

Plan for differences in:
- **Keyboard**: Alps SKCC or Hi-Tek, with different key heights and feel. The
  adjustable solenoid mount in PARTS.md already covers this.
- **Monitor**: brightness, contrast, focus, geometry, curvature, burn-in.
- **Character generator**: with or without the lowercase mod, and possibly
  different fonts.
- **Mains frequency**: 60 Hz or 50 Hz, which affects camera exposure and game
  speed.

Train the character reader on synthetic renders from the emulator, augmented
with blur, bloom, curvature, glare, moiré, noise and brightness changes. Add
some real captures from at least two different machines.

## Open decisions

- **Which games?** Candidates are the Big Five and Adventure International
  arcade titles: Robot Attack, Cosmic Fighter, Meteor Mission II, Galaxy
  Invasion, Attack Force, Defense Command, Scarfman. For each game, decide
  what "win" means; many never end, so it could be a target score or level.
- **Game identification**: should the robot recognise the running game from
  the screen by itself, or be told?
- **Emulator connection**: how to connect the TypeScript emulator
  (`~/mine/trs80`) to Python for fast, parallel headless training. Options
  include a socket bridge, running the training loop in Node, or a separate
  fast Z80 core.

## Next steps

1. Use the trs80 MCP tools to load each candidate game, and record which keys
   it uses and how it scores and ends.
2. Benchmark how fast the TS emulator runs headless.
3. Point a camera at a real Model I, lock the exposure, and prototype the
   screen-to-grid reader for block graphics.
4. Build a one-solenoid test rig and measure the loop delay.

## Sources

- [Adafruit #413 Large Push-Pull Solenoid](https://www.adafruit.com/product/413)
- [Tandy Assembly](https://www.tandyassembly.com/)
- [Deskthority: Model I Alps vs Hi-Tek keyboards](https://deskthority.net/viewtopic.php?t=13003)
- [Deskthority wiki: TRS-80 Model I](https://deskthority.net/wiki/Radio_Shack_TRS-80_Model_I)
- [trs-80.org: Model I keybounce](http://www.trs-80.org/model-1-keybounce/)
