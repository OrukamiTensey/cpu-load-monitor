# CPU Load Monitoring Application

A POSIX-compliant C++17 command-line utility for monitoring per-core CPU load under Linux. Designed specifically for embedded and automotive systems with strict resource management constraints.

## Features
- **Zero dynamic memory allocations** in the `Run` state (static pre-allocated buffers, `std::string_view`, and `std::from_chars`).
- **POSIX-compliant APIs only** (`open`, `read`, `lseek`, `write`, `poll`, `sigaction`).
- **RAII-managed resources** via custom `FdWrapper`.
- **On-demand inspection**: Press `[ENTER]` to instantly print per-core load percentages to `stdout`.
- **Configurable periodic logging**: Record per-core load snapshots to a file every *N* seconds.
- Dual build system support: **Bazel** and **CMake**.

## Build & Run

### Bazel 
```bash
bazel build //:cpu_monitor
bazel run //:cpu_monitor -- -i 2 -o cpu_metrics.log
```
### CMake
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
./cpu_monitor -i 2 -o cpu_metrics.log
```
## Architecture & Design Decisions
See [DESIGN_DECISIONS.md](DESIGN_DECISIONS.md) for full architectural justifications and compliance details.