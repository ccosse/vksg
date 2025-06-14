#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VkApp.hpp"
#include <stdexcept>
#include <array>

void VkApp::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void VkApp::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(800, 600, "VkApp", nullptr, nullptr);
}

void VkApp::initVulkan() {
    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("VkApp")
        .request_validation_layers()
        .use_default_debug_messenger()
        .require_api_version(1, 1, 0)
        .build();
    if (!inst_ret) throw std::runtime_error("Failed to create instance");
    vkbInstance = inst_ret.value();

    if (glfwCreateWindowSurface(vkbInstance.instance, window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface");

    vkb::PhysicalDeviceSelector selector{vkbInstance};
    auto phys_ret = selector.set_surface(surface).select();
    auto dev_ret = vkb::DeviceBuilder{phys_ret.value()}.build();
    if (!dev_ret) throw std::runtime_error("Failed to create device");
    vkbDevice = dev_ret.value();

    auto swap_ret = vkb::SwapchainBuilder{vkbDevice}
        .set_old_swapchain({})
        .set_desired_format({})
        .build();
    if (!swap_ret) throw std::runtime_error("Failed to create swapchain");
    vkbSwapchain = swap_ret.value();

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = vkbSwapchain.image_format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(vkbDevice.device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        throw std::runtime_error("Failed to create render pass");

    auto views = vkbSwapchain.get_image_views().value();
    for (VkImageView view : views) {
        VkImageView attachments[] = {view};
        VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fbInfo.renderPass = renderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = attachments;
        fbInfo.width = vkbSwapchain.extent.width;
        fbInfo.height = vkbSwapchain.extent.height;
        fbInfo.layers = 1;

        VkFramebuffer fb;
        if (vkCreateFramebuffer(vkbDevice.device, &fbInfo, nullptr, &fb) != VK_SUCCESS)
            throw std::runtime_error("Failed to create framebuffer");
        framebuffers.push_back(fb);
    }

    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(vkbDevice.device, &poolInfo, nullptr, &commandPool);

    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(vkbDevice.device, &allocInfo, &commandBuffer);
}

void VkApp::mainLoop() {
    auto queue = vkbDevice.get_queue(vkb::QueueType::graphics).value();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        uint32_t imageIndex;
        vkAcquireNextImageKHR(vkbDevice.device, vkbSwapchain.swapchain, UINT64_MAX,
                              VK_NULL_HANDLE, VK_NULL_HANDLE, &imageIndex);

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkClearValue clearColor = {{{0.0f, 0.1f, 0.2f, 1.0f}}};
        VkRenderPassBeginInfo rpInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rpInfo.renderPass = renderPass;
        rpInfo.framebuffer = framebuffers[imageIndex];
        rpInfo.renderArea.extent = vkbSwapchain.extent;
        rpInfo.clearValueCount = 1;
        rpInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

        RenderContext ctx{commandBuffer};
        scene.traverse(ctx);

        vkCmdEndRenderPass(commandBuffer);
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &vkbSwapchain.swapchain;
        presentInfo.pImageIndices = &imageIndex;
        vkQueuePresentKHR(queue, &presentInfo);
    }
}

void VkApp::cleanup() {
    for (auto fb : framebuffers)
        vkDestroyFramebuffer(vkbDevice.device, fb, nullptr);
    vkDestroyRenderPass(vkbDevice.device, renderPass, nullptr);
    vkDestroyCommandPool(vkbDevice.device, commandPool, nullptr);
    vkDestroyDevice(vkbDevice.device, nullptr);
    vkDestroySurfaceKHR(vkbInstance.instance, surface, nullptr);
    vkDestroyInstance(vkbInstance.instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}