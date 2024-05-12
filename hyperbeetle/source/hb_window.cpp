#include "hb_window.hpp"

#include <GLFW/glfw3.h>

#include <stdexcept>

namespace hyperbeetle {
	namespace {
		int kNumWindows = 0;
	}

	Window::Window(CreateInfo const& info) {
		if (kNumWindows == 0) {
			if (!glfwInit()) {
				throw std::runtime_error("Failed to initialize glfw");
			}
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

		mHandle = glfwCreateWindow(info.width, info.height, info.title, nullptr, nullptr);
		if (!mHandle) {
			if (kNumWindows == 0)
				glfwTerminate();

			throw std::runtime_error("Failed to create window");
		}

		++kNumWindows;
	}
		
	Window& Window::operator=(Window&& other) noexcept {
		std::swap(mHandle, other.mHandle);
		return *this;
	}

	Window::~Window() noexcept {
		if (mHandle) {
			glfwDestroyWindow(mHandle);
			--kNumWindows;

			if(kNumWindows == 0)
				glfwTerminate();
		}
	}

	void Window::swapBuffers() const {
		glfwSwapBuffers(mHandle);
	}

	void Window::makeContextCurrent() const {
		glfwMakeContextCurrent(mHandle);
	}

}