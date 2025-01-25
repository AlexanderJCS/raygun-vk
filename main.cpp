#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <vulkan/vulkan.h>

#include "vktools.h"
#include "consts.h"
#include "Window.h"

VkDeviceAddress getBufferDeviceAddress(VkDevice device, VkBuffer buffer)
{
    VkBufferDeviceAddressInfo addressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = buffer };
    return vkGetBufferDeviceAddress(device, &addressInfo);
}

uint32_t makeAccessMaskPipelineStageFlags(uint32_t accessMask)
{
    VkPipelineStageFlags supportedShaderBits = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT
                                               | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT
                                               | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                                               | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

    static const uint32_t accessPipes[] = {
            VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
            VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
            VK_ACCESS_INDEX_READ_BIT,
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            VK_ACCESS_UNIFORM_READ_BIT,
            supportedShaderBits,
            VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            supportedShaderBits,
            VK_ACCESS_SHADER_WRITE_BIT,
            supportedShaderBits,
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_TRANSFER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_ACCESS_HOST_READ_BIT,
            VK_PIPELINE_STAGE_HOST_BIT,
            VK_ACCESS_HOST_WRITE_BIT,
            VK_PIPELINE_STAGE_HOST_BIT,
            VK_ACCESS_MEMORY_READ_BIT,
            0,
            VK_ACCESS_MEMORY_WRITE_BIT,
            0,
#if VK_NV_device_generated_commands
            VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV,
            VK_PIPELINE_STAGE_COMMAND_PREPROCESS_BIT_NV,
            VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV,
            VK_PIPELINE_STAGE_COMMAND_PREPROCESS_BIT_NV,
#endif
#if VK_NV_ray_tracing
            VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV,
            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV | supportedShaderBits | VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
            VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
#endif
    };
    if(!accessMask)
    {
        return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }

    uint32_t pipes = 0;

    for(uint32_t i = 0; i < sizeof(accessPipes) / sizeof(accessPipes[0]); i += 2)
    {
        if(accessPipes[i] & accessMask)
        {
            pipes |= accessPipes[i + 1];
        }
    }
    assert(pipes != 0);

    return pipes;
}

int main() {
    try {
        // init
        window::Window renderWindow{800, 600};
        VkInstance instance = vktools::createInstance();
        VkDebugUtilsMessengerEXT debugMessenger = vktools::createDebugMessenger(instance);
        VkSurfaceKHR surface = vktools::createSurface(instance, renderWindow.getGlfwWindow());
        VkPhysicalDevice physicalDevice = vktools::pickPhysicalDevice(instance, surface);
        VkDevice logicalDevice = vktools::createLogicalDevice(surface, physicalDevice);

        vktools::QueueFamilyIndices indices = vktools::findQueueFamilies(surface, physicalDevice);
        VkQueue graphicsQueue;
        VkQueue presentQueue;
        vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);

        vktools::SwapchainObjects swapchainObjects = vktools::createSwapchain(surface, physicalDevice, logicalDevice, renderWindow.getWidth(), renderWindow.getHeight());
        std::vector<VkImageView> swapchainImageViews = vktools::createSwapchainImageViews(logicalDevice, swapchainObjects.swapchainImageFormat, swapchainObjects.swapchainImages);

        vktools::ImageObjects rtImageObjects = vktools::createRtImage(logicalDevice, physicalDevice, swapchainObjects.swapchainExtent.width, swapchainObjects.swapchainExtent.height);
        VkImageView rtImageView = vktools::createRtImageView(logicalDevice, rtImageObjects.image);

        vktools::PipelineInfo rtPipelineInfo = vktools::createRtPipeline(physicalDevice, logicalDevice);
        vktools::SyncObjects syncObjects = vktools::createSyncObjects(logicalDevice);

        // future: create render pass, frame buffer, and (maybe?) graphics pipeline for imgui stuff

        VkCommandPool commandPool = vktools::createCommandPool(physicalDevice, logicalDevice, surface);
        VkCommandBuffer commandBuffer = vktools::createCommandBuffer(logicalDevice, commandPool);

        // render

        // look at line 521 of my main cpp file for vk mini path tracer

        while (!renderWindow.shouldClose()) {
            vkWaitForFences(logicalDevice, 1, &syncObjects.inFlightFence, VK_TRUE, UINT64_MAX);
            vkResetFences(logicalDevice, 1, &syncObjects.inFlightFence);

            uint32_t imageIndex;
            VkResult result = vkAcquireNextImageKHR(logicalDevice, swapchainObjects.swapchain, UINT64_MAX, syncObjects.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                // todo: handle swapchain recreation
                break;
            } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                throw std::runtime_error("Failed to acquire swapchain image");
            }

            VkCommandBufferBeginInfo beginInfo{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = 0,  // optional
                .pInheritanceInfo = nullptr  // optional
            };

            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            VkImageMemoryBarrier rayTracingToGeneralBarrier{};
            rayTracingToGeneralBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            rayTracingToGeneralBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            rayTracingToGeneralBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            rayTracingToGeneralBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            rayTracingToGeneralBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            rayTracingToGeneralBarrier.image = rtImageObjects.image;
            rayTracingToGeneralBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            rayTracingToGeneralBarrier.subresourceRange.baseMipLevel = 0;
            rayTracingToGeneralBarrier.subresourceRange.levelCount = 1;
            rayTracingToGeneralBarrier.subresourceRange.baseArrayLayer = 0;
            rayTracingToGeneralBarrier.subresourceRange.layerCount = 1;
            rayTracingToGeneralBarrier.srcAccessMask = 0;
            rayTracingToGeneralBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

            vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &rayTracingToGeneralBarrier
            );

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rtPipelineInfo.pipeline);

            vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                    rtPipelineInfo.pipelineLayout,
                    0,  // First set
                    1, &rtPipelineInfo.descriptorSet,
                    0, nullptr
            );

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageView = rtImageView;          // The VkImageView for your ray-traced output
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;         // Layout must match what the shader expects

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = rtPipelineInfo.descriptorSet;   // The descriptor set to update
            descriptorWrite.dstBinding = 0;                         // Binding index in the shader
            descriptorWrite.dstArrayElement = 0;                    // First array element to update
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;  // Match descriptor type in the shader
            descriptorWrite.descriptorCount = 1;                    // Number of descriptors to update
            descriptorWrite.pImageInfo = &imageInfo;                // Point to the VkDescriptorImageInfo

            vkUpdateDescriptorSets(logicalDevice, 1, &descriptorWrite, 0, nullptr);

            // todo: push the push constants

            VkStridedDeviceAddressRegionKHR sbtRayGenRegion, sbtMissRegion, sbtHitRegion, sbtCallableRegion;
            VkDeviceAddress sbtStartAddress = getBufferDeviceAddress(logicalDevice, rtPipelineInfo.sbtBuffer);

            sbtRayGenRegion.deviceAddress = sbtStartAddress;
            sbtRayGenRegion.stride = rtPipelineInfo.sbtSpacing.stride;
            sbtRayGenRegion.size = rtPipelineInfo.sbtSpacing.stride;

            sbtMissRegion = sbtRayGenRegion;
            sbtMissRegion.size = 0;  // empty

            sbtHitRegion = sbtRayGenRegion;
            sbtHitRegion.size = 0;

            sbtCallableRegion = sbtRayGenRegion;
            sbtCallableRegion.size = 0;

            auto vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(
                    vkGetDeviceProcAddr(logicalDevice, "vkCmdTraceRaysKHR"));

            if (!vkCmdTraceRaysKHR) {
                throw std::runtime_error("Failed to load vkCmdTraceRaysKHR");
            }

            vkCmdTraceRaysKHR(
                    commandBuffer,
                    &sbtRayGenRegion,
                    &sbtMissRegion,
                    &sbtHitRegion,
                    &sbtCallableRegion,
                    swapchainObjects.swapchainExtent.width,
                    swapchainObjects.swapchainExtent.height,
                    1
            );

            // Transition rayTracingImage to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
            VkImageMemoryBarrier rayTracingToSrcBarrier{};
            rayTracingToSrcBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            rayTracingToSrcBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            rayTracingToSrcBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            rayTracingToSrcBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            rayTracingToSrcBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            rayTracingToSrcBarrier.image = rtImageObjects.image;
            rayTracingToSrcBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            rayTracingToSrcBarrier.subresourceRange.baseMipLevel = 0;
            rayTracingToSrcBarrier.subresourceRange.levelCount = 1;
            rayTracingToSrcBarrier.subresourceRange.baseArrayLayer = 0;
            rayTracingToSrcBarrier.subresourceRange.layerCount = 1;
            rayTracingToSrcBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            rayTracingToSrcBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &rayTracingToSrcBarrier
            );

            // Transition swapchain image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            VkAccessFlags srcAccesses = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            VkAccessFlags dstAccesses = VK_ACCESS_TRANSFER_READ_BIT;

            VkPipelineStageFlags srcStages = makeAccessMaskPipelineStageFlags(srcAccesses);
            VkPipelineStageFlags dstStages = makeAccessMaskPipelineStageFlags(dstAccesses);

            VkImageMemoryBarrier barrier        = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            barrier.srcAccessMask               = srcAccesses;
            barrier.dstAccessMask               = dstAccesses;
            barrier.oldLayout                   = VK_IMAGE_LAYOUT_GENERAL;
            barrier.newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
            barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
            barrier.image                       = rtImageObjects.image;
            barrier.subresourceRange            = {0};
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

            VkImageCopy region{
                .srcSubresource = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = 0,
                        .baseArrayLayer = 0,
                        .layerCount = 1
                },
                .srcOffset = {0, 0, 0},
                .dstSubresource = region.srcSubresource,
                .dstOffset = {0, 0, 0},
                .extent = {swapchainObjects.swapchainExtent.width, swapchainObjects.swapchainExtent.height, 1}
            };

            vkCmdCopyImage(
                    commandBuffer,
                    rtImageObjects.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    swapchainObjects.swapchainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1, &region
            );

            VkMemoryBarrier memoryBarrier{
                .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_HOST_READ_BIT
            };

            vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_HOST_BIT,
                    0,
                    1,
                    &memoryBarrier,
                    0, nullptr, 0, nullptr
            );

            VkImageMemoryBarrier swapchainToPresentBarrier{};
            swapchainToPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            swapchainToPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            swapchainToPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            swapchainToPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            swapchainToPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            swapchainToPresentBarrier.image = swapchainObjects.swapchainImages[imageIndex];
            swapchainToPresentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            swapchainToPresentBarrier.subresourceRange.baseMipLevel = 0;
            swapchainToPresentBarrier.subresourceRange.levelCount = 1;
            swapchainToPresentBarrier.subresourceRange.baseArrayLayer = 0;
            swapchainToPresentBarrier.subresourceRange.layerCount = 1;
            swapchainToPresentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            swapchainToPresentBarrier.dstAccessMask = 0;

            vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &swapchainToPresentBarrier
            );

            vkEndCommandBuffer(commandBuffer);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore waitSemaphores[] = {syncObjects.imageAvailableSemaphore};
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TRANSFER_BIT};
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;

            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            VkSemaphore signalSemaphores[] = {syncObjects.renderFinishedSemaphore};
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            vkQueueSubmit(graphicsQueue, 1, &submitInfo, syncObjects.inFlightFence);

            // Present the swapchain image
            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;

            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &swapchainObjects.swapchain;
            presentInfo.pImageIndices = &imageIndex;

            vkQueuePresentKHR(presentQueue, &presentInfo);

            glfwPollEvents();

            break;  // temporary: render only one frame.
        }

        vkDeviceWaitIdle(logicalDevice);

        // clean up
        vkDestroySemaphore(logicalDevice, syncObjects.renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(logicalDevice, syncObjects.imageAvailableSemaphore, nullptr);
        vkDestroyFence(logicalDevice, syncObjects.inFlightFence, nullptr);
        vkDestroyBuffer(logicalDevice, rtPipelineInfo.sbtBuffer, nullptr);
        vkFreeMemory(logicalDevice, rtPipelineInfo.sbtBufferMemory, nullptr);
        vkDestroyPipeline(logicalDevice, rtPipelineInfo.pipeline, nullptr);
        vkDestroyPipelineLayout(logicalDevice, rtPipelineInfo.pipelineLayout, nullptr);
        vkDestroyDescriptorPool(logicalDevice, rtPipelineInfo.descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(logicalDevice, rtPipelineInfo.descriptorSetLayout, nullptr);
        vkDestroyImageView(logicalDevice, rtImageView, nullptr);
        vkDestroyImage(logicalDevice, rtImageObjects.image, nullptr);
        vkFreeMemory(logicalDevice, rtImageObjects.imageMemory, nullptr);

        vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

        for (VkImageView imageView : swapchainImageViews) {
            vkDestroyImageView(logicalDevice, imageView, nullptr);
        }

        vkDestroySwapchainKHR(logicalDevice, swapchainObjects.swapchain, nullptr);

        vkDestroyDevice(logicalDevice, nullptr);

        if (consts::ENABLE_VALIDATION_LAYERS) {
            vktools::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
        renderWindow.destroy();

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}