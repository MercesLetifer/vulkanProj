#pragma once

#include "vdeleter.h"

class GLFWwindow;

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
	void mainLoop();
	void createInstance();

private:
	const int WIDTH = 800;
	const int HEIGHT = 600;
	GLFWwindow* window;
	
	VDeleter<VkInstance> instance{ vkDestroyInstance };
};