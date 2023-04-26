# Silverfir

The Silverfir project aims to provide a highly efficient and memory-friendly WebAssembly interpreter. It stands out from other WASM interpreters by interpreting original WASM instructions and using several optimization techniques to achieve better performance. The project is written in a unique "Rust" style C, making it less prone to memory errors. The project has been extensively tested, passing all of the WebAssembly 2.0 spec tests, with the exception of SIMD tests.

## Features

- Memory-efficient in-place WebAssembly interpreter
- Support WASI
- Optimized for performance. Check out [Optimization details](docs/Interpreter.md) for more info.
- Rust-style C for increased memory safety
- Container types to minimize pointer arithmetic
- Extensive unit testing
- Passes 100% of the WASM 2.0 spec tests, excluding SIMD tests

## Prerequisites

Make sure you have the following installed on your system:

- Python3
- CMake
- Ninja
- A C compiler (LLVM/GCC/MSVC)
- WABT (If you want to run spec tests)

## Build

To build the project, follow the steps below:

```
$ python3 build.py [Debug/Release/RelWithDebInfo/MinSizeRel]
```

To build the spec tests

```
$ python3 build_spectest.py
```

## Usage

After building the project, you will find the `sf_loader` binary in the `build/clang/<build_type>/bin` directory. To load a WebAssembly file using `sf_loader`, follow the example below:


```
$ /build/clang/Debug/bin/sf_loader ./test/perf/binarytree/binary-trees.wasm
```

To run the spec test, follow the steps below:

```
$ python3 run_spectest.py
```

## License

This project is licensed under the Apache 2.0 License