#pragma once
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <VkBootstrap.h>
#include "SceneGraph.hpp"

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
    vkb::Device vkbDevice;
    vkb::Swapchain vkbSwapchain;
    VkSurfaceKHR surface;
    VkRenderPass renderPass;
    std::vector<VkFramebuffer> framebuffers;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    SceneGraph scene;
};
