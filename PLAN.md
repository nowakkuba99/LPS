# LPS Platform — Plan of Action

## Critical Architectural Constraint

**Metal cannot run in a Linux Docker container.** Docker on macOS runs containers inside a Linux VM — Metal is a macOS-only API with no VM passthrough. This is a hard platform constraint, not a configuration issue. This shapes the entire architecture:

- The C++ simulation engine **always runs natively** on macOS as a local process
- Docker Compose handles only **PostgreSQL** and the **React/nginx frontend**
- A single startup script ties everything together

---

## Phase 1 — Repository Cleanup & GitHub Setup

**Goal:** Clean, portable, version-controlled codebase.

1. **Fix hardcoded path** in `src/FileReader/FileReader.hpp:17` — replace the absolute path with `fs::current_path()` so shaders load relative to the binary's working directory
2. **Rename project** — update Makefile target and source file from `10_Compute_LISA` to `lps-server`; update all internal references
3. **Move metal-cpp dependency** — the headers currently live at `../../inc/metal-cpp` (outside the repo). Either vendor them into `third_party/metal-cpp/` or add them as a git submodule. They are open-source (Apache 2.0, from Apple)
4. **Create `.gitignore`** — exclude `*.o`, `lps-server`, `readOut.dat`, `.DS_Store`, build artifacts
5. **Create `README.md`** — project description, build instructions, hardware requirements (Apple Silicon), how to run
6. **Init git and push to GitHub**

---

## Phase 2 — C++ Headless API Server

**Goal:** Replace Cocoa GUI loop with an HTTP server; simulation becomes a REST-callable background job.

The current app drives the simulation inside `MTKView`'s `drawInMTKView` callback. We need to decouple this:

1. **Make simulation headless** — Metal compute does not need `NSApp` or `MTKView`. Extract the `Renderer` compute pipeline into a standalone `SimulationEngine` class that just needs an `MTL::Device` and a command queue. No window, no draw loop.
2. **Add cpp-httplib** — header-only HTTP/1.1 library, zero dependencies, works natively on macOS. Drop `httplib.h` into `third_party/`.
3. **Simulation job model** — each simulation run gets a UUID, runs on a background thread, writes progress + results to an in-memory state map (later to DB)
4. **REST endpoints:**
   ```
   POST   /api/simulations          — create & enqueue a sim (body: config JSON)
   GET    /api/simulations          — list all sims (status, id, timestamps)
   GET    /api/simulations/:id      — status + results for one sim
   DELETE /api/simulations/:id      — cancel a running sim
   POST   /api/simulations/:id/fft  — trigger FFT computation on results
   GET    /api/simulations/:id/fft  — retrieve FFT result
   ```
5. **Config schema** — replace all hardcoded simulation parameters (material, grid size, excitation type, frequency, crack location) with a JSON input struct
6. **Streaming progress** — use Server-Sent Events (SSE) or polling endpoint so the frontend can track a running simulation in real time

---

## Phase 3 — PostgreSQL Integration

**Goal:** Persist simulation configs, results, and history.

1. **Add `libpqxx`** (C++ PostgreSQL client) as a dependency — installable via Homebrew
2. **Database schema:**
   ```sql
   simulations (id UUID PK, config JSONB, status TEXT, created_at, finished_at)
   results     (id UUID PK, simulation_id FK, displacement_data BYTEA, sample_count INT)
   fft_results (id UUID PK, simulation_id FK, frequencies BYTEA, magnitudes BYTEA)
   ```
3. **Replace file output** — instead of writing to `readOut.dat`, flush displacement samples to the `results` table as the simulation completes
4. **Connection pooling** — simple pool of libpqxx connections (simulation runs are async, multiple could queue)
5. **Migrations** — plain SQL files in `db/migrations/` run on startup

---

## Phase 4 — Docker Compose Setup

**Goal:** One-command launch of supporting infrastructure; C++ binary runs natively alongside.

```
docker-compose.yml
├── postgres       — PostgreSQL 16, volume-mounted data dir
└── frontend       — nginx serving the built React app, proxies /api/* to host.docker.internal:8080

start.sh           — builds C++ binary natively, runs docker-compose up, then starts lps-server
```

The C++ server listens on `localhost:8080`. nginx in Docker forwards `/api/*` to `host.docker.internal:8080` (the macOS host). This avoids CORS issues and gives the frontend a single origin.

A `Dockerfile.dev` for the frontend dev server (Vite HMR) is separate from the production nginx image.

---

## Phase 4.5 — Live Simulation Visualization

**Goal:** Let users watch the displacement field evolve in real time, both in the browser and optionally as a native window.

Two modes run in parallel:

**Mode A — WebSocket frame streaming (primary)**
The C++ server reads back the Metal texture after each compute step, encodes it as a compact float array or PNG, and pushes it over a WebSocket connection. The React frontend renders each frame onto a `<canvas>` element using the same jet colormap. This is the default live-view experience inside the web UI.

- Texture readback is already done for file output — same mechanism, different sink
- Downsample the 8002×22 grid for streaming (e.g. 1:4) to keep bandwidth reasonable
- Frontend canvas updates at ~10–30 fps depending on sim speed

**Mode B — Native popup window (optional)**
The existing Cocoa/MTKView rendering pipeline is preserved as an opt-in mode. A `"show_window": true` flag in the simulation request config opens a native `NSWindow` with the full-resolution displacement field visualization. The window closes when the simulation finishes. Useful for local power-user sessions.

Both modes can be active simultaneously.

---

## Phase 5 — React Frontend

**Goal:** Usable UI for defining, running, and reviewing simulations.

**Stack:** Vite + React 18 + TypeScript + TailwindCSS + shadcn/ui components

**Libraries:**
- **Recharts** — time-domain displacement plots, FFT frequency spectrum
- **react-three-fiber + drei** — 3D geometry viewer (material block, crack position, transducer location)
- **React Query** — data fetching, polling for sim status, cache invalidation
- **React Hook Form + Zod** — simulation parameter forms with validation

**Pages / Views:**

| View | Features |
|---|---|
| **New Simulation** | Parameter form: material (E, ρ, ν), grid dimensions, crack config, excitation type + frequency, time steps |
| **Live Run** | Progress bar (steps completed / total), real-time displacement waveform updating as sim runs |
| **Results** | Time-domain plot, FFT spectrum, 2D heatmap of displacement field across grid |
| **Live View** | Canvas element fed by WebSocket stream; jet colormap, frame counter, pause/resume |
| **3D Geometry** | Interactive Three.js view: material block, crack as a colored region, source/monitor points |
| **History** | Table of past simulations, filter by status/date, click to re-open results or clone config |
| **Saved Configs** | Save/load named parameter sets to/from DB |

---

## Phase 6 — FFT & Server-Side Analysis

**Goal:** Compute frequency content of simulation output without needing the frontend to do it.

1. **Use Apple Accelerate / vDSP** — already available on macOS, no new dependency; `vDSP_fft_zrip` for real FFT. Alternatively FFTW3 via Homebrew for portability.
2. **FFT endpoint** — takes `simulation_id`, reads displacement array from DB, computes FFT, stores frequencies + magnitudes, returns them
3. **Window functions** — Hanning window (already used in excitation generation) applied before FFT
4. **Peak detection** — optional: find dominant frequencies and return them annotated in the response

---

## Sequencing & Rough Effort

```
Phase 1    — Repo cleanup                ~1 day     (mostly mechanical)
Phase 2    — C++ API server              ~4–5 days  (biggest architectural change)
Phase 3    — PostgreSQL                  ~2 days
Phase 4    — Docker Compose              ~1 day
Phase 4.5  — Live visualization          ~2 days    (WebSocket streaming + optional native window)
Phase 5    — React frontend              ~5–7 days  (most lines of code)
Phase 6    — FFT analysis                ~1–2 days
```

Total estimate: ~2–3 weeks of focused work. Phases 1–4 can be done sequentially before touching the frontend. Phase 6 can be done in parallel with Phase 5 once the API shape is stable.
