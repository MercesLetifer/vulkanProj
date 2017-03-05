#pragma once

#include "vdeleter.h"
#include <vector>

class GLFWwindow;

struct QueueFamilyIndices {
	int graphicsFamily = -1;

	bool isComplete() { 
		return graphicsFamily >= 0; 
	}
};

VkResult CreateDebugReportCallbackEXT(VkInstance instance, 
	const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);

void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, 
	const VkAllocationCallbacks* pAllocator);

class Application {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
	}

private:
	void initWindow();
	void initVulkan();
	void setupDebugCallback();
	void mainLoop();
	bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions();
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData
	);

	void createInstance();
	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

private:
	const int WIDTH = 800;
	const int HEIGHT = 600;

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_LUNARG_standard_validation"
	};

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	GLFWwindow* window;
	VDeleter<VkInstance> instance{ vkDestroyInstance };
	VDeleter<VkDebugReportCallbackEXT> callback{ instance, DestroyDebugReportCallbackEXT };
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
};