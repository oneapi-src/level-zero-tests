# Description
ze_bandwidth is a performance micro benchmark suite for measuring bandwidth and latency 
for transfers between Host memory and GPU Device memory.

ze_bandwidth measures the following:
* Host->Device Memory transfer bandwidth in GigaBytes Per Second 
* Host->Device Memory transfer latency in microseconds
* Device->Host Memory transfer bandwidth in GigaBytes Per Second
* Device->Host Memory transfer latency in microseconds
* Bidirectional Host<->Device transfer bandwidth, driven either by two copy
  engines, by a copy engine paired with a compute engine, or by a single
  compute kernel that alternates direction internally

# Features
* Configurable range of transfer size measurements
* Configurable number of iterations per transfer size
* Optional user flag enables verification of first and last byte of every transfer
* Engine selection by name (`bcs0`, `ccs0`, ...) per direction
* Optional GPU timestamp based timing
* Compute kernel driven transfers as an alternative to copy engine transfers

# How to Run it
To run all benchmarks using the default settings: 
```
Default Settings:
* Both Host->Device and Device->Host memory transfer measurments performed
* Verification option disabled
* iterations per transfer size = 500
* transfer size range:  1 byte up to 2^28 bytes, with doubling in size per test case
* timing taken with the host timer

To use command line option features:
 ze_bandwidth [OPTIONS]

 OPTIONS:
  -t, string               selectively run a particular test:
      h2d or H2D                       run only Host-to-Device tests
      d2h or D2H                       run only Device-to-Host tests
      bidir                            both directions at once, aggregate figure
      bidir_h2d                        both directions, report Host-to-Device only
      bidir_d2h                        both directions, report Device-to-Host only
      h2d_kernel                       Host-to-Device driven by a compute kernel
      d2h_kernel                       Device-to-Host driven by a compute kernel
      bidir_kernel                     both directions through a single kernel
      all_to_host                      Device-to-Host on one device while every
                                       other device interferes, per device
      host_to_all                      Host-to-Device on one device while every
                                       other device interferes, per device
      all_to_host_bidir                as all_to_host, with bidirectional traffic
      host_to_all_bidir                as host_to_all, with bidirectional traffic
      all_to_host_kernel               kernel driven variants of the above;
      host_to_all_kernel               these default to every device, override
      host_to_all_bidir_kernel         with -d
                            [default:  h2d and d2h]
  -v                       enable verification
                            [default:  disabled]
  -i                       set number of iterations per transfer
                            [default:  500]
  -w                       set number of warmup iterations
                            [default:  100]
  -s                       select only one transfer size (bytes)
  -sb                      select beginning transfer size (bytes)
                            [default:  1]
  -se                      select ending transfer size (bytes)
                            [default: 2^28]
  -q                       query for number of engines available
  -d                       comma separated list of devices (default: 0)
  -g, group                select engine group (default: 0)
  -n, number               select engine index (default: 0)
  --h2dEngine name         engine used for the Host-to-Device direction
  --d2hEngine name         engine used for the Device-to-Host direction
  --useEvents              measure with GPU timestamps instead of the host timer
  --hostNuma mode          NUMA placement of the host buffers and of the
                            submitting thread: none (default), auto, or a node id
  --immediate              use immediate command lists (default: disabled)
  --csv                    output in csv format (default: disabled)
  -h, --help               display help message

For example to run a single Host->Device test for transfer_size = 300 bytes, 100 iterations, verification enabled:

 ./ze_bandwidth -t h2d -s 300 -i 100 -v
```

# Which options apply to which test

`-i`, `-w`, `-s`, `-sb`, `-se`, `-d` and `--csv` apply to every test. `-q` lists
the engines and exits without running anything. The rest differ per test:

| Test | `-v` | `--h2dEngine` (`-g`/`-n` first value) | `--d2hEngine` (`-g`/`-n` second value) | `--useEvents` | `--immediate` |
|---|---|---|---|---|---|
| `h2d` | yes | the measured copy | unused | yes | yes |
| `d2h` | yes | unused | the measured copy | yes | yes |
| `bidir` | **no**, silently unverified | the H2D copy | the D2H copy | yes, adds `overlap` | yes |
| `bidir_h2d` | **no**, silently unverified | the measured copy | the interfering copy | yes, adds `coverage` | yes |
| `bidir_d2h` | **no**, silently unverified | the interfering copy | the measured copy | yes, adds `coverage` | yes |
| `h2d_kernel` | yes | the kernel, must be compute | unused | yes | yes |
| `d2h_kernel` | yes | unused | the kernel, must be compute | yes | yes |
| `bidir_kernel` | yes | the kernel, must be compute | must name the same engine | yes | yes |
| `all_to_host`, `all_to_host_kernel` | measured device only | unused | the measured copy | yes | **rejected** |
| `host_to_all`, `host_to_all_kernel` | measured device only | the measured copy | unused | yes | **rejected** |
| `all_to_host_bidir` | measured device only | the opposite direction | the measured copy | yes, adds `coverage` | **rejected** |
| `host_to_all_bidir` | measured device only | the measured copy | the opposite direction | yes, adds `coverage` | **rejected** |
| `host_to_all_bidir_kernel` | measured device only | the kernel, must be compute | must name the same engine | yes | **rejected** |

Notes on the table:

* An engine listed as unused is not selected away from its default, it simply
  carries no traffic in that test, so naming it changes nothing.
* A test that drives one engine accepts either name, so naming only the
  direction it measures is enough. A test that drives two takes each direction
  from its own name, and an unnamed direction keeps the default engine rather
  than following the one that was named.
* Where a test names a compute engine, a copy only engine is rejected rather
  than substituted.
* `-v` sets the iteration count to 1, so a verified run reports a bandwidth
  figure that includes the first-touch cost and should not be compared against
  an unverified one.
* `-v` is accepted but not acted on by `bidir`, `bidir_h2d` and `bidir_d2h`;
  the run prints a note saying so.
* `--useEvents` reports an extra column only where a second timed operation
  exists to compare against. The kernel driven tests are a single dispatch, so
  they gain accuracy from `--useEvents` but no extra column.

# Selecting engines

`-q` lists the queue groups a device exposes together with the engine name of
every queue in them:

```
$ ./ze_bandwidth -q
 Group 0 (compute): 1 queues -> CCS0
 Group 1 (copy):    2 queues -> BCS0, BCS1
```

An engine may then be chosen per direction by name. Both `--h2dEngine bcs0` and
`--h2dEngine=bcs0` are accepted, and a bare class name means the first engine of
that class, so `ccs` is the same as `ccs0`:

```
 ./ze_bandwidth -t bidir_h2d --h2dEngine=bcs0 --d2hEngine=bcs1
 ./ze_bandwidth -t bidir_h2d --h2dEngine=bcs0 --d2hEngine=ccs0
```

The numeric `-g` / `-n` form still works and selects the same engines by queue
group ordinal and queue index; the two forms cannot be mixed in one invocation.
An engine that the device does not expose is rejected, it is not replaced by a
default. On a part with a single copy engine, `--d2hEngine=bcs1` therefore fails
rather than quietly running both directions on `BCS0` and reporting the result as
though two engines had been used.

Every test runs its measured transfer on the engine named for the direction it
measures, so `-t d2h --d2hEngine=bcs1` measures on `BCS1` and `--h2dEngine` has
no effect on it. The banner printed at start-up names the engine each direction
will use; check it against the table above when a result looks unexpected.

Naming assumes that where a device splits copy engines across several queue
groups, the main copy group carries the lower ordinal. Check `-q` before relying
on `bcs1` and above.

The kernel driven tests (`h2d_kernel`, `d2h_kernel`, `bidir_kernel`) need a compute engine,
because a copy only queue group cannot run a kernel.

# Bidirectional tests

Both directions are enqueued behind a single event that the host signals once
everything is submitted, so they are released together and no submission cost
lands inside the measured window.

`bidir` runs both directions at the same transfer size and reports the aggregate.

`bidir_h2d` and `bidir_d2h` report one direction only and run the other as
interfering traffic at **twice** the size, so that the interference outlives the
measured copy. Without that, the interfering copy finishes first and the tail of
the measured copy runs unopposed, which inflates the result while still looking
like a bidirectional measurement.

With `--useEvents` these tests also report how well the two directions actually
overlapped, and warn when they did not:

* `bidir` reports `overlap`, the overlapping fraction of the combined span. It
  should approach 1.0.
* `bidir_h2d` and `bidir_d2h` report `coverage`, the fraction of the measured
  copy that was contended. It should approach 1.0. The combined-span fraction is
  about 0.5 here by construction, because the interfering copy is twice as long,
  so `coverage` rather than `overlap` is the meaningful number.

A low value means the two directions did not run concurrently for the whole
measurement, and the reported bandwidth is not full duplex. The two common causes
are different:

* Both directions were placed on the same engine, in which case they serialise
  completely and the metric reads 0. Two copies on one copy engine, or the
  measured and interfering copy on one engine, both produce this.
* The device has a single path to host memory, as an integrated GPU does. There
  the two directions contend for one memory controller, so a value below the
  threshold is expected rather than a misconfiguration.

The first case is easy to hit by accident, because both directions default to
the same engine. It is therefore reported at start-up, before any measurement
and whether or not `--useEvents` was passed:

```
WARNING: both directions were placed on CCS0, so they serialise and the reported
         figure is not full duplex.
         Name a second engine to measure both directions at once, for example
         --h2dEngine=bcs0 --d2hEngine=ccs0.
```

The run still proceeds, because a deliberate same-engine run is the control that
shows the overlap metric collapsing to 0. A part with one copy engine can still
measure genuine bidirectional traffic by pairing it with the compute engine.

# Kernel driven transfers

`h2d_kernel` and `d2h_kernel` copy with a kernel instead of a copy engine. Each thread
moves several 16 byte elements with a software pipeline of loads issued before
the first store.

`bidir_kernel` uses a single kernel that alternates copy direction across fixed
512 byte stripes, so one dispatch on one queue produces traffic in both
directions at once. This is the only way to obtain bidirectional traffic over the
EUs on a part that exposes a single compute engine.

Notes on interpreting `bidir_kernel`:

* The reported bandwidth is the **aggregate** of both directions; each direction
  carries half of it.
* Only an aggregate figure exists. A single dispatch yields a single pair of
  timestamps, so the two directions cannot be separated.
* Transfer sizes are rounded down to a whole number of stripe pairs, and sizes
  below one stripe pair (1024 bytes) are skipped with a message naming the size.
  The size printed on each result line is the number of bytes actually moved.
* `-v` for this test compares against the stripe pattern rather than expecting a
  plain copy. The kernel is a bandwidth generator: odd stripes move the device
  buffer into the host buffer while even stripes move the host buffer into the
  device buffer, so afterwards both buffers hold the same value in each stripe
  and which of the two fill patterns that is depends on the stripe parity.

The kernel is built from `ze_bandwidth_copy.cl` at run time, which is installed
next to the binary; run the benchmark from its install directory. Building
OpenCL C directly is a driver extension, so on a driver without it the module
creation fails and the build log is printed.

# Multi device host transfers

`all_to_host` and `host_to_all` measure one device at a time while every other
device generates interfering traffic in the same direction, and report the
measured device's bandwidth per device. The interfering copies run at twice the
size so that they outlive the measured copy.

`all_to_host_bidir` and `host_to_all_bidir` add traffic in the opposite
direction, on the measured device and on every interfering device.

The `_kernel` variants drive the same topologies with a compute kernel instead
of a copy engine. `host_to_all_bidir_kernel` uses the split direction kernel,
so each device needs only one queue and one buffer pair, and the reported
figure is the aggregate of both directions.

There is deliberately no `all_to_host_bidir_kernel`. The split direction kernel
is symmetric, so it would have been the same measurement under a second name;
passing that token reports where it went rather than silently aliasing it.

The measured direction runs on the engine named for it, so `all_to_host*` takes
its engine from `--d2hEngine` and `host_to_all*` from `--h2dEngine`. The
bidirectional variants run the opposite direction on the other engine.

These tests use every enumerated device by default. Pass `-d` to restrict them
to a subset; with a single device there is no interfering traffic and the test
degenerates to the corresponding single device measurement, which the output
notes.

With `--useEvents`, `all_to_host_bidir` and `host_to_all_bidir` also report
`coverage`: the fraction of the measured copy during which the opposite
direction on the *same* device was also in flight. It should approach 1.0, and
a low value means the two directions serialised on one engine.

Coverage deliberately says nothing about the interfering traffic on the other
devices. Each device timestamps against its own free running clock, so spans
from different devices cannot be compared and cross device interference cannot
be measured this way. To confirm the interference is biting, compare a run
against the same test with `-d` restricted to one device.

A transfer size is skipped, with a message naming the shortfall, when the
interfering devices would not fit their buffers in device memory. `-v` checks
the measured device only; the interfering devices move data but are not
verified. `--immediate` is not supported by these tests.


# Host NUMA placement

`--hostNuma` decides where the host side of a transfer lives and which CPUs
drive it. Without it nothing changes: the placement is the kernel's, which is
what the tool has always done.

| value | host buffers | submitting thread |
| --- | --- | --- |
| `none` (default) | wherever the kernel puts them | unpinned |
| `auto` | each device's buffers on that device's own NUMA node | pinned to the measured device's node |
| `N` | every buffer on node `N` | pinned to node `N`'s CPUs |

`auto` is the interesting one for the multi device tests: each device's host
buffers go on the node that device hangs off, and `all_to_host` re-pins the
submitting thread to the measured device's node for each row. The other tests
run against the first device in `-d` and pin to that one for the whole run.

A device whose sysfs reports no NUMA node is left unbound, with a warning
naming it. A node that is not online is rejected at the command line.

## Why pinning the CPU is not enough

First-touch puts a page on the node of the CPU that writes it first, so
pinning the submitting thread would place the host buffer too, if that buffer
were ordinary memory. It is not. Measured on a two socket host with 16 GPUs,
writing all 2 GB from the pinned thread:

| 2 GB allocation | thread pinned to node 0 | thread pinned to node 1 |
| --- | --- | --- |
| `mmap` anonymous | node 0 | node 1 |
| `zeMemAllocHost` | node 0 | node 0 |

The driver allocates those pages itself, inside the ioctl, from the node of
the GPU it routes the allocation through, and never from the node of the
calling CPU. Exposing only a node 1 GPU with `ZE_AFFINITY_MASK` moves them to
node 1 whichever CPU asks for them, which is what identifies the device, not
the thread, as what the node follows.

That is the right choice for a context holding one device and the wrong one
for a context holding several, because every host allocation is routed
through the first of them:

| 2 GB host buffer for device 15, which sits on node 1 | served from |
| --- | --- |
| default | node 0, remote |
| `--hostNuma=auto` | node 1, local |

So the pinning and the placement are two separate mechanisms here and only
the second one moves the buffer. This is also what the multi device tests
have been measuring all along: in a 16 device run every host buffer comes
from the first device's node, so each device on the other socket was crossing
the interconnect without saying so. Measured across 16 devices on four nodes,
with every other device interfering:

| `all_to_host_kernel` | socket 0 devices | socket 1 devices | ratio |
| --- | --- | --- | --- |
| default | 0.27 GB/s | 0.10 GB/s | 2.7x |
| `--hostNuma=auto` | 11.29 GB/s | 11.22 GB/s | 1.01x |

The gap is not only between the sockets. Concentrating every buffer on one
node also saturates that node's memory controller, so the figures there are
far below what the devices can do; spreading them raises all of them.

A run that asks for no placement now says when this applies to it, naming the
devices it applies to:

```
NOTE: device 8 9 10 11 12 13 14 15 sit on a NUMA node other than node 0, which
is where device 0 sits and where a driver that serves every host allocation
from the first device in the context places them. Those transfers then cross
the interconnect without the figure saying so; pass --hostNuma=auto to place
each device's host buffers on its own node
```

The note is silent on a single node host, when every selected device shares
the first device's node, and when a placement was asked for. Like every other
commentary line it goes to stderr, so `--csv` output is unaffected.

## How the placement is applied, and when it fails

`auto` and `--hostNuma=N` get there by different routes, because they are not
asking for the same thing.

### `auto`: one context per device

The driver serves a host allocation from the node of the device it routes the
allocation through, and in a context holding several devices that is always
the first of them. `auto` only ever wants each device's own node, so it asks
for exactly that: every device is given a context of its own with
`zeContextCreateEx`, and `zeMemAllocHost` on that context lands on that
device's node. This is core API only, needs no extension, and works for the
kernel driven tests as well, which cannot use an imported pointer at all.

Everything made from a context belongs to it, so this also makes the command
lists, queues, event pools and modules per device. One consequence is visible
in the measurement: a single event cannot gate devices in different contexts,
so each device has its own release gate and the host signals them in a short
loop instead of once. The resulting start skew measured at around 30 us
across 16 devices, against transfers of hundreds of us whose interfering
copies are twice the size, so it does not change what the tests report.

### `--hostNuma=N`: placed here, handed over

A node of the caller's choosing cannot be expressed as a context arrangement,
so the fixed mode places the memory itself. `zeMemAllocHost` takes no node,
and on a driver that answers it from its own device mapping the pages are
outside the kernel's page allocator entirely, so `mbind` on the returned
range is accepted and then ignored, which is also why `numactl` has no effect
on those buffers. The tool therefore allocates anonymous memory, binds and
faults it on the requested node, and hands it to the driver through
`zexDriverImportExternalPointer`.

### Which path a run took

The start-up line says so:

```
Host NUMA placement: mode=auto, host buffers=per-device context, device 0 -> node 2, device 15 -> node 7
```

`host buffers=` is one of:

| value | meaning |
| --- | --- |
| `per-device context` | `auto`, placed by the driver per device. Core API only |
| `imported` | `--hostNuma=N`, placed by the tool and imported |
| `driver allocated` | nothing was placed; a warning gives the reason |

An imported allocation is checked after it is faulted in, and a mismatch
between the node asked for and the node the pages landed on is reported once
per run. A per-device context allocation is not checked, because those pages
are the driver's own mapping and the page allocator does not account for
them: `move_pages` reports no node for a buffer that is in fact placed
correctly.

The `driver allocated` fallback is taken when the driver has no import
extension, or when a `_kernel` test needs the fixed mode, for the reason
below.

### What an imported pointer can and cannot do

This only limits `--hostNuma=N`. Since `auto` no longer imports anything, the
kernel driven tests are placed like every other test.

Intel's [System Memory Allocations in Level Zero][sysmem] states the
constraint directly:

> System allocations, e.g. those created using `new` and `malloc`, are
> currently only supported on copy operations, such as
> `zeCommandListAppendMemoryCopy`. This means that if a GPU kernel requires
> access to a system allocation, it needs first to copy it to a USM
> allocation.

`zexDriverImportExternalPointer` does not lift that restriction, because
lifting it is not what the extension is for. A copy from system memory makes
the driver create an internal graphics allocation to back it, and that
allocation is owned by the command list and is rebuilt whenever the list is
reset. Importing moves its ownership to the driver handle so that it
survives, which is a copy path optimisation and nothing more.

"Copy operation" there means the API, not the hardware. `all_to_host` runs on
a compute engine by default and is unaffected, because it transfers through
`zeCommandListAppendMemoryCopy`; forcing it onto a copy engine with
`--d2hEngine=bcs0` changes nothing either way. What separates the `_kernel`
variants is that they pass the buffer as a kernel argument.

Measured against the driver on the test host:

| | imported | `zeMemAllocHost` |
| --- | --- | --- |
| `zeMemGetAllocProperties` | `type=UNKNOWN` | `type=HOST` |
| `zeMemGetAddressRange` | `ERROR_ADDRESS_NOT_FOUND` | base and size |
| GPU address vs host address | identical | identical |
| `zeCommandListAppendMemoryCopy` over the whole range | yes, data correct | yes |
| kernel reads one byte, anywhere in the range | yes | yes |
| full size kernel dispatch | hangs the compute engine | yes |

The first two rows are the documented position restated by the API: an
imported range is not a USM allocation and is not meant to answer as one. The
pages are still mapped in the GPU's address space at the same address the
host sees, and a transfer reads them correctly end to end, which is the
supported use.

Passing one to a kernel is outside what the driver supports, and it does not
fail cleanly. It works at low occupancy and stops working once a real
dispatch runs against it, appearing in the kernel log as
`Engine reset: engine_class=ccs` and surfacing as
`ZE_RESULT_ERROR_DEVICE_LOST`. The point at which it starts failing depends
on both the dispatch size and the number of distinct pages touched and is not
sharp, so it cannot be avoided by staying under a limit; every dispatch at
the sizes these tests use is well past it.

None of `zeContextMakeMemoryResident`, `zeKernelSetIndirectAccess(HOST)`, or
appending a copy over the same range earlier in the command list changes the
outcome, so this is not a residency problem and cannot be hinted away.

[sysmem]: https://github.com/intel/compute-runtime/blob/master/programmers-guide/SYSTEM_MEMORY_ALLOCATIONS.md

## Reading the numbers

Neither placed path is the same allocation the driver hands out for
`--hostNuma=none`, so comparing against a `none` run mixes a placement change
with an allocation change. To isolate the NUMA effect, compare two placed
runs against each other, for example `--hostNuma=0` against `--hostNuma=1` on
a device that sits on node 0.

Under `--csv` the placement is emitted as a comment line above the column
header, so a stored result still says how it was produced:

```
# host_numa: mode=auto, host buffers=per-device context, device 0 -> node 2
Transfer_size,Bandwidth_(GBPS),Latency_(usec)
```

A default run emits no such line and its output is unchanged.

Whether placement moves a figure at all is a property of the part. Where the
link saturates well below host memory bandwidth, local and remote nodes
measure the same, and that is a result worth having rather than an assumption.


# Getting stable numbers

* Each device's PCI address and NUMA node are printed at start-up. Per-device
  figures under `-d` frequently split along the topology, and knowing which
  devices share a socket or an upstream port tells you whether a slow device is
  actually slow or merely sharing a saturated path with its neighbour. Compare
  each device alone against pairs before reading a split as a device
  difference. Use `--hostNuma` to place the host side deliberately rather than
  inferring it; `numactl` does not reach host allocations made through
  `zeMemAllocHost`.
* On parts whose GPU frequency ramps under load, the default `-w 100` may
  still not be enough and run to run means can differ by more than 1.5x. Use a
  much larger warmup, for example `-w 500`, for any figure that will be
  reported.
* On Linux, set the CPU frequency governor to `performance`. A `powersave`
  governor is frequently the dominant source of run to run variance.
* Pinning away from the CPU core that services the GPU interrupt has been
  measured to matter little for this benchmark: on a PTL part whose `xe`
  interrupt is bound to a single core, running there cost 0.6% at a 128 MB
  transfer and 3.4% at a 4 KB transfer. Transfers of this size are bound by the
  device, not by submission latency. Pin with `taskset` away from the core listed
  in `/proc/interrupts` if chasing small differences, but do not expect it to
  change a bandwidth figure.
* `--useEvents` excludes submission and synchronisation cost, so it is the better
  choice with `--immediate`, where the copy is appended inside the timing loop,
  and for `bidir_h2d` / `bidir_d2h`, where it also reports the coverage.
* `--useEvents` changes the per device figures only. The `[Total` line stays on
  the host timer, because timestamps taken on different devices come from
  different clocks and cannot be combined into one span.
