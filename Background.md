# Optimisation of client-side proving for consumer gpus

GPUs are being used for their compute capabilities in LLMs and increasing use in zero-knowledge proving/verifying.

Cloud services seek the latest, most performant gpus to serve users, and algorithms are adapted to get the most out the high-specification hardware. Outsourcing proving to incentivised servers brings about better ux at a cost to decentralisation.

Client-side proving enables privacy, sovereignty and maximum decentralisation. PSE articulates a gap in the space (Jan 2026):  "- **The Gap**: No standard cryptographic library exists for client-side GPU implementations. Projects reinvent primitives from scratch, and best practices for mobile-specific constraints (hybrid CPU-GPU coordination, thermal management) remain unexplored." - [link](https://pse.dev/blog/client-side-gpu-everyday-ef-privacy)

Various efforts around this space target specific hardware with their libraries for their applications (see ref), some are not yet open source. So I agree that there is important work to be done optimising algorithms towards consumer grade gpus from *all* vendors.

## Areas to consider for faster proving
- Alignment of calculation groups to hardware sizes
	- eg Subgroup (aka warp, SIMD lanes, ...)
- Use of specific shader operations (ray tracing) for specific calculations
	- eg one clock accumulating multiply of 3 vars
- Number bit sizes and algorithm options
	- eg in MSM using Pippenger algorithm: windowing for bucketing
- Pre-computation for certain repeat computations

## Why use Vulkan?

Vulkan is a low‑level, cross‑platform graphics and compute API that was standardized by the Khronos Group in February 2016 (the first version, 1.0). It is designed to give developers much finer control over GPU hardware than older APIs such as OpenGL or Direct3D 11, while still providing a portable abstraction that works on many operating systems and devices.

The alternative is to maintain multiple implementations using Vendor specific languages like: CUDA for NVIDIA, Metal for Apple, ... etc.

### Established and broadly adopted

Earliest support:
- 2014 - NVIDIA
- 2015 - AMD (Radeon HD 7000), Intel (Integrated HD Graphics 530), Arm (Mali-G71)
- 2016 - Qualcomm (Snapdragon 820)
- No native support from Apple, uses MoltenVK as a translation layer

By 2024 virtually every mainstream desktop GPU (AMD, NVIDIA, Intel) and most mobile GPUs (Qualcomm, Samsung Exynos, MediaTek) ship with Vulkan‑compatible drivers.

Vulkan concepts influence next‑generation web graphics: WebGPU (still experimental) and WebAssembly.

## GPU architecture (Vulkan terminology)

### Subgroup

A set of shader invocations that execute in lock‑step on a single SIMD/vector engine.

Vulkan code can use `VkPhysicalDeviceSubgroupProperties.subgroupSize` to more efficiently pipe through computations.

Vendor sizes:
- NVIDIA → warp (usually 32 lanes)
- AMD → wavefront (typically 64 lanes)
- Intel → SIMD lane group (often 8‑16 lanes)
- Apple → SIMD execution unit (size varies)
  
### Compute unit / Shader core

Contains one or more subgroups, registers, shared memory, etc.
Similarly can use `maxComputeWorkGroupInvocations`, `maxComputeSharedMemorySize`, and `maxComputeWorkGroupCount` in Vulkan for efficiency. Not only to maximally use the gpu, but to optionally leave enough headroom for the UI (important for mobile OSs).

- NVIDIA → SM (Streaming Multiprocessor) containing many CUDA cores
- AMD → CU (Compute Unit) containing many stream processors
- Intel → Xe‑core (or EU – Execution Unit)
- Apple → SIMD execution unit within a GPU tile

## Effective use of Subgroup Size
...

## ...

## Appendix
### Other work using gpus in cryptography

Mostly vendor specific code...

Apr 2025 - Using WebGPU abstraction - [link](https://blog.zksecurity.xyz/posts/webgpu/) - mentions various cryptographic improvements.

https://github.com/zkmopro/gpu-acceleration
https://github.com/ICME-Lab/msm-webgpu - Wyatt Benno

https://github.com/mratsim/constantine/blob/master/constantine/math_compiler/impl_msm_nvidia.nim
https://github.com/mratsim/constantine/tree/master/constantine/math_compiler/experimental
https://github.com/mratsim/constantine/blob/master/constantine%2Fplatforms%2Fabis%2Fnvidia_abi.nim#L23

Jolt?

https://prove.wtf 

https://github.com/z-prize
https://github.com/zkmopro/gpu-acceleration
https://x.com/diego_aligned/status/2021225640792412541 - Metal
https://github.com/lambdaclass/lambdaworks

MoPro Rapidsnark vs ICICLE mobile prover, MoPro had smaller proof sizes - MoPro public tg - (GPU channel)