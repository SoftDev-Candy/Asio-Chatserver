# SOUL Architecture

> [!WARNING]
> This is still a student-project architecture doc, not a giant multimillion dollar crowd funded project ........... ┌( ಠ_ಠ)┘.
> But it is finally updated enough for me to not cry about it ....yet

## Current Shape

SOUL is split into a few practical modules:

![SOUL Preview](SOUL_preview.gif)

| Folder | Job |
|---|---|
| `Common/` | shared frame codec + telemetry model |
| `SatelliteSender/` | TCP client that simulates and sends telemetry |
| `simulation/` | tiny per-satellite behavior logic |
| `ReceiverServer/` | TCP server that receives frames and sends ACKs |
| `Database/` | SQLite setup and inserts |
| `ui/` | Qt operator window and telemetry tables |
| `OrbitalRendering/` | Earth, camera, satellite visuals, orbit scene |
| `docs/` | requirement notes, protocol notes, architecture notes, media |

## Current End-To-End Flow

```text
SimulationState (SAT_1 / SAT_2 / SAT_3)
  -> make next telemetry frame
  -> TelemetryHub sends framed TCP packet
  -> Receiver decodes packet
  -> Database inserts telemetry row into SQLite
  -> Receiver sends ACK
  -> Qt MainWindow reads SQLite
  -> OrbitView + tables update from latest state
```

## Sender Side

The sender is still intentionally small:

- `TelemetryHub` owns the socket and send loop
- `SimulationState` owns the fake health behavior per satellite
- each send round walks through all satellites one by one

This is not a huge networking architecture and thats okay.

### Current Satellite Modes

| Satellite | Mode | What It Does |
|---|---|---|
| `SAT_1` | `Nominal` | steady basic behavior |
| `SAT_2` | `PowerDrain` | extra battery drop |
| `SAT_3` | `SignalLoss` | heats up and sometimes skips sending |

## Receiver Side

The receiver does four main things:

1. accept a TCP client
2. decode framed telemetry
3. store telemetry in SQLite
4. send a small ACK back

It also keeps a latest-state map in memory, but SQLite is the thing the UI reads from for now.

## UI Side

The Qt window now has two table roles:

- `SatelliteTable`
  - latest row per satellite
  - selection target for the operator
- `DatabaseTable`
  - recent telemetry rows for the selected satellite

The detail panels stay hidden until a satellite is selected, which makes the screen way less noisy tbh.

## Orbit Rendering Side

`OrbitView` currently handles:

- Earth textured mesh
- day/night Earth shader
- reusable satellite textured mesh
- three `SatelliteVisual` orbit definitions
- orbit trails
- satellite glow markers
- camera drag + zoom

Rendering behavior right now:

- if no satellite is selected, all three can be shown
- if one satellite is selected, only that one is rendered for clarity

## Data Ownership Notes

- SQLite remains the shared truth between backend and UI
- no database schema change was needed for the recent multi-satellite work
- the same satellite mesh is reused for all rendered satellites
- link health is inferred from stale `received_ms`, not directly transmitted

## Docs + Media

Useful files in this folder now:

- [01_requirement.md](01_requirement.md)
- [02_protocol.md](02_protocol.md)
- [SOUL.mp4](SOUL.mp4)
- [SOUL_preview.gif](SOUL_preview.gif) for the quick preview version

Also yes the screenshots are still here because they are useful and because deleting them would be kinda rude.
