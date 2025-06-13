#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <iostream>

#include <vector>
#include <memory>

// Scenegraph setup (from earlier)
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

    if (!inst_ret) {
        std::cerr << "Failed to create Vulkan instance\n";
        return 1;
    }

    auto inst = inst_ret.value();
    VkSurfaceKHR surface;
    glfwCreateWindowSurface(inst.instance, window, nullptr, &surface);

    vkb::PhysicalDeviceSelector selector{ inst };
    auto phys_ret = selector.set_surface(surface).select();
    auto dev_ret = vkb::DeviceBuilder{ phys_ret.value() }.build();
    if (!dev_ret) {
        std::cerr << "Failed to create device\n";
        return 1;
    }

    auto device = dev_ret.value();
    VkQueue queue = device.get_queue(vkb::QueueType::graphics).value();
//    VkCommandPool pool = device.get_command_pool().value();
VkCommandPoolCreateInfo poolInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = device.get_queue_index(vkb::QueueType::graphics).value()
};
VkCommandPool pool;
vkCreateCommandPool(device.device, &poolInfo, nullptr, &pool);

    // Allocate one command buffer for stub use
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    VkCommandBuffer cmdBuf;
    vkAllocateCommandBuffers(device.device, &allocInfo, &cmdBuf);

    // Scenegraph stub
    auto root = std::make_shared<Group>("root");
    root->add(std::make_shared<Drawable>("cube"));
    RenderContext ctx{ cmdBuf };

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        root->traverse(ctx);  // Just printing for now
    }

//    device.destroy();
//    inst.destroy();
vkDestroyCommandPool(device.device, pool, nullptr);
vkDestroyDevice(device.device, nullptr);
vkDestroySurfaceKHR(inst.instance, surface, nullptr);
vkDestroyInstance(inst.instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
