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
}