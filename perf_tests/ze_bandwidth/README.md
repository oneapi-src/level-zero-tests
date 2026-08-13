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


# Getting stable numbers

* Each device's PCI address and NUMA node are printed at start-up. Per-device
  figures under `-d` frequently split along the topology, and knowing which
  devices share a socket or an upstream port tells you whether a slow device is
  actually slow or merely sharing a saturated path with its neighbour. Compare
  each device alone against pairs before reading a split as a device
  difference. Note that `numactl` does not reach host allocations made through
  `zeMemAllocHost`, so it is not a reliable way to test placement effects.
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
