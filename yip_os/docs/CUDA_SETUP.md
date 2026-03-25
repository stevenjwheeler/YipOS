# Translation (INTRP) and CUDA Setup

YipOS includes an offline neural machine translation feature (the INTRP program) powered by CTranslate2 and NLLB-200. Three installer variants are available depending on your GPU setup and download preferences.

## Installer Variants

| Installer | Translation | GPU Acceleration | Size |
|-----------|------------|-----------------|------|
| **YipOS Setup.exe** | CPU only | No | ~80 MB |
| **YipOS Setup (CUDA Lite).exe** | GPU (user supplies cuBLAS) | Yes | ~80 MB |
| **YipOS Setup (CUDA).exe** | GPU (cuBLAS bundled) | Yes | ~700 MB |

All three variants support the INTRP program and all languages. The difference is inference speed.

## Which Should I Choose?

- **No NVIDIA GPU, or don't care about translation speed**: Standard installer. Translation works on CPU.
- **NVIDIA GPU + you already have the CUDA Toolkit installed**: CUDA Lite. Smaller download, same GPU performance.
- **NVIDIA GPU + you want it to just work**: CUDA Full. Larger download, but everything is included.

## CUDA Lite: Obtaining cuBLAS

The CUDA Lite installer includes a CUDA-enabled translation engine but does **not** bundle the NVIDIA cuBLAS libraries (~600 MB). You must have these available on your system, or the INTRP program will not work.

### Option 1: Install the CUDA Toolkit

1. Go to the [NVIDIA CUDA Toolkit download page](https://developer.nvidia.com/cuda-toolkit-archive)
2. Download **CUDA Toolkit 12.x** (any 12.x version)
3. Run the installer. Choose **Custom** and install only:
   - **CUDA > Runtime > Libraries** (includes cuBLAS)
   - Uncheck everything else to save space
4. Restart YipOS

The full toolkit is ~3 GB, but a minimal install with just the runtime libraries is much smaller. CTranslate2 will find cuBLAS on your system PATH automatically.

### Option 2: Copy DLLs Manually

If you'd rather not install the full toolkit, copy these three files from a CUDA 12.x installation into your YipOS install directory (e.g. `C:\Program Files\YipOS\`):

- `cublas64_12.dll` (~100 MB)
- `cublasLt64_12.dll` (~500 MB)
- `cudart64_12.dll` (~1 MB)

These are typically found in `C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.x\bin\`.

## Requirements

- **Standard installer**: No additional requirements.
- **CUDA Lite / Full**: NVIDIA GPU (GTX 900 series or newer) with up-to-date drivers.

## Verifying GPU Translation

1. Launch YipOS
2. Go to the **INTRP** tab
3. Check the **Log** tab for messages about the CTranslate2 backend
