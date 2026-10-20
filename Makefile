CXX ?= g++
SRC_DIR = components/nice_t4
CXXFLAGS = -std=c++11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -I$(SRC_DIR) -Itests
SRCS = $(SRC_DIR)/t4_frame.cpp $(SRC_DIR)/t4_rx.cpp $(SRC_DIR)/t4_engine.cpp $(SRC_DIR)/t4_chunk.cpp $(SRC_DIR)/t4_state.cpp
TESTS = tests/test_main.cpp tests/test_frame.cpp tests/test_rx.cpp tests/test_engine.cpp tests/test_state.cpp

.PHONY: test gen clean
test: build/run_tests
	./build/run_tests

build/run_tests: $(SRCS) $(TESTS) tests/test_util.h tests/test_vectors.h $(wildcard $(SRC_DIR)/*.h)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(SRCS) $(TESTS) -o $@

gen:
	python3 tools/gen_registers.py
	python3 tools/gen_test_vectors.py

clean:
	rm -rf build
