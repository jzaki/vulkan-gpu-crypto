#define VK_ENABLE_BETA_EXTENSIONS

#include <vulkan/vulkan.h>

#include <fstream>
#include <iostream>
#include <vector>

#include "common.h"

#define VK_CHECK(x)                                                                  \
    do {                                                                             \
        VkResult err = x;                                                            \
        if (err) {                                                                   \
            std::cerr << "Vulkan Error: " << err << " at " << __LINE__ << std::endl; \
            exit(1);                                                                 \
        }                                                                            \
    } while (0)

struct VulkanCompute {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue queue;
    uint32_t queueFamilyIndex;

    void init(bool verbose = false) {
        if (verbose) std::cout << "Initializing Vulkan..." << std::endl;

        VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
        appInfo.pApplicationName = "Vulkan GPU Crypto";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        createInfo.pApplicationInfo = &appInfo;
        VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));
        if (verbose) std::cout << "Vulkan Instance created." << std::endl;

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        physicalDevice = devices[0];  // Just take the first one for simplicity

        if (verbose) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
            std::cout << "Selected GPU: " << deviceProperties.deviceName << std::endl;
            std::cout << "Device ID: " << deviceProperties.deviceID << std::endl;
            std::cout << "Vendor ID: " << deviceProperties.vendorID << std::endl;
            std::cout << "API Version: " << VK_API_VERSION_MAJOR(deviceProperties.apiVersion) << "."
                      << VK_API_VERSION_MINOR(deviceProperties.apiVersion) << "."
                      << VK_API_VERSION_PATCH(deviceProperties.apiVersion) << std::endl;
        }

        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &deviceCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(deviceCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &deviceCount,
                                                 queueFamilies.data());

        bool foundCompute = false;
        for (uint32_t i = 0; i < deviceCount; i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                queueFamilyIndex = i;
                foundCompute = true;
                if (verbose)
                    std::cout << "Found compute queue family at index " << queueFamilyIndex
                              << std::endl;
                break;
            }
        }
        if (!foundCompute) {
            std::cerr << "Failed to find a compute queue family!" << std::endl;
            exit(1);
        }

        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkDeviceCreateInfo deviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
        VK_CHECK(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));
        if (verbose) std::cout << "Logical Device created." << std::endl;

        vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
    }

    VkBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkDeviceMemory* memory) {
        VkBuffer buffer;
        VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_CHECK(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(device, buffer, &memReqs);

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        uint32_t memoryTypeIndex = uint32_t(-1);
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((memReqs.memoryTypeBits & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags &
                 (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
                memoryTypeIndex = i;
                break;
            }
        }

        VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = memoryTypeIndex;
        VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, memory));
        VK_CHECK(vkBindBufferMemory(device, buffer, *memory, 0));

        return buffer;
    }

    void cleanup() {
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
    }
};
