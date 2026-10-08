# Architectural & Design Decisions

This document outlines the architectural decisions, constraints, and rationales implemented in the **CPU Load Monitoring Application**.

---

## 1. Compliance with Constraints & Requirements

### 1.1 State Machine (Init and Run States)
* **Init State:** 
  * Parses command-line arguments via POSIX `getopt()`.
  * Pre-opens `/proc/stat` and target logging file descriptors.
  * Pre-allocates fixed-size static buffers and core tracking data arrays (`prev_ticks_`, `current_ticks_`).
  * Gathers baseline ticks and automatically transitions to the `Run` state.
* **Run State:**
  * Runs the primary event loop via non-blocking `poll()` on `STDIN_FILENO`.
  * Zero dynamic allocations (`malloc`, `free`, `new`, `delete`, or dynamically resizing containers) are executed during this state.

### 1.2 Zero Dynamic Allocations in Run State
* **Buffer Management:** A static fixed-size buffer of 16 KB is allocated on the stack/object memory during initialization. This is sufficient to read `/proc/stat` for up to 256 logical cores.
* **In-place Parsing:** String parsing relies entirely on `std::string_view` slices and `std::from_chars` (introduced in C++17), operating directly over the static buffer without temporary string copies or heap allocations.
* **Metric Formatting:** Output formatting uses `snprintf()` writing into bounded stack-allocated char buffers.

### 1.3 Pure POSIX-compliant APIs & RAII
* **File Operations:** Standard C++ streams (`std::ifstream`, `std::ofstream`) were intentionally avoided in favor of direct POSIX system calls (`open`, `read`, `lseek`, `write`, `close`). This eliminates internal buffering overhead and implicit dynamic memory allocations.
* **File Descriptors:** Handled using `FdWrapper`, a lightweight RAII wrapper enforcing resource safety and closing open descriptors automatically.
* **Signal Handling:** Configured via `sigaction` for `SIGINT` and `SIGTERM` to facilitate graceful shutdowns.

### 1.4 "By Request" Interpretation
* To fulfill requirement #4 (*"Application shall print CPU load for every core by request"*), the application performs non-blocking I/O multiplexing (`poll` on standard input).
* The user presses **[ENTER]** in the terminal to immediately trigger a reading and print the load for each logical core to `stdout`.

### 1.5 Configurable Periodic Logging 
* When `-i <seconds>` and `-o <file_path>` are provided, the application records per-core load snapshots into the specified log file at the specified interval using monotonic timestamps (`clock_gettime(CLOCK_MONOTONIC)`).

---

## 2. Build Instructions

### Building with Bazel
```bash
bazel build //:cpu_monitor
```
Run directly:
```bash
bazel run //:cpu_monitor -- -i 2 -o cpu_metrics.log
```
### Building with CMake
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
./cpu_monitor -i 2 -o cpu_metrics.log
```
---
## 3. Verification & Memory Profiling
The binary can be verified with Valgrind to ensure zero memory leaks and no heap allocations in the Run state:
```bash
valgrind --tool=memcheck --leak-check=full ./cpu_monitor
```