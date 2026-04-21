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

## APIs Tested
- `zeDeviceImportExternalSemaphoreExt`
- `zeDeviceReleaseExternalSemaphoreExt`
- `zeMemAllocDevice` (via `lzt::allocate_device_memory`) with `ze_external_memory_import_win32_handle_t`
