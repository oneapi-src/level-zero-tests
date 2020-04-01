# Description
ze_image_copy is a performance microbenchmark for measuring the image transfer bandwidth between Host memory and GPU Device memory.

ze_image_copy  measures image (size is configurable via commandline)  transfer bandwidth for the following:
* From Host to Device  in GigaBytes Per Second 
* From Device to Host in GigaBytes Per Second
* From Host to Device to Host in GigaBytes per Second

# Features
* Configurable image width,height,depth,xoffset,yoffset,zoffset
* Configurable number of iterations per image transfer

# How to Build it
See Build instructions in [BUILD](../BUILD.md) file.

# How to Run it
To run all benchmarks using the default settings: 
```
Default Settings:
* Both Host->Device and Device->Host image transfer bandwidth measurments performed 
* Host->Device->Host image tranfer bandwdith measurment performed
* Default image size is 2048x2048x1
* Default iterations per image transfer = 50

To use command line option features:
 ze_image_copy [OPTIONS]

 OPTIONS:
   --help                     produce help message
  -w [ --width ]              set image width (by default it is 2048)
  -h [ --height ]             set image height (by default it is 2048)
  -d [ --depth ]              set image depth (by default it is 1
  --offx                      set image xoffset (by default it is 0)
  --offy                      set image yoffset (by default it is 0)
  --offz                      set image zoffset (by default it is 0)
  --warmup                    set number of warmup operations (by default it is 10)
  --iter                      set number of iterations (by default it is 50)
  --layout                    image layout like 8/16/32/8_8/8_8_8_8/16_16/16_16_16_16/32_32/32_32_32_32/10_10_10_2/11_11_10/5_6_5/5_5_5_1/4_4_4_4/Y8/NV12/YUYV/VYUY/YVYU/UYVY/AY
                              UV/YUAV/P010/Y410/P012/Y16/P016/Y216/P216/P416
  --flags                     image program flags like READ/WRITE/CACHED/UNCACHED
  --type arg                  Image  type like 1D/2D/3D/1DARRAY/2DARRAY
  --format arg                image format like UINT/SINT/UNORM/SNORM/FLOAT


For example to run a ze_image_copy with width 1024 height 1024:

 ./ze_image_copy -w 1024 -h 1024

