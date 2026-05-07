CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread
TARGET = copytool
SRC = copytool.cpp
GTEST_LIB = -lgtest -lgtest_main -lpthread

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

test: $(TARGET) test/test_copytool.cpp
	$(CXX) $(CXXFLAGS) test/test_copytool.cpp -o test_runner $(GTEST_LIB)

run_test: test
	./test_runner

clean:
	rm -f $(TARGET) test_runner