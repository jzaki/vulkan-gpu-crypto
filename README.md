# 3D Matrix Multiplication Benchmark (Vulkan vs CPU)

A minimal benchmark to compare CPU and GPU performance for 3-dimensional (Batch) Matrix Multiplication. Both implementations share the same indexing logic for consistency.

## Performance Result

On this environment:
- **CPU Time**: ~60-70 ms
- **Vulkan GPU Time**: ~2 ms (~30x Speedup)

## Features

- **Shared Logic**: CPU (`cpu_matmul.cpp`) and GPU (`vulkan_matmul.comp`) share identical indexing macros in `common.h`.
- **Vulkan Compute**: Uses raw Vulkan API for GPU acceleration with a compute shader.
- **Verification**: Built-in verification compares GPU results against CPU results with a floating-point tolerance.
- **Minimal Boilerplate**: Focused on the core math and Vulkan compute setup.

## Project Structure

- `common.h`: Shared definitions and indexing macros (`IX3D`).
- `main.cpp`: Main benchmark driver, Vulkan host code, and verification.
- `cpu_matmul.cpp`: Sequential CPU implementation.
- `vulkan_matmul.comp`: GLSL compute shader.
- `vulkan_helper.h`: Minimal Vulkan initialization helpers.
- `CMakeLists.txt`: Build configuration.
- `Makefile`: Legacy build instructions.

## Prerequisites

- C++ Compiler (GCC/G++ or Clang)
- CMake (3.10+)
- Vulkan SDK (Headers and Loader)
- `glslangValidator` (to compile the shader)

## Build and Run

1. **Build the benchmark**:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```
   This will compile the compute shaders into SPIR-V and build the main executable.

2. **Run**:
   ```bash
   ./benchmark [options]
   ```

### CLI Parameters

The benchmark supports the following options:

- `--matmul`: Run only the Matrix Multiplication benchmark.
- `--msm`: Run only the Multi-Scalar Multiplication (MSM) benchmark.
- `--all`: Run all benchmarks (default).

Example:
```bash
./build/benchmark --matmul
```

## Configuration

You can adjust dimensions in [common.h](src/common.h):
- `BATCH`: Number of matrices to multiply.
- `M`, `K`, `N`: Dimensions for the multiplication `(M x K) * (K x N)`.
- `MSM_POINTS`: Number of points for the MSM benchmark.
