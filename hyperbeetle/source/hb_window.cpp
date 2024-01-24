#include "hb_window.hpp"

#include <stdexcept>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <tracy/Tracy.hpp>
#include <spdlog/spdlog.h>

namespace hyperbeetle {
	namespace {
		unsigned short kWindowCount = 0;

		void ErrorCallback(int error, char const* description) {
			spdlog::error("GLFW Error {}: {}", error, description);
		}
	}

	void Window::PollEvents() {
		glfwPollEvents();
	}

	Window::Window(CreateInfo const& info) {
		if (!glfwSetErrorCallback)
			glfwSetErrorCallback(&ErrorCallback);

		if (!glfwInit())
			throw std::runtime_error("Glfw failed to initialize");

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_SAMPLES, 32);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

		try {
			mHandle = glfwCreateWindow(info.width, info.height, info.title, nullptr, nullptr);
			if (!mHandle)
				throw std::runtime_error("Glfw failed to create window");
			
			glfwMakeContextCurrent(mHandle);
			if (!gladLoadGL(&glfwGetProcAddress))
				throw std::runtime_error("Glad failed to load OpenGL function pointers");	

			++kWindowCount;
		}
		catch (std::runtime_error const& e) {
			if (mHandle) {
				glfwMakeContextCurrent(nullptr);
				glfwDestroyWindow(mHandle);
				mHandle = nullptr;
			}
			
			if (kWindowCount == 0)
				glfwTerminate();

			throw;
		}
	}

	Window& Window::operator=(Window&& other) noexcept {
		std::swap(mHandle, other.mHandle);
		return *this;
	}

	Window::~Window() noexcept {
		if (mHandle) {
			glfwMakeContextCurrent(nullptr);
			glfwDestroyWindow(mHandle);
			--kWindowCount;
			if (kWindowCount == 0)
				glfwTerminate();
		}
	}

	void Window::SwapBuffers() const {
		ZoneScoped;
		glfwSwapBuffers(mHandle);
	}
}