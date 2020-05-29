# Sysman CLI

`zesysman` provides a command-line interface to oneAPI Level Zero System Resource Management services. It supports reading and modifying GPU settings. (The ability to modify settings depends on system privileges.) It is closely tied to the oneAPI Level Zero System Resource Management library: consult the current [oneAPI Level Zero Specification][1] to understand important concepts.

To list the current options supported, use the `--help` option:

```bash
% zesysman --help
```

## Specifying devices

By default, `zesysman` operates on all devices in the system controlled by an L0 driver. `--device` limits operations to specific devices selected by device index or UUID and `--driver` similarly limits operations to devices supported by a given driver. The available devices and drivers can be listed as follows:

```bash
% zesysman --device list
```
```bash
% zesysman --driver list
```

## Queries

The following options query status of various system management components:

Option                               | Feature Group
-------------------------------------|------------------------
`-t`, `--show-temp`                  | [Temperature][2]
`-p`, `--show-power`                 | [Power][3]
`-c`, `--show-freq`, `--show-clocks` | [Frequency][4]
`-u`, `--show-util`                  | [Utilization][5]
`-m`, `--show-mem`                   | [Memory modules][6]
`-x`, `--show-pci`                   | [PCI][7]
`--show-fabric-ports`                | [Fabric ports][8]
`-b`, `--show-standby`               | [Standby domains][9]
`-e`, `--show-errors`                | [Errors][10]
`-a`, `--show-all`                   | *all of the above*
*Not included in `--show-all`:*      |
`--show-device`                      | [Device attributes][11]
`--show-processes`                   | [Process usage][12]
`--show-scheduler`                   | [Scheduler mode][13]

By default, *telemetry* data is reported (as applicable) for the specified component. This is variable/dynamic data such as current temperature or actual frequency. To report *inventory* data instead, specify `--show-inventory`. This is fixed/configured data such as vendor information or current frequency limits. To show both, specify both `--show-inventory` and `--show-telemetry`:

Option                   | Attribute Type
-------------------------|-----------------
`-i`, `--show-inventory` | fixed/configured
`-l`, `--show-telemetry` | variable/dynamic

For device/power/frequency/PCI inventory and frequency telemetry, `--verbose` requests additional data be shown.

*Note that not all oneAPI Level Zero System Resource Management features are supported by any given driver, device, or system configuration. If a request to access a oneAPI Level Zero System Resource Management feature returns an UNSUPPORTED indication, the tool skips it.*

## Polling

*Telemetry* data can be polled by specifying the number of `--iterations` to perform (setting `--iterations` to `0` or specifying only a `--poll` time requests polling until stopped by keyboard interrupt). Some telemetry data (e.g., bandwidth) requires at least two reads. When requesting such data, the initial report is delayed by the current `--poll` time *(which defaults to 1 second)*.

Option           | Polling Parameter
-----------------|------------------------------
`--poll TIME`    | Time between polls in seconds
`--iterations N` | Number of times to poll

## Output formats

By default, queries are returned in *list* format. When requested, they can be output in *XML*, *table*, or *CSV* format. If an output filename ending in `.xml` or `.csv` is specified via `--output`, the corresponding format is selected by default.

Option                       | Output Control
-----------------------------|------------------------------
`-f list`, `--format list`   | use list output format
`-f xml`, `--format xml`     | use XML output format
`-f table`, `--format table` | use table output format
`-f csv`, `--format csv`     | use CSV table output format
`--output FILE`              | output to FILE
`--tee`                      | print to standard output also

Here are examples of all four formats:

```bash
% zesysman --show-freq
Device 0
    UUID                       : 00000000-0000-0000-0000-3ea500008086
    FrequencyDomains
        GPU
            RequestedFrequency : 300.0 MHz
            ActualFrequency    : 300.0 MHz
            ThrottleReasons    : Not throttled
```
```
% zesysman --show-freq --format xml
<?xml version="1.0" ?>
<Devices>
    <Device Index="0" UUID="00000000-0000-0000-0000-3ea500008086">
        <FrequencyDomains>
            <FrequencyDomain Index="0" Name="GPU">
                <RequestedFrequency Units="MHz">300.0</RequestedFrequency>
                <ActualFrequency Units="MHz">300.0</ActualFrequency>
                <ThrottleReasons>Not throttled</ThrottleReasons>
            </FrequencyDomain>
        </FrequencyDomains>
    </Device>
</Devices>
```
```
% zesysman --show-freq --format table
+-------+--------------------------+-----------------------+---------------------------+
| Index | Freq[GPU].Requested[MHz] | Freq[GPU].Actual[MHz] | Freq[GPU].ThrottleReasons |
|=======|==========================|=======================|===========================|
| 0     | 300.0                    | 300.0                 | Not throttled             |
+-------+--------------------------+-----------------------+---------------------------+
```
```
% zesysman --show-freq --format csv
Index,Freq[GPU].Requested[MHz],Freq[GPU].Actual[MHz],Freq[GPU].ThrottleReasons
0,300.0,300.0,Not throttled
```

There are additional controls to fine-tune specific output formats:

Option              | Output Control
--------------------|----------------------------------------
`--indent COUNT`    | indent by `COUNT` spaces in list format
`--indent -COUNT`   | indent by `COUNT` tabs in list format
`--style condensed` | do not line up colons in list format
`--style aligned`   | line up colons in list format 
`--uuid-index`      | use UUID as index for table formats
`--ascii`           | do not use Unicode degree sign

## Controls

The tool allows various System Manager settings to be set as well. The process must have the appropriate privileges to change these:

Option                     | Sets
---------------------------|--------------------------------
`--set-power POW [TAU]`    | [sustained power limit][3]
`--set-burst-power POW`    | [burst power limit][3]
`--set-peak-power AC [DC]` | [peak power limit][3]
`--set-freq MIN [MAX]`     | [frequency limits][4]
`--set-standby enabled`    | [enables standby promotion][9]
`--set-standby disabled`   | [disables standby promotion][9]
`--set-scheduler MODE`     | [scheduler timeout mode][13]

for `--set-scheduler`, `MODE` may be one of the following:

MODE                    | Specifies
------------------------|-----------------------------------------
`timeout [TIME]`        | timeout mode, value
`timeslice [INT] [YLD]` | timeslice mode, interval, yield interval
`exclusive`             | exclusive mode

Values specified to `--set` operations can be specified with units (e.g., `ms`, `MHz`) and values are scaled to match the units used by the fields.

The tool includes some reset support support as well, if supported by the underlying device and driver:

Option           | Resets
-----------------|-----------------------
`--clear-errors` | [error counters][10]
`--reset`        | [specified device][14]

If `--reset` is used and there is more than one supported device in the system, `--device` must be used to limit operation to a single device. The tool will ask for confirmation unless you also specify `-y` or `--yes` on the command line.

[1]: https://spec.oneapi.com/versions/0.7/oneL0/tools/SYSMAN.html
[2]: https://spec.oneapi.com/versions/0.7/oneL0/tools/SYSMAN.html#querying-temperature
[3]: https://spec.oneapi.com/versions/0.7/oneL0/tools/SYSMAN.html#operations-on-power-domains
[4]: https://spec.oneapi.com/versions/0.7/oneL0/tools/SYSMAN.html#operations-on-frequency-domains
[5]: https://spec.oneapi.com/versions/0.7/oneL0/tools/SYSMAN.html#operations-on-engine-groups
[6]: https://spec.oneapi.com/versions/0.7/oneL0/tools/SYSMAN.html#querying-memory-modules
[7]: https://spec.oneapi.com/versions/0.7/oneL0/tools/SYSMAN.html#pci-link-operations
[8]: https://spec.oneapi.com/versions/0.7/oneL0/tools/SYSMAN.html#operations-on-fabric-ports
[9]: https://spec.oneapi.com/versions/0.7/oneL0/tools/SYSMAN.html#operations-on-standby-domains
[10]: https://spec.oneapi.com/versions/0.7/oneL0/tools/SYSMAN.html#querying-ras-errors
[11]: https://spec.oneapi.com/versions/0.7/oneL0/tools/SYSMAN.html#device-properties
[12]: https://spec.oneapi.com/versions/0.7/oneL0/tools/SYSMAN.html#host-processes
[13]: https://spec.oneapi.com/versions/0.7/oneL0/tools/SYSMAN.html#scheduler-operations
[14]: https://spec.oneapi.com/versions/0.7/oneL0/tools/SYSMAN.html#device-reset
