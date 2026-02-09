cmake_minimum_required(VERSION 3.10)
project(vulkan_gpu_crypto CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Find glslangValidator
find_program(GLSLANG_VALIDATOR glslangValidator)
if(NOT GLSLANG_VALIDATOR)
    message(FATAL_ERROR "glslangValidator not found. Please install Vulkan SDK.")
endif()

# Directories
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(BLST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/blst)
set(BLST_INC ${BLST_DIR}/bindings)
set(BLST_LIB ${BLST_DIR}/libblst.a)

# Shader compilation function
function(compile_shader SHADER_FILE OUTPUT_FILE)
    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/build
        COMMAND ${GLSLANG_VALIDATOR} -I${SRC_DIR} -V ${SHADER_FILE} -o ${OUTPUT_FILE}
        DEPENDS ${SHADER_FILE}
        COMMENT "Compiling shader ${SHADER_FILE}"
    )
endfunction()

# Compile Shaders
set(MATMUL_SPV ${CMAKE_CURRENT_BINARY_DIR}/build/vulkan_matmul.spv)
set(MSM_SPV ${CMAKE_CURRENT_BINARY_DIR}/build/msm.spv)

compile_shader(${SHADER_DIR}/vulkan_matmul.comp ${MATMUL_SPV})
compile_shader(${SHADER_DIR}/msm.comp ${MSM_SPV})

# Executable
add_executable(benchmark
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/cpu_matmul.cpp
    ${SRC_DIR}/cpu_msm.cpp
    ${MATMUL_SPV}
    ${MSM_SPV}
)

target_include_directories(benchmark PRIVATE
    ${SRC_DIR}
    ${BLST_INC}
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(benchmark PRIVATE
    ${Vulkan_LIBRARIES}
    ${BLST_LIB}
)

# Set runtime output directory for SPV files if needed 
# or ensure main.cpp looks in the right place.
# The Makefile puts SPV in BUILD_DIR along with the executable.
# CMake by default puts executable in CMAKE_CURRENT_BINARY_DIR.
