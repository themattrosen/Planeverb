#include "Graphics.h"
#include <glad.h>
#include <glfw3.h>
#include <PvDefinitions.h>
#include <iostream>

void Graphics::Init()
{
	bool res = glfwInit();
	if (!res)
	{
		std::cout << "GLFW Init failed!" << std::endl;
		return;
	}
	m_window = glfwCreateWindow(640, 480, "Planeverb Sandbox", NULL, NULL);
	if (!m_window)
	{
		glfwTerminate();
		std::cout << "Failed to create a window" << std::endl;
		PV_ASSERT(false);
		return;
	}

	glfwMakeContextCurrent(m_window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		glfwTerminate();
		std::cout << "Failed to initialize GLAD" << std::endl;
		PV_ASSERT(false);
	}
}

void Graphics::Update()
{
	glfwPollEvents();
	glClear(GL_COLOR_BUFFER_BIT);
}

void Graphics::Draw()
{
	glfwMakeContextCurrent(m_window);
	glfwSwapBuffers(m_window);
}

void Graphics::Exit()
{
	glfwTerminate();
}

bool Graphics::IsRunning() const
{
	return !glfwWindowShouldClose(m_window);
}
