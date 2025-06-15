//#include "VkApp.hpp"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <cstring>
#include <memory>
#include "SceneGraph.hpp"

#define VK_CALL(x) do { \
    VkResult res = (x); \
    if (res != VK_SUCCESS) throw std::runtime_error("Vulkan call failed: " #x); \
} while(0)

class VkApp {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    uint32_t graphicsQueueFamily = UINT32_MAX;

    VkSwapchainKHR swapchain;
    VkFormat swapchainFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> imageViews;
    std::shared_ptr<Group> group; 

    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(800, 600, "Vulkan Window", nullptr, nullptr);
    }

    void initVulkan() {
        // Instance
        VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        appInfo.pApplicationName = "VkApp";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "NoEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        createInfo.pApplicationInfo = &appInfo;

        uint32_t glfwExtCount = 0;
        const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
        createInfo.enabledExtensionCount = glfwExtCount;
        createInfo.ppEnabledExtensionNames = glfwExts;

        VK_CALL(vkCreateInstance(&createInfo, nullptr, &instance));

        if (!instance) throw std::runtime_error("Instance is null!");

        // Surface
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
            throw std::runtime_error("Failed to create window surface");
        if (!surface) throw std::runtime_error("Surface is null!");

        // Physical Device
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) throw std::runtime_error("No Vulkan devices found");

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        physicalDevice = devices[0];
        if (!physicalDevice) throw std::runtime_error("PhysicalDevice is null!");

        // Queue family
        uint32_t queueCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueProps(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());

        for (uint32_t i = 0; i < queueCount; ++i) {
            if (queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                VkBool32 supportsPresent = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent);
                if (supportsPresent) {
                    graphicsQueueFamily = i;
                    break;
                }
            }
        }
        if (graphicsQueueFamily == UINT32_MAX) throw std::runtime_error("No suitable queue family");

        // Logical device
        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreate{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueCreate.queueFamilyIndex = graphicsQueueFamily;
        queueCreate.queueCount = 1;
        queueCreate.pQueuePriorities = &queuePriority;

        VkDeviceCreateInfo devCreate{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
const char* requiredExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        devCreate.enabledExtensionCount = 1;
        devCreate.ppEnabledExtensionNames = requiredExtensions;
        devCreate.queueCreateInfoCount = 1;
        devCreate.pQueueCreateInfos = &queueCreate;

        VK_CALL(vkCreateDevice(physicalDevice, &devCreate, nullptr, &device));
        if (!device) throw std::runtime_error("Device is null!");

        vkGetDeviceQueue(device, graphicsQueueFamily, 0, &graphicsQueue);

        // Swapchain
        VkSurfaceCapabilitiesKHR surfaceCaps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

        VkSurfaceFormatKHR surfaceFormat = formats[0];
        swapchainFormat = surfaceFormat.format;

        VkSwapchainCreateInfoKHR swapInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        swapInfo.surface = surface;
        swapInfo.minImageCount = surfaceCaps.minImageCount;
        swapInfo.imageFormat = surfaceFormat.format;
        swapInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapInfo.imageExtent = surfaceCaps.currentExtent;
        swapInfo.imageArrayLayers = 1;
        swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapInfo.preTransform = surfaceCaps.currentTransform;
        swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        swapInfo.clipped = VK_TRUE;
        swapInfo.oldSwapchain = VK_NULL_HANDLE;

        VK_CALL(vkCreateSwapchainKHR(device, &swapInfo, nullptr, &swapchain));
        swapchainExtent = surfaceCaps.currentExtent;

        uint32_t imageCount = 0;
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
        swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

        imageViews.resize(imageCount);
        for (size_t i = 0; i < imageCount; ++i) {
            VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            viewInfo.image = swapchainImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = swapchainFormat;
            viewInfo.components = {
                VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY
            };
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.layerCount = 1;

            VK_CALL(vkCreateImageView(device, &viewInfo, nullptr, &imageViews[i]));
        }
        group = std::make_shared<Group>("root");
        group->add(std::make_shared<Geometry>("cube"));

    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            //group->traverse();
        }
    }

    void cleanup() {
        for (auto view : imageViews)
            vkDestroyImageView(device, view, nullptr);
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main() {
    VkApp app;
    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}