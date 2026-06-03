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

This compiles `copytool.cpp` and `shared_segment.hpp` into the `copytool` executable.

### Running the Tool

The executable accepts three arguments:

```sh
copytool.exe <source_file> <target_file> <shared_memory_name>
```

Run it twice with the same shared memory name:

1. First process creates the shared memory segment and reads the source file.
2. Second process opens the shared memory segment and writes the target file.

On Windows, use the helper script:

```bat
copy_ipc.bat <source_file> <target_file> <shared_memory_name>
```

Example:

```bat
copy_ipc.bat input.txt output.txt copytool_shm
```

### Runtime Sequence

Synchronization goes through `SharedChannel`, which wraps the `SharedControl` block in shared memory (mutex, condition variables, and a fixed-size ring buffer).

```mermaid
sequenceDiagram
    participant Script as Script
    participant Reader as Reader process
    participant Writer as Writer process
    participant Channel as SharedChannel
    participant Source as Source file
    participant Target as Target file

    Script->>Reader: start copytool source target shm_name
    Reader->>Channel: SharedSegment create + placement-new SharedControl
    Reader->>Source: open source file
    Reader->>Channel: signal_reader_init(status)

    Script->>Writer: start copytool source target shm_name
    Writer->>Channel: SharedSegment open existing segment
    Writer->>Channel: wait_for_reader_init()
    alt reader init failed
        Writer-->>Script: exit with reader status code
    else reader init OK
        Writer->>Target: open target file
        alt writer init failed
            Writer->>Channel: signal_writer_finished()
            Writer-->>Script: exit with error code
        else writer init OK
            par Reader transfer loop
                loop Until push returns false
                    Reader->>Source: read chunk into local Slot
                    Reader->>Channel: push(slot)
                    Note over Channel: wait for empty slot (or writer abort), copy to ring buffer, notify writer
                end
            and Writer transfer loop
                loop Until pop returns false
                    Writer->>Channel: pop(slot)
                    Note over Channel: wait for ready slot, copy from ring buffer, release slot, notify reader
                    Writer->>Target: write chunk
                end
            end
            Note over Channel: EOF slot (size 0) ends both loops, pop sets writer_finished
            Reader->>Channel: wait_for_writer_finished()
            Reader->>Channel: destroy SharedControl and remove segment
        end
    end
```

### Building the Tests

```sh
make test
```

Builds the tests. Requires gtest.

### Building and Running the Tests

```sh
make run_test
```

Builds and runs the tests. Requires gtest.