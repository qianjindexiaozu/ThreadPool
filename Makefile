BUILD_DIR := build

.PHONY: all clean

all:
	CC=gcc-16 CXX=g++-16 cmake -S . -B $(BUILD_DIR) \
		-G "Unix Makefiles" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	ln -sf $(BUILD_DIR)/compile_commands.json compile_commands.json
	cmake --build $(BUILD_DIR) -j


run: all
	./$(BUILD_DIR)/ThreadPool

clean:
	rm -rf $(BUILD_DIR) compile_commands.json
