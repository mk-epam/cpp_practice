CXX = g++
CXXFLAGS = -std=c++17 -Wall
TARGET = copytool
SRC = copytool.cpp
HEADERS = shared_segment.hpp
GTEST_LIB = -lgtest -lgtest_main -lpthread

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

test: $(TARGET) test/test_copytool.cpp
	$(CXX) $(CXXFLAGS) test/test_copytool.cpp -o test_runner $(GTEST_LIB)

run_test: test
	./test_runner

clean:
	rm -f $(TARGET) test_runner