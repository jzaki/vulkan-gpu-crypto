#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <cstring>
#include "common.h"
#include "cpu_matmul.h"
#include "vulkan_helper.h"

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

int main() {
    MatrixParams params = { BATCH, M, K, N };
    size_t sizeA = BATCH * M * K;
    size_t sizeB = BATCH * K * N;
    size_t sizeC = BATCH * M * N;

    std::vector<float> h_A(sizeA), h_B(sizeB), h_C_cpu(sizeC, 0.0f), h_C_gpu(sizeC, 0.0f);
    for (size_t i = 0; i < sizeA; ++i) h_A[i] = static_cast<float>(rand()) / RAND_MAX;
    for (size_t i = 0; i < sizeB; ++i) h_B[i] = static_cast<float>(rand()) / RAND_MAX;

    // CPU Benchmark
    std::cout << "Starting CPU Benchmark..." << std::endl;
    // print size of matrices
    std::cout << "Matrix A size: " << sizeA << std::endl;
    std::cout << "Matrix B size: " << sizeB << std::endl;
    std::cout << "Matrix C size: " << sizeC << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    cpu_matmul(h_A, h_B, h_C_cpu, params);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> cpu_duration = end - start;
    std::cout << "CPU Time: " << cpu_duration.count() << " ms" << std::endl;

    // Vulkan Benchmark
    VulkanCompute vk;
    vk.init();

    VkDeviceMemory memA, memB, memC;
    VkBuffer bufA = vk.createBuffer(sizeA * sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &memA);
    VkBuffer bufB = vk.createBuffer(sizeB * sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &memB);
    VkBuffer bufC = vk.createBuffer(sizeC * sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &memC);

    void *data;
    vkMapMemory(vk.device, memA, 0, sizeA * sizeof(float), 0, &data);
    memcpy(data, h_A.data(), sizeA * sizeof(float));
    vkUnmapMemory(vk.device, memA);

    vkMapMemory(vk.device, memB, 0, sizeB * sizeof(float), 0, &data);
    memcpy(data, h_B.data(), sizeB * sizeof(float));
    vkUnmapMemory(vk.device, memB);

    auto spirv = load_spirv("build/vulkan_matmul.spv");
    VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    createInfo.codeSize = spirv.size() * 4;
    createInfo.pCode = spirv.data();
    VkShaderModule shaderModule;
    VK_CHECK(vkCreateShaderModule(vk.device, &createInfo, nullptr, &shaderModule));

    VkDescriptorSetLayoutBinding bindings[3] = {};
    for(int i=0; i<3; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutInfo.bindingCount = 3;
    layoutInfo.pBindings = bindings;
    VkDescriptorSetLayout descriptorSetLayout;
    VK_CHECK(vkCreateDescriptorSetLayout(vk.device, &layoutInfo, nullptr, &descriptorSetLayout));

    VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(MatrixParams) };
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

    VkDescriptorPoolSize poolSize = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 };
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

    VkDescriptorBufferInfo bufferInfos[3] = {
        {bufA, 0, VK_WHOLE_SIZE}, {bufB, 0, VK_WHOLE_SIZE}, {bufC, 0, VK_WHOLE_SIZE}
    };
    VkWriteDescriptorSet writes[3] = {};
    for(int i=0; i<3; ++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = descriptorSet;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].pBufferInfo = &bufferInfos[i];
    }
    vkUpdateDescriptorSets(vk.device, 3, writes, 0, nullptr);

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
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(MatrixParams), &params);
    vkCmdDispatch(commandBuffer, (M + 7) / 8, (N + 7) / 8, BATCH);
    vkEndCommandBuffer(commandBuffer);

    std::cout << "Starting Vulkan Benchmark..." << std::endl;
    start = std::chrono::high_resolution_clock::now();
    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    VK_CHECK(vkQueueSubmit(vk.queue, 1, &submitInfo, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(vk.queue));
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> gpu_duration = end - start;
    std::cout << "Vulkan GPU Time: " << gpu_duration.count() << " ms" << std::endl;

    vkMapMemory(vk.device, memC, 0, sizeC * sizeof(float), 0, &data);
    memcpy(h_C_gpu.data(), data, sizeC * sizeof(float));
    vkUnmapMemory(vk.device, memC);

    // Verification
    bool match = true;
    for (size_t i = 0; i < sizeC; ++i) {
        if (std::abs(h_C_cpu[i] - h_C_gpu[i]) > 1e-3) {
            match = false;
            std::cout << "Mismatch at " << i << ": CPU=" << h_C_cpu[i] << " GPU=" << h_C_gpu[i] << std::endl;
            break;
        }
    }
    std::cout << "Verification: " << (match ? "PASSED" : "FAILED") << std::endl;

    // Cleanup
    vkDestroyCommandPool(vk.device, commandPool, nullptr);
    vkDestroyDescriptorPool(vk.device, descriptorPool, nullptr);
    vkDestroyPipeline(vk.device, pipeline, nullptr);
    vkDestroyPipelineLayout(vk.device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(vk.device, descriptorSetLayout, nullptr);
    vkDestroyShaderModule(vk.device, shaderModule, nullptr);
    vkDestroyBuffer(vk.device, bufA, nullptr);
    vkDestroyBuffer(vk.device, bufB, nullptr);
    vkDestroyBuffer(vk.device, bufC, nullptr);
    vkFreeMemory(vk.device, memA, nullptr);
    vkFreeMemory(vk.device, memB, nullptr);
    vkFreeMemory(vk.device, memC, nullptr);
    vk.cleanup();

    return 0;
}
