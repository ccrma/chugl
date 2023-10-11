<div align="center">
<!-- Add ChuGL logo -->
<!-- <img align="left" style="width:260px" src="https://github.com/raysan5/raylib/blob/master/logo/raylib_logo_animation.gif" width="288px"> -->

![logo](images/chugl-glogo2023g.png)

## ChuGL =&gt; ChucK Graphics
Version 0.1.0 [alpha]

</div> <!-- end center -->

<p align="justify">
ChuGL is a unified audiovisual programming tool built into the ChucK language. It combines ChucK's <b>strongly-timed, concurrent programming model</b> and <b>real-time audio synthesis</b> capabilities with a <b>hardware-accelerated 3D graphics engine and API</b>. At its core is a scenegraph architecture that provides <b>low-latency, high performance audiovisual synchronization at frame-level accuracy</b>.

ChuGL was created by <a href="https://ccrma.stanford.edu/~azaday/">Andrew Zhu Aday</a> and
<a href="https://ccrma.stanford.edu/~ge/">Ge Wang</a>, with support from the <a 
href="https://chuck.stanford.edu/doc/authors.html">ChucK Team</a>, and benefitted from prior 
prototypes of ChuGL and GLucK by Spencer Salazar and Philip Davidson.
</p>

___

## Installing ChuGL

1. **Get the latest ChucK (requires 1.5.1.5 or higher)**
https://chuck.stanford.edu/release/
2. **Get the latest ChuGL chugin**
https://chuck.stanford.edu/release/alpha/chugl/

### MacOS

Run the .pkg file, which will install the ChuGL chugin in the appropriate location

### Windows

Download the `ChuGL.chug` binary from the installation links above and move it to `C:\Users\<usename>\Documents\ChucK\chugins\`

### Linux

We are working on it! Let us know if you get it working (or would like to help us to get it working)!

## Building ChuGL
To build the latest ChuGL from source, clone the `chugl` repo from github:
```
git clone https://github.com/ccrma/chugl.git
```
Compatibility note: ChuGL requires ChucK 1.5.1.5 or higher. See the <a href="https://github.com/ccrma/chuck/">chuck</a> repo for more details regarding installing/building ChucK.

### macOS
navigate to the `chugl/src` directory, and run `make mac`:
```
cd chugl/src
make mac
```
This should build a `ChuGL.chug` executable in `build-release`, which is also copied to the `chugl/src` directory. This file can be installed/used as any other chugin (e.g., install ChuGL.chug by copying it into `~/.chuck/lib`.)

### Windows
To build chuck using Visual Studio (2019 or later recommended), navigate to `chugl\src\`, and run `make build-release`:
```
cd chugl/src
make build-release
```
This creates a CMake compatible `build-release` directory with Visual Studio 2019 project files. Navigate into this directory and open `ChuGL.sln`. Building this project should create `ChuGL.chug`. This file can be installed/used as any other chugin (e.g., install ChuGL.chug by copying it into `C:\Users\<usename>\Documents\ChucK\chugins\`.)
### Linux
We are working on it!

## Running ChuGL

**Note:** Currently ChuGL only supports command-line chuck. MiniAudicle support to come soon. 

You can run commandline ChucK with the option `--chugin-probe` to check which chugins are found and properly loaded.

### Minimal Example

If the chugin is properly loaded, running the following example via commandline chuck will open a blank window. Press `esc` to exit. 

```cpp
while (true) { GG.nextFrame() => now; }
```

Congrats, you now have ChuGL properly installed!

## Learning Resouces

- [API Reference](https://chuck.stanford.edu/chugl/api/)
- [Examples](https://chuck.stanford.edu/chugl/examples/)
- [ChuGL Tutorial](https://chuck.stanford.edu/chugl/doc/tutorial.html)
