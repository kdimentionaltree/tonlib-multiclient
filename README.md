# TONlib Multiclient

The TONlib Multi Client is crafted with modern C++ to serve as a robust and flexible tool for seamless communication with multiple TON blockchain lite servers. It's engineered for efficiency, allowing for streamlined request handling across a diverse array of server endpoints.

## Features

- Multi-threaded Design: Enhances the ability to handle concurrent connections and requests efficiently.
- Flexible Request Handling: Supports a variety of request types, ensuring robust interaction with blockchain lite servers.
- Dynamic Request Creation: Allows for the creation of customizable requests, enabling tailored blockchain queries and operations.
- Advanced Configuration Options: Offers detailed settings for requests, including server selection and archival data queries, providing users with control over their blockchain interactions.

## Requirements

- CMake: Version 3.15 or newer.
- C++ Compiler: Must support the C++20 standard or more recent.
- Python: Version 3.9 or above is required for utilizing Python bindings.

## Usage

Examples could be found in the `examples` directory.

## Building

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

Build with Python bindings, pass `-DPY_TONLIB_MULTICLIENT:BOOL=TRUE` to CMake:

```bash
cmake -DPY_TONLIB_MULTICLIENT:BOOL=TRUE ..
cmake --build .
```

To use built python binding you need to copy `tonlib_multiclient` dynamic lib (.so) from `build/py` directory to your python project.
```bash
cp build/py/tonlib_multiclient.so /path/to/your/python/project
```
