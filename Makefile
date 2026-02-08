BUILD_DIR = build

all: $(BUILD_DIR)/benchmark

$(BUILD_DIR)/benchmark: src/main.cpp src/cpu_matmul.cpp $(BUILD_DIR)/vulkan_matmul.spv
	@mkdir -p $(BUILD_DIR)
	g++ -O3 -Isrc src/main.cpp src/cpu_matmul.cpp -o $(BUILD_DIR)/benchmark -lvulkan

$(BUILD_DIR)/vulkan_matmul.spv: shaders/vulkan_matmul.comp
	@mkdir -p $(BUILD_DIR)
	glslangValidator -Isrc -V shaders/vulkan_matmul.comp -o $(BUILD_DIR)/vulkan_matmul.spv

clean:
	rm -rf $(BUILD_DIR)
