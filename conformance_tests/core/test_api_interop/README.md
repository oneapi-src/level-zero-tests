# test_api_interop

## Description
test_api_interop is a conformance test which validates oneAPI Level Zero driver interoperability features with DirectX APIs as described in https://oneapi-src.github.io/level-zero-spec/level-zero/latest/core/PROG.html#interoperability-with-other-apis.

All tests are Windows-only and are skipped on Linux.

The test fixture setup matches the Level Zero device to the corresponding DXGI adapter by device ID before creating the DirectX device.

## Tests

### DX11 Interoperability (`DX11InteroperabilityTests`)

| Test | Description |
|------|-------------|
| `GivenDX11SharedFenceWhenImportingExternalSemaphoreThenIsSuccess` | Creates a shared DX11 fence (`ID3D11Fence`), then imports it into Level Zero as an external semaphore via `zeDeviceImportExternalSemaphoreExt` with the `ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D11_FENCE` flag. Verifies the returned handle is non-null and releases it with `zeDeviceReleaseExternalSemaphoreExt`. |

### DX12 Interoperability (`DX12InteroperabilityTests`)

| Test | Description |
|------|-------------|
| `GivenDX12SharedFenceWhenImportingExternalSemaphoreThenIsSuccess` | Creates a shared DX12 fence (`ID3D12Fence`), then imports it into Level Zero as an external semaphore via `zeDeviceImportExternalSemaphoreExt` with the `ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D12_FENCE` flag. Verifies the returned handle is non-null and releases it. |
| `GivenDX12SharedHeapWhenImportingThenValuesAreCorrect` | Creates a shared DX12 heap (`ID3D12Heap`) and imports it into Level Zero device memory using `ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_HEAP`. Writes a known pattern into the heap via DX12 commands, then copies the data to host memory via Level Zero and verifies the values are correct. |
| `GivenDX12SharedCommittedResourceWhenImportingThenValuesAreCorrect` | Creates a shared DX12 committed resource (`ID3D12Resource`) and imports it into Level Zero device memory using `ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE`. Writes a known pattern via DX12 commands, then copies the data to host memory via Level Zero and verifies the values are correct. |
| `GivenDX12SharedFenceAndImportedBufferWhenPingPongingKernelResultsAcrossFramesThenValuesAreCorrect` | Ping-pongs one shared fence across 16 frames. Each frame DX12 writes the frame index into the imported committed resource and signals; Level Zero waits on the fence, launches the `double_values` kernel over the imported memory and signals back; DX12 waits, reads the resource back and asserts every element equals `frame * 2`. The fence is imported once and reused for all frames, and each frame is drained on the host before the next begins. |
| `GivenDX12SharedFenceAndImportedBufferWhenPingPongingCopyResultsAcrossFramesThenValuesAreCorrect` | As above, but the per-frame Level Zero work op is `append_memory_copy` from the imported memory to host memory rather than a kernel launch. Asserts the copied values on the Level Zero side and, since a copy leaves the source untouched, that DX12's readback still equals `frame`. |

Both ping-pong cases exercise the two gaps the one-shot cases above do not: reuse of a
shared fence across frames, and Level Zero kernel execution behind that fence.

## Kernels
- `interop_double.cl` — `double_values`, doubles each `uint` of the imported buffer in
  place. Written for this suite; the checked-in `interop_double.spv` is what
  `add_lzt_test(... KERNELS ...)` installs, regenerate it with
  `ocloc -file interop_double.cl -spv_only -output_no_suffix -options "-cl-std=CL3.0"`.

## APIs Tested
- `zeDeviceImportExternalSemaphoreExt`
- `zeDeviceReleaseExternalSemaphoreExt`
- `zeCommandListAppendWaitExternalSemaphoreExt` / `zeCommandListAppendSignalExternalSemaphoreExt`
- `zeMemAllocDevice` (via `lzt::allocate_device_memory`) with `ze_external_memory_import_win32_handle_t`
