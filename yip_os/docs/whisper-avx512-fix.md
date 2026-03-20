# Whisper CPU Backend Crash on Consumer PCs

## Problem

Closed Captions (whisper.cpp) failed to load on consumer gaming PCs (tested on
i9-13900HX and a friend's machine), while working fine on the dev machine.  Both
the Vulkan GPU path **and** the CPU fallback path failed silently — the SEH crash
handler caught the exceptions and logged only a generic "init failed" message.

## Root Cause

**`GGML_NATIVE=ON`** (ggml's default) tells the build system to compile the CPU
backend using whatever instruction set the *build machine* supports.  Our Windows
dev machine has AVX-512, so the CPU backend was compiled with `/arch:AVX512`.

Consumer 12th/13th/14th-gen Intel CPUs (Alder Lake, Raptor Lake, etc.) have
AVX-512 **disabled** because the E-cores don't support it.  When the whisper CPU
backend executed an AVX-512 instruction on these CPUs, it threw an illegal
instruction exception (`STATUS_ILLEGAL_INSTRUCTION`).

Crucially, whisper.cpp **always** initializes the CPU backend — even when using
GPU acceleration — so the AVX-512 crash killed both the GPU and CPU code paths.
The SEH handler (`TryInitWhisperSEH`) caught the structured exception silently,
making it look like a mysterious model-loading failure rather than a CPU
compatibility issue.

### How we found it

1. Added SEH exception code logging to `TryInitWhisperSEH` (the exception code
   would have been `0xC000001D` — illegal instruction).
2. Compared the `CMakeCache.txt` from the dev build vs a local build on the
   gaming PC:
   - Dev machine: `HAS_AVX512_1_EXITCODE:INTERNAL=0` (AVX-512 supported)
   - Gaming PC:   `HAS_AVX512_1_EXITCODE:INTERNAL=FAILED_TO_RUN` (not supported)
3. Built locally on the gaming PC with the same source — worked immediately,
   because `GGML_NATIVE=ON` detected AVX2 (not AVX-512) on that machine.

## Fix

In `yip_os/CMakeLists.txt`, explicitly disable native CPU detection and target
AVX2/FMA/F16C — a portable baseline supported by all x86-64 CPUs from ~2013
onward (Intel Haswell / AMD Excavator and newer):

```cmake
set(GGML_NATIVE OFF CACHE BOOL "" FORCE)
set(GGML_AVX    ON  CACHE BOOL "" FORCE)
set(GGML_AVX2   ON  CACHE BOOL "" FORCE)
set(GGML_FMA    ON  CACHE BOOL "" FORCE)
set(GGML_F16C   ON  CACHE BOOL "" FORCE)
```

Also improved the SEH handler to log the exception code, so future crashes won't
be swallowed silently.

## Files Changed

- `yip_os/CMakeLists.txt` — added `GGML_NATIVE OFF` and explicit AVX2 flags
- `yip_os/src/audio/WhisperWorker.cpp` — SEH exception code logging

## Testing

- Verified on i9-13900HX (13th gen, no AVX-512): model loads and transcribes
- Both GPU (Vulkan on RTX 4090) and CPU paths work correctly
- The fix is also safe for the dev machine (AVX2 is a subset of AVX-512)
