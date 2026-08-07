# oneAPI Level Zero Negative Tests

Negative test content for validating all features of the oneAPI Level Zero validation
layer, which checks and handles all invalid inputs to the oneAPI Level Zero APIs.

## Environment

* `ZE_ENABLE_VALIDATION_LAYER` - set to `1` to enable the Validation Layer; required to
  get valid results.
* `ZE_ENABLE_PARAMETER_VALIDATION` - set to `1` to enable checking of individual
  arguments to an L0 API. Requires `ZE_ENABLE_VALIDATION_LAYER` to be enabled as well.

See the top-level [BUILD.md](../BUILD.md) for prerequisites, build and run
instructions.
