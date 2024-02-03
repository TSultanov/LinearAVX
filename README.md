**Note**: this is an experiment and currently only few trivial programs cen be executed

LinearAVX is an attempt at implementing a JIT for AVX instructions, mainly targeting Rosetta and Wine

# Build
1. Clone and build Intel XED
```sh
git clone https://github.com/intelxed/xed.git xed
git clone https://github.com/intelxed/mbuild.git mbuild
cd xed
./mfile.py --ar=ar --extra-flags="-target x86_64-darwin-macho" --install-dir="kits/xed" install
```

2. Clone this repo
```sh
git clone https://github.com/TSultanov/LinearAVX
```

3. Build using CMake
```sh
mkdir build
cd build
cmake ..
make
```
the build will produce a file with a name `libavxhandler.dylib` in the `build` directory

4. Launch Windows application using Wine
```sh
DYLD_INSERT_LIBRARIES=</full/path/to/build/libavxhandler.dylib> wine <youwindowsapp.exe>
```
or, using CrossOver shell
```sh
wine --env DYLD_INSERT_LIBRARIES=</full/path/to/build/libavxhandler.dylib> <youwindowsapp.exe>
```
