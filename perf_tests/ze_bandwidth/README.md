# Description
ze_bandwidth is a performance micro benchmark suite for measuring bandwidth and latency 
for transfers between Host memory and GPU Device memory.

ze_bandwidth measures the following:
* Host->Device Memory transfer bandwidth in GigaBytes Per Second 
* Host->Device Memory transfer latency in microseconds
* Device->Host Memory transfer bandwidth in GigaBytes Per Second
* Device->Host Memory transfer latency in microseconds

# Features
* Configurable range of transfer size measurements
* Configurable number of iterations per transfer size
* Optional user flag enables verification of first and last byte of every transfer
  
# How to Build it
See Build instructions in [BUILD](../BUILD.md) file.

# How to Run it
To run all benchmarks using the default settings: 
```
Default Settings:
* Both Host->Device and Device->Host memory transfer measurments performed
* Verification option disabled
* iterations per transfer size = 500
* transfer size range:  1 byte up to 2^30 bytes, with doubling in size per test case

To use command line option features:
 ze_bandwidth [OPTIONS]

 OPTIONS:
  -t, string               selectively run a particular test:
      h2d or H2D                       run only Host-to-Device tests
      d2h or D2H                       run only Device-to-Host tests
                            [default:  both]
  -v                       enable verification
                            [default:  disabled]
  -i                       set number of iterations per transfer
                            [default:  500]
  -w                       set number of warmup iterations
                            [default:  10]
  -s                       select only one transfer size (bytes)
  -sb                      select beginning transfer size (bytes)
                            [default:  1]
  -se                      select ending transfer size (bytes)
                            [default: 2^30]
  -q                       query for number of engines available
  -g, group                select engine group (default: 0)
  -n, number               select engine index (default: 0)
  --csv                    output in csv format (default: disabled)
  -h, --help               display help message

For example to run a single Host->Device test for transfer_size = 300 bytes, 100 iterations, verification enabled:

 ./ze_bandwidth -t h2d -s 300 -i 100 -v

