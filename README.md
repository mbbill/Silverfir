# Silverfir

Silverfir is a memory-efficient in-place WebAssembly (WASM) interpreter specifically designed for embedded systems with limited memory resources. Its unique design enables direct execution of WASM bytecode without compiling it to an intermediate format, saving memory in the process. Additionally, Silverfir eliminates the need to modify the WASM binary, allowing it to reside in a read-only memory region, such as Norflash. This feature is crucial for memory-constrained devices, as it greatly reduces RAM usage.

To further enhance the project's robustness and memory efficiency, Silverfir employs Rust-style C coding practices, which helps minimize memory errors and promote safer memory management. Moreover, the project utilizes container types instead of direct heap memory access, contributing to improved memory efficiency and overall system stability.

## Features

- Memory-efficient in-place WebAssembly interpreter
- Allows WASM binary to be in the read only memory region, suitable for embedded systems
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