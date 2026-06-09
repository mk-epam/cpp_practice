## copytool

A C++ file copy tool that uses two separate processes and shared memory.

The first started process acts as the reader and reads from the source file into a shared-memory ring buffer. The second started process acts as the writer and writes from shared memory into the destination file. Synchronization is done with Boost.Interprocess mutex and condition variables stored inside the shared memory segment.

### Build Requirements

- g++ with C++17 support
- make
- Boost.Interprocess headers
- gtest for running tests

### Building the Tool

```sh
make
```

This compiles the sources in `src/` into `build/copytool.exe`.

### Running the Tool

The executable accepts three arguments:

```sh
build/copytool.exe <source_file> <target_file> <shared_memory_name>
```

Run it twice with the same shared memory name:

1. First process creates the shared memory segment and reads the source file.
2. Second process opens the shared memory segment and writes the target file.

On Windows, use the helper script:

```bat
scripts\copy_ipc.bat <source_file> <target_file> <shared_memory_name>
```

Example:

```bat
scripts\copy_ipc.bat input.txt output.txt copytool_shm
```

### Building the Tests

```sh
make test
```

Builds the test runner at `build/test_runner.exe`. Requires gtest.

### Building and Running the Tests

```sh
make run_test
```

Builds and runs the tests. Requires gtest.
