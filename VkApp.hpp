#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <VkBootstrap.h>
#include "SceneGraph.hpp"
#include <vector>
#include <memory>

class VkApp {
public:
    void run();
private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

    GLFWwindow* window = nullptr;
    vkb::Instance vkbInstance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily = 0;
    vkb::Swapchain vkbSwapchain;
    VkSurfaceKHR surface;
    std::vector<VkImageView> imageViews;
    std::vector<VkImage> swapchainImages;
    VkExtent2D swapchainExtent;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    SceneGraph scene;
    std::shared_ptr<Group> group; 
};