# How to Build module_fptr kernels:
* clang -c -emit-llvm -target spir64 -o module_fptr_call.bc module_fptr_call.cl
* llvm-spirv module_fptr_call.bc -o module_fptr_call.spv --spirv-ext=+SPV_INTEL_function_pointers
* clang -c -emit-llvm -target spir64 -o module_fptr_call_kernels.bc module_fptr_call_kernels.cl
* llvm-spirv module_fptr_call_kernels.bc -o module_fptr_call_kernels.spv --spirv-ext=+SPV_INTEL_function_pointers