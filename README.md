# LPS — LISA Parallel Solver

GPU-accelerated elastic wave simulation on Apple Silicon using the **Local Interaction Simulation Approach (LISA)**. Solves 2D SH (Shear Horizontal) wave propagation in cracked elastic specimens, with nonlinear crack hysteresis modelling.

## Requirements

- macOS with Apple Silicon (M1/M2/M3) or Intel Mac with Metal support
- Xcode Command Line Tools (`xcode-select --install`)
- clang++ with C++17

## Build

```bash
make           # release build → ./lps-server
make DEBUG=1   # debug build with symbols
make ASAN=1    # build with AddressSanitizer
make clean     # remove object files
make cleanExe  # remove binary
```

## Run

```bash
./lps-server
```

The simulation window opens automatically. Output displacement data is written to `readOut.dat` in the working directory.

## Project Structure

```
src/
  main.cpp                  — entry point (Cocoa app setup)
  Renderer/                 — Metal compute & render pipeline
  MyAppDelegate/            — Cocoa app delegate
  MyMTKViewDelegate/        — MTKView draw loop
  FileReader/               — shader loading, file I/O
  MathHelpers/              — SIMD math, excitation signal generation
shaders/
  computeCrackElastic.metal — active compute kernel (elastic wave + crack)
  computeCrack.metal        — alternative crack variant
  computeNonlinear.metal    — nonlinear variant
  triangle.metal            — vertex/fragment shaders (jet colormap display)
```

## Simulation Parameters

All parameters are currently defined in the compute shader (`shaders/computeCrackElastic.metal`):

| Parameter | Value | Description |
|---|---|---|
| Grid | 8002 × 22 | Spatial domain (x × y nodes) |
| dx, dy | 1×10⁻⁴ m | Node spacing |
| dt | 1×10⁻⁸ s | Time step |
| Material | Aluminium | E=70 GPa, ρ=2700 kg/m³, ν=0.33 |
| Excitation | 175 kHz | Single sinusoid, Hanning windowed, 10 cycles |
| Crack | x ∈ [3750, 4250] | Nonlinear hysteresis zone |

## Roadmap

See [PLAN.md](PLAN.md) for the full development plan — transforming LPS into a self-hosted web platform with a REST API, PostgreSQL storage, live simulation streaming, and a React frontend.
