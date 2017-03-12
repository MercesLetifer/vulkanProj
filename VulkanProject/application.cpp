#include "application.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <iostream>
#include <set>
#include <algorithm>
#include <fstream>

void Application::initWindow()
{
	if (!glfwInit())
		throw std::runtime_error("Failed to init GLFW!");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void Application::initVulkan()
{
	createInstance();
	setupDebugCallback();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
}

void Application::setupDebugCallback()
{
	if (!enableValidationLayers) return;

	VkDebugReportCallbackCreateInfoEXT createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = debugCallback;

	if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, callback.replace()) != VK_SUCCESS)
		throw std::runtime_error("failed to set up debug callback!");
}

void Application::mainLoop()
{
	while (!glfwWindowShouldClose(window))
		glfwPollEvents();

	glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::createInstance()
{
	if (enableValidationLayers && !checkValidationLayerSupport())
		throw std::runtime_error("Validation layer requested, but not available!");

	VkApplicationInfo appInfo = { };
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto requiredExtensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = requiredExtensions.size();
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}
	
	// Create Vulkan instance
	if (vkCreateInstance(&createInfo, nullptr, instance.replace()) != VK_SUCCESS)
		throw std::runtime_error("Failed to create instance!");


//	uint32_t extensionCount = 0;
//	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
//	std::vector<VkExtensionProperties> extensions(extensionCount);
//	
//	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
//	
//	std::cout << "vk instance extensions: " << std::endl;
//	for (const auto& ext : extensions)
//		std::cout << ext.extensionName << std::endl;
//	std::cout << std::endl;
}

void Application::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (!deviceCount)
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device : devices)
		if (isDeviceSuitable(device)) {
			physicalDevice = device;
			return;
		}

	if (physicalDevice == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to find suitable GPU!");
}

void Application::createSurface()
{
	if (glfwCreateWindowSurface(instance, window, nullptr, surface.replace()) != VK_SUCCESS)
		throw std::runtime_error("Failed to create window surface!");
}

bool Application::isDeviceSuitable(VkPhysicalDevice device)
{
	// TODO: rewrite
	auto indices = findQueueFamilies(device);

	auto extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		auto swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return indices.isComplete() && swapChainAdequate;
}

QueueFamilyIndices Application::findQueueFamilies(VkPhysicalDevice device)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	// TODO: rewrite
	QueueFamilyIndices indices;
	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
			indices.graphicsFamily = i;
			
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (queueFamily.queueCount > 0 && presentSupport)
			indices.presentFamily = i;

		if (indices.isComplete())
			break;

		++i;
	}
	
	return indices;
}

void Application::createLogicalDevice()
{
	auto indices = findQueueFamilies(physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

	float queuePriority = 1.0f;
	for (const auto& queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = { };
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}
	
	VkPhysicalDeviceFeatures deviceFeatures = { };

	VkDeviceCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = deviceExtensions.size();
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, device.replace()) != VK_SUCCESS)
		throw std::runtime_error("Failed to create logical device!");

	vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
}

void Application::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
		imageCount = swapChainSupport.capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;		// opt
		createInfo.pQueueFamilyIndices = nullptr;	// opt
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, swapChain.replace()) != VK_SUCCESS)
		throw std::runtime_error("Failed to create swap chain!");

	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void Application::createImageViews()
{
	swapChainImageViews.resize(swapChainImages.size(), VDeleter<VkImageView> {device, vkDestroyImageView});
	
	for (uint32_t i = 0; i < swapChainImages.size(); ++i) {
		VkImageViewCreateInfo createInfo = { };
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &createInfo, nullptr, swapChainImageViews[0].replace()) != VK_SUCCESS)
			throw std::runtime_error("failed to create image views!");
	}
}

void Application::createGraphicsPipeline()
{
	auto vertShaderCode = readFile("vert.spv");
	auto fragShaderCode = readFile("frag.spv");

	VDeleter<VkShaderModule> vertShaderModule{ device, vkDestroyShaderModule };
	VDeleter<VkShaderModule> fragShaderModule{ device, vkDestroyShaderModule };

	createShaderModule(vertShaderCode, vertShaderModule);
	createShaderModule(fragShaderCode, fragShaderModule);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = { };
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	vertShaderStageInfo.module = fragShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
}

void Application::createShaderModule(const std::vector<char>& code, VDeleter<VkShaderModule>& shaderModule)
{
	VkShaderModuleCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = (uint32_t*)code.data();

	if (vkCreateShaderModule(device, &createInfo, nullptr, shaderModule.replace()) != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module");
}

bool Application::checkValidationLayerSupport()
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> layers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

	for (const auto& layerName : validationLayers) 
		for (const auto& layerProperties : layers) 
			if (strcmp(layerName, layerProperties.layerName) == 0)
				return true;

	return false;
}

bool Application::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	// TODO: rewrite
	std::set<std::string> requiredExtensions(deviceExtensions.cbegin(), deviceExtensions.cend());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

SwapChainSupportDetails Application::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR Application::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };

	for (const auto& availableFormat : availableFormats)
		if (availableFormat.format == VK_FORMAT_B8G8R8_UNORM && availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
			return availableFormat;

	return availableFormats[0];
}

VkPresentModeKHR Application::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
{
	for (const auto& availableMode : availablePresentModes)
		if (availableMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return availableMode;

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Application::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;
	else {
		VkExtent2D actualExtent = { (uint32_t)WIDTH, (uint32_t)HEIGHT };
		actualExtent.width = std::max(capabilities.minImageExtent.width,
			std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height,
			std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

std::vector<const char*> Application::getRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions;
	for (uint32_t i = 0; i < glfwExtensionCount; ++i)
		extensions.push_back(glfwExtensions[i]);

	if (enableValidationLayers)
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Application::debugCallback(
	VkDebugReportFlagsEXT flags, 
	VkDebugReportObjectTypeEXT objType, 
	uint64_t obj, 
	size_t location, 
	int32_t code, 
	const char* layerPrefix, 
	const char* msg, 
	void* userData)
{
	std::cerr << "validation layer: " << msg << std::endl;

	return VK_FALSE;
}

std::vector<char> Application::readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("Failed to open file!");

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDebugReportCallbackEXT * pCallback)
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr)
		return func(instance, pCreateInfo, pAllocator, pCallback);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks * pAllocator)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}
