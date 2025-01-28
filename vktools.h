#ifndef RAYGUN_VK_VKTOOLS_H
#define RAYGUN_VK_VKTOOLS_H

#include <optional>
#include <vector>
#include <string>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <memory>

#include "Shader.h"
#include "DescriptorSet.h"

namespace vktools {
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        [[nodiscard]] bool isComplete() const;
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct SwapchainObjects {
        VkSwapchainKHR swapchain;
        std::vector<VkImage> swapchainImages;
        VkFormat swapchainImageFormat;
        VkExtent2D swapchainExtent;
    };

    struct ImageObjects {
        VkImage image;
        VkDeviceMemory imageMemory;
    };

    struct SbtSpacing {
        VkDeviceSize headerSize;
        VkDeviceSize baseAlignment;
        VkDeviceSize handleAlignment;
        VkDeviceSize stride;
    };

    struct PipelineInfo {
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
    };

    struct SbtInfo {
        VkBuffer buffer;
        VkDeviceMemory deviceMemory;
    };

    struct SyncObjects {
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;
    };

    std::vector<char> readFile(const std::string& filename);
    bool hasValidationLayerSupport();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    QueueFamilyIndices findQueueFamilies(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice);
    SwapChainSupportDetails querySwapChainSupport(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice);
    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData
    );

    // todo: rename the create and destroy debug utils messenger EXT functions to something in-line with other code standards
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    uint64_t getDeviceLocalMemory(VkPhysicalDevice device);
    bool isDeviceSuitable(VkPhysicalDevice device);

    SyncObjects createSyncObjects(VkDevice logicalDevice);
    SbtSpacing calculateSbtSpacing(VkPhysicalDevice physicalDevice);
    SbtInfo createSbt(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkPipeline rtPipeline, SbtSpacing spacing);
    PipelineInfo createRtPipeline(VkDevice logicalDevice, const DescriptorSet& descriptorSet, const std::vector<Shader>& shaders);
    VkImageView createRtImageView(VkDevice logicalDevice, VkImage rtImage);
    ImageObjects createRtImage(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height);

    VkCommandBuffer createCommandBuffer(VkDevice logicalDevice, VkCommandPool commandPool);
    VkCommandPool createCommandPool(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkSurfaceKHR surface);
    std::vector<VkImageView> createSwapchainImageViews(VkDevice logicalDevice, VkFormat swapchainImageFormat, std::vector<VkImage> swapchainImages);
    SwapchainObjects createSwapchain(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, VkDevice logicalDevice, int windowWidth, int windowHeight);
    VkDevice createLogicalDevice(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice);
    VkPhysicalDevice pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
    VkSurfaceKHR createSurface(VkInstance instance, GLFWwindow* window);
    std::optional<VkDebugUtilsMessengerEXT> createDebugMessenger(VkInstance instance);
    VkInstance createInstance();
}


#endif //RAYGUN_VK_VKTOOLS_H
