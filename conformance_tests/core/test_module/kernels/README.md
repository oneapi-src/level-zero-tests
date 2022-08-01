# How to Build CL Modules with Function Pointers:
* clang -c -emit-llvm -target spir64 -o module_name.bc module_name.cl
* llvm-spirv module_name.bc -o module_name.spv --spirv-ext=+SPV_INTEL_function_pointers