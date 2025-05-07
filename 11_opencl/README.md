
[NVIDIA Redist](https://developer.download.nvidia.com/compute/cuda/redist/cuda_opencl/windows-x86_64/)

[Khronos SDK](https://github.com/KhronosGroup/OpenCL-SDK)

```
cmake .. `
  -DOpenCL_INCLUDE_DIR="C:/Program Files (x86)/Intel/oneAPI/compiler/latest/include" `
  -DOpenCL_LIBRARY="C:/Program Files (x86)/Intel/oneAPI/compiler/latest/lib/OpenCL.lib"
```