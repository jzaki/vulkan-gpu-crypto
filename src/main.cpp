#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <cstring>
#include "common.h"
#include "cpu_matmul.h"
#include "cpu_msm.h"
#include "vulkan_helper.h"
#include "blst.h"

std::vector<uint> load_spirv(const char* filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("failed to open shader file");
    size_t fileSize = (size_t)file.tellg();
    std::vector<uint> buffer(fileSize / 4);

    file.seekg(0);
    file.read((char*)buffer.data(), fileSize);
    file.close();
    return buffer;
}

double run_vulkan(VulkanCompute& vk, const char* shader_path, void* params_ptr, size_t params_size, 
                  const std::vector<void*>& inputs, const std::vector<size_t>& input_sizes,
                  void* output, size_t output_size, uint32_t gx, uint32_t gy, uint32_t gz) {
    
    std::vector<VkBuffer> buffers(inputs.size() + 1);
    std::vector<VkDeviceMemory> memories(inputs.size() + 1);
    
    for (size_t i = 0; i < inputs.size(); ++i) {
        buffers[i] = vk.createBuffer(input_sizes[i], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &memories[i]);
        void *data;
        vkMapMemory(vk.device, memories[i], 0, input_sizes[i], 0, &data);
        memcpy(data, inputs[i], input_sizes[i]);
        vkUnmapMemory(vk.device, memories[i]);
    }
    
    buffers.back() = vk.createBuffer(output_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &memories.back());

    auto spirv = load_spirv(shader_path);
    VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    createInfo.codeSize = spirv.size() * 4;
    createInfo.pCode = spirv.data();
    VkShaderModule shaderModule;
    VK_CHECK(vkCreateShaderModule(vk.device, &createInfo, nullptr, &shaderModule));

    uint32_t num_bindings = inputs.size() + 1;
    std::vector<VkDescriptorSetLayoutBinding> bindings(num_bindings);
    for(uint32_t i=0; i<num_bindings; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutInfo.bindingCount = num_bindings;
    layoutInfo.pBindings = bindings.data();
    VkDescriptorSetLayout descriptorSetLayout;
    VK_CHECK(vkCreateDescriptorSetLayout(vk.device, &layoutInfo, nullptr, &descriptorSetLayout));

    VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_COMPUTE_BIT, 0, (uint32_t)params_size };
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    VkPipelineLayout pipelineLayout;
    VK_CHECK(vkCreatePipelineLayout(vk.device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

    VkComputePipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = shaderModule;
    pipelineInfo.stage.pName = "main";
    pipelineInfo.layout = pipelineLayout;
    VkPipeline pipeline;
    VK_CHECK(vkCreateComputePipelines(vk.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

    VkDescriptorPoolSize poolSize = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, num_bindings };
    VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    VkDescriptorPool descriptorPool;
    VK_CHECK(vkCreateDescriptorPool(vk.device, &poolInfo, nullptr, &descriptorPool));

    VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    VkDescriptorSet descriptorSet;
    VK_CHECK(vkAllocateDescriptorSets(vk.device, &allocInfo, &descriptorSet));

    std::vector<VkDescriptorBufferInfo> bufferInfos(num_bindings);
    std::vector<VkWriteDescriptorSet> writes(num_bindings);
    for(uint32_t i=0; i<num_bindings; ++i) {
        bufferInfos[i] = {buffers[i], 0, VK_WHOLE_SIZE};
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = descriptorSet;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].pBufferInfo = &bufferInfos[i];
    }
    vkUpdateDescriptorSets(vk.device, num_bindings, writes.data(), 0, nullptr);

    VkCommandPoolCreateInfo cmdPoolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    cmdPoolInfo.queueFamilyIndex = vk.queueFamilyIndex;
    VkCommandPool commandPool;
    VK_CHECK(vkCreateCommandPool(vk.device, &cmdPoolInfo, nullptr, &commandPool));

    VkCommandBufferAllocateInfo cmdBufAllocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cmdBufAllocInfo.commandPool = commandPool;
    cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;
    VK_CHECK(vkAllocateCommandBuffers(vk.device, &cmdBufAllocInfo, &commandBuffer));

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, (uint32_t)params_size, params_ptr);
    vkCmdDispatch(commandBuffer, gx, gy, gz);
    vkEndCommandBuffer(commandBuffer);

    auto v_start = std::chrono::high_resolution_clock::now();
    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    VK_CHECK(vkQueueSubmit(vk.queue, 1, &submitInfo, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(vk.queue));
    auto v_end = std::chrono::high_resolution_clock::now();

    void *out_data;
    vkMapMemory(vk.device, memories.back(), 0, output_size, 0, &out_data);
    memcpy(output, out_data, output_size);
    vkUnmapMemory(vk.device, memories.back());

    // Cleanup
    vkDestroyCommandPool(vk.device, commandPool, nullptr);
    vkDestroyDescriptorPool(vk.device, descriptorPool, nullptr);
    vkDestroyPipeline(vk.device, pipeline, nullptr);
    vkDestroyPipelineLayout(vk.device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(vk.device, descriptorSetLayout, nullptr);
    vkDestroyShaderModule(vk.device, shaderModule, nullptr);
    for (auto b : buffers) vkDestroyBuffer(vk.device, b, nullptr);
    for (auto m : memories) vkFreeMemory(vk.device, m, nullptr);

    return std::chrono::duration<double, std::milli>(v_end - v_start).count();
}

void benchmark_matmul(VulkanCompute& vk) {
    MatrixParams params = { BATCH, M, K, N };
    size_t sizeA = (size_t)BATCH * M * K;
    size_t sizeB = (size_t)BATCH * K * N;
    size_t sizeC = (size_t)BATCH * M * N;

    std::vector<float> h_A(sizeA), h_B(sizeB), h_C_cpu(sizeC, 0.0f), h_C_gpu(sizeC, 0.0f);
    for (size_t i = 0; i < sizeA; ++i) h_A[i] = static_cast<float>(rand()) / RAND_MAX;
    for (size_t i = 0; i < sizeB; ++i) h_B[i] = static_cast<float>(rand()) / RAND_MAX;

    std::cout << "\nStarting Matrix Multiplication Benchmark..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    cpu_matmul(h_A, h_B, h_C_cpu, params);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> cpu_duration = end - start;
    std::cout << "CPU MatMul Time: " << cpu_duration.count() << " ms" << std::endl;

    double gpu_matmul_time = run_vulkan(vk, "build/vulkan_matmul.spv", &params, sizeof(params), 
                                        {(void*)h_A.data(), (void*)h_B.data()}, 
                                        {h_A.size() * sizeof(float), h_B.size() * sizeof(float)}, 
                                        h_C_gpu.data(), h_C_gpu.size() * sizeof(float), 
                                        (M + 7) / 8, (N + 7) / 8, BATCH);
    std::cout << "Vulkan GPU MatMul Time: " << gpu_matmul_time << " ms" << std::endl;

    // Verification
    bool match = true;
    for (size_t i = 0; i < sizeC; ++i) {
        if (std::abs(h_C_cpu[i] - h_C_gpu[i]) > 1e-3) {
            match = false;
            std::cout << "MatMul Mismatch at " << i << ": CPU=" << h_C_cpu[i] << " GPU=" << h_C_gpu[i] << std::endl;
            break;
        }
    }
    std::cout << "MatMul Verification: " << (match ? "PASSED" : "FAILED") << std::endl;
}

void benchmark_msm(VulkanCompute& vk) {
    std::cout << "\nStarting Multi-Scalar Multiplication (MSM) Benchmark..." << std::endl;
    MsmParams params_msm = { MSM_POINTS };
    size_t sizePoints = MSM_POINTS * sizeof(blst_p1_affine) / sizeof(float);
    size_t sizeScalars = MSM_POINTS * sizeof(blst_scalar) / sizeof(float);
    size_t sizeResult = sizeof(blst_p1_affine) / sizeof(float);

    std::vector<float> h_points(sizePoints), h_scalars(sizeScalars), h_res_cpu(sizeResult, 0.0f), h_res_gpu(sizeResult, 0.0f);
    
    // Generate valid test data using blst
    blst_p1_affine* p_points = reinterpret_cast<blst_p1_affine*>(h_points.data());
    blst_scalar* p_scalars = reinterpret_cast<blst_scalar*>(h_scalars.data());
    for (uint i = 0; i < MSM_POINTS; ++i) {
        blst_scalar sk;
        byte ikm[32];
        for (int j = 0; j < 32; ++j) ikm[j] = rand() & 0xFF;
        blst_keygen(&sk, ikm, 32);
        p_scalars[i] = sk;
        
        blst_p1 p;
        blst_sk_to_pk_in_g1(&p, &sk);
        blst_p1_to_affine(&p_points[i], &p);
    }

    std::cout << "MSM Points: " << MSM_POINTS << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    cpu_msm(h_points, h_scalars, h_res_cpu, params_msm);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> cpu_msm_duration = end - start;
    std::cout << "CPU MSM Time: " << cpu_msm_duration.count() << " ms" << std::endl;

    double gpu_msm_time = run_vulkan(vk, "build/msm.spv", &params_msm, sizeof(params_msm), 
                                     {(void*)h_points.data(), (void*)h_scalars.data()}, 
                                     {h_points.size() * sizeof(float), h_scalars.size() * sizeof(float)}, 
                                     h_res_gpu.data(), h_res_gpu.size() * sizeof(float), 
                                     (MSM_POINTS + 255) / 256, 1, 1);
    std::cout << "Vulkan GPU MSM Time (Stub): " << gpu_msm_time << " ms" << std::endl;
}

int main(int argc, char** argv) {
    bool do_matmul = false;
    bool do_msm = false;

    if (argc == 1) {
        do_matmul = true;
        do_msm = true;
    } else {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--matmul") do_matmul = true;
            else if (arg == "--msm") do_msm = true;
            else if (arg == "--all") { do_matmul = true; do_msm = true; }
            else {
                std::cerr << "Unknown argument: " << arg << std::endl;
                std::cerr << "Usage: " << argv[0] << " [--matmul] [--msm] [--all]" << std::endl;
                return 1;
            }
        }
    }

    VulkanCompute vk;
    vk.init();

    if (do_matmul) benchmark_matmul(vk);
    if (do_msm) benchmark_msm(vk);

    vk.cleanup();
    return 0;
}

