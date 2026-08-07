# test_memory_overcommit

Device memory overcommit related test content - Level Zero Specification reference: [Memory](https://oneapi-src.github.io/level-zero-spec/level-zero/latest/core/api.html#memory)

Verifies that allocating more device memory than the device physically has lets the
device page memory on and off the device without corrupting it. A memory pattern test
is used to detect corruption.
