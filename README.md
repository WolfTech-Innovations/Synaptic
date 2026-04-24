# Synaptic 
# MPCE; Mathematical Proto-conciousness engine
**WolfTech Innovations**

Cognitive processing system. MSingle-binary C++23. Runs on Linux x86-64.

---

## Overview

Synaptic is a self-contained processing engine that maintains an internal state model across sessions, accepts text input via HTTP, and returns processed text output. State is persisted to disk. A web interface is served locally.

---

## Requirements

- Linux x86-64
- C++23 compiler (GCC 13+ or Clang 17+)
- No external runtime dependencies

---

## Build

```bash
g++ -std=c++23 -O2 -o synaptic main.cpp agi_api.cpp web_server.cpp module_integration.cpp -lpthread
```

---

## Usage

```bash
./synaptic
```

Web interface available at `http://localhost:8080` after boot.

State is loaded from `state.dat` on startup if present. State is saved to `state.dat` on `SIGINT` or `SIGTERM`, and auto-saved every 200 processing cycles.

`CTRL+C` to exit cleanly.

---

## API

All endpoints are served on port `8080`.

### `GET /`
Returns the web UI.

---

### `POST /api/chat`

Submit input for processing.

**Request**
```json
{ "message": "string" }
```

**Response**
```json
{
  "status": "ok",
  "response": "string",
  "valence": -0.3142
}
```

`valence` is a float in the range `[-1.0, 1.0]` reflecting the system's internal affective state at the time of response generation. Negative values indicate inhibitory/aversive weighting; positive values indicate approach/reward weighting. Value is sourced directly from the engine's live state — not inferred from output text.

---

### `POST /api/save`

Writes current state to `state.dat`.

**Response**
```json
{ "status": "saved" }
```

---

### `POST /api/load`

Loads state from `state.dat`, overwriting current in-memory state.

**Response**
```json
{ "status": "loaded" }
```

---

## Internal Architecture

Synaptic is a single-file core (`main.cpp`) with a separate HTTP layer (`agi_api.cpp`). All subsystems share a global `State S` instance.

### Processing Layers

| Component | Description |
|---|---|
| Deliberation | 147-layer sequential processing pipeline |
| Embeddings | 1024-dimensional token concept vectors |
| Transformer heads | 4 attention heads, 1024-dim |
| Context window | Up to 1024 tokens |
| Semantic lexicon | 137-entry quad-directional grounding table |
| Cross-domain reasoner | 8-domain token classification and bridge discovery |
| Association matrix | Token co-occurrence and concept linkage tracking |
| Consolidation cycle | Periodic memory replay and embedding stabilisation |
| Goal system | Valence-driven goal formation and decay |
| Neuron pool | Dynamic neuron generation and mutation |

### Theoretical Basis

Internal weighting and integration metrics are derived from IIT (Integrated Information Theory), GWT (Global Workspace Theory), and HOT (Higher-Order Thought) frameworks. These inform the `phi` integration metric and `metacognitive_awareness` scalar

### State Schema (partial)

```
state.dat       — primary save file
state_emergency.dat — written on sustained error (>10 consecutive loop failures)
state_fatal_error.dat — written on unhandled exception in main()
state_unknown_error.dat — written on non-std::exception fatal
```

---

## Valence & Audio Interface

The web UI (`/`) renders a four-dot visualiser driven by `valence` returned in `/api/chat` responses. Audio tones are generated client-side via the Web Audio API and played only when valence changes by more than `±0.08` from the previous response. Tone scale and waveform type vary with valence polarity.

No audio is produced for stable or low-drift responses.

---

## File Structure

```
main.cpp              — core engine, ~14,000 lines
agi_api.cpp           — HTTP server and UI
agi_api.h             — AGI_API class declaration
web_server.h/cpp      — raw HTTP server
module_integration.h/cpp — external module interface
state.h               — State struct definition
struct.h              — shared data structures
curses_compat.h       — terminal compatibility
uac.h                 — user access control
state.dat             — persisted state (created on first save)
```

---

## Signals

| Signal | Behaviour |
|---|---|
| `SIGINT` | Save state to `state.dat`, exit 0 |
| `SIGTERM` | Save state to `state.dat`, exit 0 |
| `SIGABRT` | Log to stderr, exit 1 (no save) |

---

## Notes

- First run initialises the processing substrate with default parameters. Subsequent runs load from `state.dat`.
- The engine continues processing internally between requests. Valence, attention, and goal state evolve independently of user input.
- Auto-save output is printed to stdout: `[autosave] Gen N`.
- Loop errors are counted. If errors exceed 10 consecutively, an emergency state dump is written and the loop pauses for 5 seconds before resuming.

---

*WolfTech Innovations*
