#include "application.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>

void Application::initWindow()
{
	if (!glfwInit())
		throw std::runtime_error("glfwInit() error!");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void Application::mainLoop()
{
	while (!glfwWindowShouldClose(window))
		glfwPollEvents();
}
