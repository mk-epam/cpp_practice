## copytool
A simple two-threaded file copy tool in C++

### Build Requirements
- g++ and make
- gtest for running tests

### Building the Tool
```make``` compile copytool.cpp and produce the executable copytool 

### Running the Tool
```copytool.exe <source_file> <target_file>``` copies a file from source to target using two threads (Windows)

### Building the Tests
```make test``` build the tests (requires gtest)

### Building and running the Tests
```make run_test``` build and run the tests (requires gtest)