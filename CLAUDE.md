# Notes for Claude

TRS-Bot is a robot that plays video games on an **unmodified** TRS-80 Model I.
A USB camera on a tripod watches the CRT, and solenoids sitting on top of the
keyboard press the keys. The policy is trained in simulation. The demo is at
**Tandy Assembly 2027** (about October 2027; 2026 was Oct 2–4 in Cincinnati).
At the show it must work on any Model I: aim the camera, set the key rig on the
keyboard, then play and win whatever supported game is running.

## Current state (Oct 2026)

There's no code yet. `README.md` has the goal, `PARTS.md` has the candidate
parts, and `SUGGESTIONS.md` has the design review: the reasoning, open
decisions, next steps and sources. As code arrives, add the layout and commands
here.

## Hard constraints

- No changes to the Model I: no expansion interface, no reading RAM, no taps on
  the bus or keyboard matrix. All input comes from the camera and all output
  goes through physical key presses.
- It must work on *any* Model I. Expect differences in keyboard type (Alps SKCC
  or Hi-Tek, with different key heights and feel), monitor (brightness, focus,
  geometry, curvature), character generator ROM (with or without lowercase), and
  mains frequency (60 Hz in the US, 50 Hz in the UK and Europe).
- Setup at the show has to be quick and done by hand: a tripod, a rig placed on
  the keyboard, and a laptop.

## Hardware notes

- Solenoid: Adafruit #413. 12 V, ~250 mA, ~12 Ω, 10 mm throw, 6 N, rated **50%
  duty cycle**. Holding a key down for a long time (as when steering) heats it,
  so use peak-and-hold PWM: full power for ~20 ms, then a much lower holding
  duty.
- Driver: ULN2803A. Connect COM (pin 10) to +12 V to use the built-in flyback
  diodes. Each channel handles 500 mA, but the package is limited to ~2.25 W in
  total. At ~1 V Vce(sat) and 250 mA, that's ~0.25 W per active channel, so
  several keys held for a long time get warm.
- USB-to-GPIO options in PARTS.md: FT232H, Numato 8-ch, MCP2221A (only 4 GPIO,
  one short). A microcontroller such as a Raspberry Pi Pico is worth a look. It
  gives timed pulses, peak-and-hold PWM, and USB CDC serial, and it timestamps
  presses on its own clock.
- The rig is a 3D-printed adapter with adjustable solenoid height, because the
  force falls off quickly with distance and keyboards differ.

## Useful Model I facts

- **Keyboard matrix**: memory-mapped at `0x3800–0x38FF`. Row 6 (`0x3840`) has
  ENTER(b0) CLEAR(b1) BREAK(b2) UP(b3) DOWN(b4) LEFT(b5) RIGHT(b6) SPACE(b7).
  SHIFT is row 7 bit 0. The matrix has no diodes, so some combinations of 3+
  keys ghost. Keys within one row never ghost, so arrows + space is safe.
  Games usually scan the matrix directly, so a press has to last longer than
  one scan period of the game (aim for 30–50 ms or more).
- **Screen**: 64×16 characters in 1 KB of video RAM at `0x3C00`. Each cell is
  6×12 pixels (384×192 in total). Codes 128–191 are 2×3 block graphics, which
  give a 128×48 graphics grid. There's also a 32-column wide mode. The whole
  game state that the robot can see fits in 1,024 bytes.
- Refresh is 60 Hz in the US and 50 Hz in the UK, which matters for camera
  exposure (see below).

## Intended architecture

1. **Screen reader**: camera frame → 64×16 grid of character codes, the same
   thing that's in video RAM. The steps:
   - Calibrate the screen geometry. Clicking the four corners by hand works,
     then refine with a mesh or thin-plate spline to handle CRT curvature.
   - Classify each cell. Block graphics can be decoded exactly. Text needs a
     classifier trained on synthetic renders from the emulator, augmented with
     blur, bloom, curvature, glare, moiré, and brightness changes.
   - Lock the camera exposure to a multiple of the refresh period (1/60 s or
     1/30 s, or 1/50 s on 50 Hz machines). Otherwise rolling bars appear.
2. **Policy**: video RAM grid (plus a few past frames) → key set. Trained in
   the emulator, where video RAM is exact, so the only gap between sim and the
   real machine is the screen reader plus timing.
3. **Actuator**: key set → solenoid pulses.

With this split, all ML training can run headless in the emulator, and the
screen reader can be tested on its own by pointing the camera at a real Model I.

### Bridging sim and the real machine

- Simulate the real end-to-end latency during training (camera exposure + USB +
  inference + solenoid travel, likely 50–150 ms), with random jitter, random
  press durations, and the occasional misread cell.
- Measure the real latency early. One way: a solenoid presses a key, the camera
  watches for the on-screen change, and you time the round trip.

## Related code (outside this repo)

- `~/mine/trs80`: the user's TypeScript TRS-80 monorepo. It has the Z80 and
  Model I/III emulator (`packages/trs80-emulator`), screen rendering, file
  formats, and `trs80-tool`, which includes an MCP server
  (`packages/trs80-tool/src/mcp.ts`). The `mcp__trs80__*` tools in this session
  are that server, so you can boot a Model I, load a game, read the screen and
  memory, and press keys. The key matrix map is
  `packages/trs80-emulator/src/Keyboard.ts`.
- `~/mine/defense-command-ai`: the user's earlier attempt (Python, Keras DQN)
  at the TRS8BIT 2024 Defense Command contest. It drove trs80gp (a Model III
  emulator) over TCP using a modified game that sent out entity state. That
  state isn't available here; TRS-Bot only has pixels.

## Open questions

- Which games? Big Five and Adventure International arcade titles are the
  obvious candidates (e.g. Robot Attack, Cosmic Fighter, Meteor Mission II,
  Galaxy Invasion, Attack Force, Defense Command, Scarfman). Check which keys
  each one uses, and what "win" means for games that never end.
- Should the robot identify the game from the screen by itself?
- RL (PPO or DQN) or search-and-distill? In sim, savestates allow lookahead
  search (Go-Explore style) to produce expert play, which can then be
  distilled into a reactive policy. A scripted bot per game is a reliable
  fallback.
- Is the TS emulator fast enough headless for RL, and how to connect it to
  Python: a socket bridge, or a separate fast core?
