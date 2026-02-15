# Algorithm Implementations Benchmarking - GPU (Language: Vulkan) vs CPU ()

A set of implementations to compare optimised CPU and GPU performance. Where possible, key functions are shared in both implementations.

For more context see [Background.md](Background.md).

Algorithms:
- 3D Matrix Multiplication (Batch)
  - TODO: experiment with params to align with hardware size/limits read via vulkan
- TODO: Multi-Scalar Multiplication (MSM) Pippenger algorithm

## Performance Results

### MatMul on old gpu:
- **Optimised CPU Time**: ~4.5 ms
- **Vulkan GPU Time**: ~2 ms (2.25x Speedup)

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

1. **Configure and Build**:
   From the project's base directory, run:
   ```bash
   cmake .
   cmake make
   ```
   This will compile the compute shaders into SPIR-V and build the main executable.

2. **Run**:
   ```bash
   ./benchmark [options]
   ```

3. **Clean build artifacts**:
   To clean the build directory but keep the configuration:
   ```bash
   make clean
   ```

## Development

### Code Formatting

To format the source code using `clang-format`:

```bash
make format
```

### CLI Parameters

The benchmark supports the following options:

- `--matmul`: Run only the Matrix Multiplication benchmark.
- `--msm`: Run only the Multi-Scalar Multiplication (MSM) benchmark.
- `--all`: Run all benchmarks (default).

Example:
```bash
./benchmark --matmul
```

## Configuration

You can adjust dimensions in [common.h](src/common.h):
- `BATCH`: Number of matrices to multiply.
- `M`, `K`, `N`: Dimensions for the multiplication `(M x K) * (K x N)`.
- `MSM_POINTS`: Number of points for the MSM benchmark.
