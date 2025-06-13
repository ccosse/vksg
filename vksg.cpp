#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <iostream>
#include <vector>
#include <memory>

struct RenderContext {
    VkCommandBuffer cmdBuf;
};

struct Node {
    virtual void traverse(RenderContext& ctx) = 0;
    virtual ~Node() = default;
};

using NodePtr = std::shared_ptr<Node>;

struct Drawable : Node {
    std::string name;
    Drawable(const std::string& n) : name(n) {}
    void traverse(RenderContext& ctx) override {
        std::cout << "Drawing: " << name << "\n";
    }
};

struct Group : Node {
    std::string name;
    std::vector<NodePtr> children;
    Group(const std::string& n) : name(n) {}
    void add(const NodePtr& c) { children.push_back(c); }
    void traverse(RenderContext& ctx) override {
        std::cout << "Group: " << name << "\n";
        for (auto& c : children) c->traverse(ctx);
    }
};

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vk2025 Scenegraph", nullptr, nullptr);

    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("vk2025")
        .request_validation_layers()
        .use_default_debug_messenger()
        .require_api_version(1, 1, 0)
        .build();

    auto inst = inst_ret.value();
    VkSurfaceKHR surface;
    glfwCreateWindowSurface(inst.instance, window, nullptr, &surface);

    vkb::PhysicalDeviceSelector selector{ inst };
    auto phys_ret = selector.set_surface(surface).select();
    auto dev_ret = vkb::DeviceBuilder{ phys_ret.value() }.build();
    auto device = dev_ret.value();
    VkQueue queue = device.get_queue(vkb::QueueType::graphics).value();

    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = device.get_queue_index(vkb::QueueType::graphics).value()
    };
    VkCommandPool pool;
    vkCreateCommandPool(device.device, &poolInfo, nullptr, &pool);

    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    VkCommandBuffer cmdBuf;
    vkAllocateCommandBuffers(device.device, &allocInfo, &cmdBuf);

    vkb::SwapchainBuilder swapBuilder{ device };
    auto swap_ret = swapBuilder.set_old_swapchain(VK_NULL_HANDLE).build();
    auto swap = swap_ret.value();

    VkFormat format = swap.image_format;
    VkExtent2D extent = swap.extent;

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkRenderPass renderPass;
    vkCreateRenderPass(device.device, &renderPassInfo, nullptr, &renderPass);

    std::vector<VkFramebuffer> framebuffers;
    for (VkImageView view : swap.get_image_views().value()) {
        VkFramebufferCreateInfo fbInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        fbInfo.renderPass = renderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &view;
        fbInfo.width = extent.width;
        fbInfo.height = extent.height;
        fbInfo.layers = 1;

        VkFramebuffer fb;
        vkCreateFramebuffer(device.device, &fbInfo, nullptr, &fb);
        framebuffers.push_back(fb);
    }

    auto root = std::make_shared<Group>("root");
    root->add(std::make_shared<Drawable>("cube"));
    RenderContext ctx{ cmdBuf };

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        uint32_t imageIndex;
        vkAcquireNextImageKHR(device.device, swap.swapchain, UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &imageIndex);

        VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        vkBeginCommandBuffer(cmdBuf, &beginInfo);

        VkClearValue clearColor = { {{0.1f, 0.1f, 0.2f, 1.0f}} };
        VkRenderPassBeginInfo rpBegin{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        rpBegin.renderPass = renderPass;
        rpBegin.framebuffer = framebuffers[imageIndex];
        rpBegin.renderArea.extent = extent;
        rpBegin.clearValueCount = 1;
        rpBegin.pClearValues = &clearColor;

        vkCmdBeginRenderPass(cmdBuf, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
        root->traverse(ctx);
        vkCmdEndRenderPass(cmdBuf);
        vkEndCommandBuffer(cmdBuf);

        VkSubmitInfo submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmdBuf;
        vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE);

        VkPresentInfoKHR present{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        present.swapchainCount = 1;
        present.pSwapchains = &swap.swapchain;
        present.pImageIndices = &imageIndex;
        vkQueuePresentKHR(queue, &present);

        vkQueueWaitIdle(queue);
    }

    for (auto fb : framebuffers) vkDestroyFramebuffer(device.device, fb, nullptr);
    vkDestroyRenderPass(device.device, renderPass, nullptr);
    vkDestroySwapchainKHR(device.device, swap.swapchain, nullptr);
    vkDestroyCommandPool(device.device, pool, nullptr);
    vkDestroyDevice(device.device, nullptr);
    vkDestroySurfaceKHR(inst.instance, surface, nullptr);
    vkDestroyInstance(inst.instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
