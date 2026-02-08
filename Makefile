BUILD_DIR = build
BLST_DIR = third_party/blst
BLST_LIB = $(BLST_DIR)/libblst.a
BLST_INC = $(BLST_DIR)/bindings

all: $(BUILD_DIR)/benchmark

$(BUILD_DIR)/benchmark: src/main.cpp src/cpu_matmul.cpp src/cpu_msm.cpp $(BUILD_DIR)/vulkan_matmul.spv $(BUILD_DIR)/msm.spv
	@mkdir -p $(BUILD_DIR)
	g++ -O3 -Isrc -I$(BLST_INC) src/main.cpp src/cpu_matmul.cpp src/cpu_msm.cpp $(BLST_LIB) -o $(BUILD_DIR)/benchmark -lvulkan

$(BUILD_DIR)/vulkan_matmul.spv: shaders/vulkan_matmul.comp
	@mkdir -p $(BUILD_DIR)
	glslangValidator -Isrc -V shaders/vulkan_matmul.comp -o $(BUILD_DIR)/vulkan_matmul.spv

$(BUILD_DIR)/msm.spv: shaders/msm.comp
	@mkdir -p $(BUILD_DIR)
	glslangValidator -Isrc -V shaders/msm.comp -o $(BUILD_DIR)/msm.spv

clean:
	rm -rf $(BUILD_DIR)
