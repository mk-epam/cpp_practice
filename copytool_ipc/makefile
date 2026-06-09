CXX = g++
CXXFLAGS = -std=c++17 -Wall -Isrc
BUILD_DIR = build
TARGET = $(BUILD_DIR)/copytool
TEST_RUNNER = $(BUILD_DIR)/test_runner
SRC = \
      src/Main.cpp \
      src/CopyTool.cpp \
      src/Role.cpp \
      src/ReaderSharedChannel.cpp \
      src/WriterSharedChannel.cpp \
      src/SharedSegment.cpp
HEADERS = \
      src/Slot.hpp \
      src/SharedControl.hpp \
      src/ReaderSharedChannel.hpp \
      src/WriterSharedChannel.hpp \
      src/SharedSegment.hpp \
      src/CopyResult.hpp \
      src/Role.hpp \
      src/CopyTool.hpp
GTEST_LIB = -lgtest -lgtest_main -lpthread

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(SRC) $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

test: $(TARGET) test/test_copytool.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) test/test_copytool.cpp -o $(TEST_RUNNER) $(GTEST_LIB)

run_test: test
	./$(TEST_RUNNER)

clean:
	rm -rf $(BUILD_DIR)
