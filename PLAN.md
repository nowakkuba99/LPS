# LPS Platform — Plan of Action

## Architecture

```
React Frontend (Vite)
      ↕  HTTP / WebSocket
Go Server (Docker)
   ├─ Job manager — spawns C++ subprocess per simulation
   ├─ PostgreSQL client — stores results and history
   └─ FFT computation — post-processing
      ↕  stdin/stdout pipe
C++ Compute Binary (native macOS — requires Metal)
   └─ SimulationEngine — LISA wave solver on GPU
      ↕
PostgreSQL (Docker)
```

**Why this split:**
- Metal cannot run in Docker — C++ must be native on macOS
- Go is far better than C++ for HTTP, JSON, auth, middleware, and DB access
- C++ stays minimal — just Metal compute + stdout streaming
- Go server and frontend can be fully Dockerized

**Running the stack:**
```bash
./start.sh          # docker-compose up (postgres) + Go server (native) + lps binary (native)
./lps --gui         # optional native window for live visualization
```

---

## Critical Constraint

Metal is a macOS-only API with no Docker/VM passthrough. The C++ compute binary (`lps`) always runs natively. Docker Compose handles PostgreSQL and the React frontend (nginx) only. The Go server is compiled for macOS and runs natively alongside the C++ binary.

---

## Phase 1 — Repository Cleanup & GitHub Setup ✅

- Fixed hardcoded shader path
- Renamed project from `10_Compute_LISA` to `lps`
- Vendored `metal-cpp` and `metal-cpp-extensions` into `third_party/`
- Created `.gitignore`, `README.md`, `PLAN.md`
- Initial commit on `main`

---

## Phase 1.5 — Code Cleanup ✅

- Renamed `triangle.metal` → `display.metal`
- Renamed kernel `add` → `computeLISA`
- Renamed `extortion` → `excitation` throughout
- Removed dead code: `MathHelpers.hpp`, unused overloads, `_semaphoreDisplay`, `hannWindowMultiply`
- Fixed display buffer counter bug

---

## Phase 2 — C++ Compute Subprocess ✅ (in progress)

**Goal:** Strip C++ to a minimal compute binary. No HTTP server in C++.

### Interface
```
./lps --excitation wave_mix --steps 30000
```
- Reads config from CLI flags
- Writes one readout float per line to stdout (streaming, flushed every 256 steps)
- Exits 0 on completion, non-zero on error
- `./lps --gui` opens the native Cocoa window (existing visualization)

### Files
- `src/SimulationEngine/` — Metal compute, extracted from Renderer (headless)
- `src/main.cpp` — mode select: `--gui` → Cocoa app, default → compute subprocess

---

## Phase 3 — Go Server

**Goal:** HTTP API server that manages simulation jobs by spawning C++ subprocesses.

### Stack
- Go 1.22+
- `chi` for HTTP routing
- `pgx` for PostgreSQL
- Standard library for everything else

### Job lifecycle
1. `POST /api/simulations` — Go creates a job record, spawns `./lps --excitation X --steps N`
2. Go reads stdout line-by-line, updating job progress in memory
3. On process exit, Go writes final results to PostgreSQL
4. `GET /api/simulations/:id` — returns status, progress, results

### REST endpoints
```
POST   /api/simulations          — create & start simulation
GET    /api/simulations          — list all (metadata only)
GET    /api/simulations/:id      — status + results when completed
DELETE /api/simulations/:id      — cancel (kills subprocess)
POST   /api/simulations/:id/fft  — compute FFT on stored results
GET    /api/simulations/:id/fft  — retrieve FFT result
```

### Directory structure
```
server/
  main.go
  jobs/
    job.go       — Job struct + View() for safe reads
    manager.go   — submit, list, cancel
    runner.go    — subprocess lifecycle
  api/
    router.go    — chi routes + CORS middleware
    handlers.go  — request handlers
  db/
    db.go        — PostgreSQL connection + migrations
    migrations/
      001_init.sql
  Dockerfile
  go.mod
```

---

## Phase 4 — Docker Compose

**Goal:** One-command launch of supporting infrastructure.

```yaml
# docker-compose.yml (project root)
services:
  postgres:    # PostgreSQL 16
  frontend:    # nginx serving built React app (Phase 5)
```

```bash
# start.sh
docker-compose up -d postgres
cd server && go run . &
```

Note: Go server and C++ binary run natively — they cannot run inside Docker due to Metal dependency.

---

## Phase 4.5 — Live Simulation Visualization

**Goal:** Watch the displacement field evolve in real time.

**Mode A — WebSocket frame streaming (primary)**
- After the headless compute path is established, add a WebSocket endpoint to the Go server
- C++ binary writes encoded frame data (e.g. raw float rows) to stdout periodically
- Go server fans this out to connected WebSocket clients
- React frontend renders frames on a `<canvas>` with jet colormap

**Mode B — Native popup window**
- `./lps --gui` runs the existing Cocoa/MTKView visualization
- Can be launched on demand from the frontend via a dedicated API endpoint

---

## Phase 5 — React Frontend

**Stack:** Vite + React 18 + TypeScript + TailwindCSS + shadcn/ui

**Libraries:**
- **Recharts** — time-domain displacement plots, FFT spectrum
- **react-three-fiber + drei** — 3D geometry viewer (material block, crack, transducers)
- **React Query** — data fetching, polling for simulation progress
- **React Hook Form + Zod** — simulation parameter forms with validation

**Pages / Views:**

| View | Features |
|---|---|
| **New Simulation** | Parameter form: excitation type, frequency, step count |
| **Live Run** | Progress bar, real-time displacement waveform, canvas live view (WebSocket) |
| **Results** | Time-domain plot, FFT spectrum, 2D heatmap of displacement field |
| **3D Geometry** | Three.js: material block, crack region, source/monitor points |
| **History** | Past simulations, filter by status/date, clone config |
| **Saved Configs** | Named parameter sets, save/load from DB |

---

## Phase 6 — FFT & Server-Side Analysis

- Use Go's `gonum/dsp` or call `vDSP_fft_zrip` via cgo for FFT
- `POST /api/simulations/:id/fft` — reads results from DB, computes spectrum, stores it
- Return frequencies + magnitudes; frontend plots frequency content
- Peak detection — annotate dominant frequencies in response

---

## Sequencing & Rough Effort

```
Phase 1    — Repo cleanup                ✅ done
Phase 1.5  — Code cleanup                ✅ done
Phase 2    — C++ compute subprocess      ~1 day   (strip server code, CLI interface)
Phase 3    — Go server                   ~3 days  (HTTP + job runner + DB)
Phase 4    — Docker Compose              ~0.5 day
Phase 4.5  — Live visualization          ~2 days  (WebSocket streaming + canvas)
Phase 5    — React frontend              ~5–7 days
Phase 6    — FFT analysis                ~1–2 days
```
