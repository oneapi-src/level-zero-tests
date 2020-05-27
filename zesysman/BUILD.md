# Building the Sysman CLI

`zesysman` provides a command-line interface to the oneAPI Level Zero System Resource Management services.

If the Level Zero library is installed in the default location (`/usr/local`], the tool can be build by simply calling make from this directory:

```bash
% make
```

If it is installed in another location, specify the location using `-DZEPREFIX`. For example:

```bash
% make -DZEPREFIX=/usr/opt
```

The executable image and supporting libraries are built into the `build/zesysman` subdirectory of this directory and should be executed in place. For example:

```bash
% build/zesysman/zesysman --help
```

```bash
% alias zesysman=$PWD/build/zesysman/zesysman
% zesysman --help
```
