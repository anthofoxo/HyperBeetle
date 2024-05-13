#include "hb_window.hpp"

#include <atomic>
#include <thread>
#include <string_view>
#include <iostream>
#include <sstream>

#include <glad/gl.h>

// Avoid including entirty of the glfw headers
extern "C" int glfwWindowShouldClose(GLFWwindow* window);
extern "C" void glfwWaitEvents(void);
extern "C" void glfwPostEmptyEvent(void);
extern "C" typedef void (*hbglproc)(void);
extern "C" hbglproc glfwGetProcAddress(const char* procname);

#include <GLFW/glfw3.h>

#define HB_VERSION "v0.0.1-a.3+" __DATE__ " " __TIME__
#define HB_VERSION_FULL "HyperBeetle " HB_VERSION

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg.h>
#include <nanovg_gl.h>

namespace {
	std::atomic_bool kRunning = true;

	std::string_view glEnumToString(unsigned int val) {
		switch (val) {
		case GL_DEBUG_SOURCE_API: return "API";
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "WINDOW SYSTEM";
		case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER COMPILER";
		case GL_DEBUG_SOURCE_THIRD_PARTY: return "THIRD PARTY";
		case GL_DEBUG_SOURCE_APPLICATION: return "APPLICATION";
		case GL_DEBUG_SOURCE_OTHER: return "OTHER";

		case GL_DEBUG_TYPE_ERROR: return "ERROR";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED_BEHAVIOR";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "UNDEFINED_BEHAVIOR";
		case GL_DEBUG_TYPE_PORTABILITY: return "PORTABILITY";
		case GL_DEBUG_TYPE_PERFORMANCE: return "PERFORMANCE";
		case GL_DEBUG_TYPE_MARKER: return "MARKER";
		case GL_DEBUG_TYPE_OTHER: return "OTHER";

		case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFICATION";
		case GL_DEBUG_SEVERITY_LOW: return "LOW";
		case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
		case GL_DEBUG_SEVERITY_HIGH: return "HIGH";
		default: return "?";
		}
	}

	void message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param) {
		std::cout << glEnumToString(source) << ", " << glEnumToString(type) << ", " << glEnumToString(severity) << ", " << (void*)(intptr_t)id << ": " << message << std::endl;
	}
}

struct Application final {
	void runMainThread() {
		mWindow = hyperbeetle::Window({ .width = 1280, .height = 720, .title = "HyperBeetle"});

		glfwSetWindowUserPointer(mWindow.handle(), this);

		glfwGetWindowContentScale(mWindow.handle(), &contentScaleX, &contentScaleY);
		glfwGetFramebufferSize(mWindow.handle(), &framebufferWidth, &framebufferHeight);

		glfwSetFramebufferSizeCallback(mWindow.handle(), [](GLFWwindow* window, int width, int height) {
			auto& application = *static_cast<Application*>(glfwGetWindowUserPointer(window));
			application.framebufferWidth = width;
			application.framebufferHeight = height;
		});

		glfwSetWindowContentScaleCallback(mWindow.handle(), [](GLFWwindow* window, float xscale, float yscale) {
			auto& application = *static_cast<Application*>(glfwGetWindowUserPointer(window));
			application.contentScaleX = xscale;
			application.contentScaleY = yscale;
		});

		std::jthread thread = std::jthread(&Application::runRenderThread, this);

		while (kRunning) {
			glfwWaitEvents();

			if (glfwWindowShouldClose(mWindow.handle()))
				kRunning = false;
		}

		thread.join();
	}

	void runRenderThread() {
		mWindow.makeContextCurrent();

		if (!gladLoadGL(&glfwGetProcAddress))
			kRunning = false;

		if (GLAD_GL_KHR_debug) {
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
			glDebugMessageCallback(&message_callback, nullptr);
		}

		mVg = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_ANTIALIAS);

		int id = nvgCreateFont(mVg, "notosans-regular", "NotoSans-Regular.ttf");
		if (id != -1)
			nvgFontFaceId(mVg, id);

		glClearColor(0, 0, 0, 0);

		double lastTime = glfwGetTime();
		double currentTime;
		double deltaTime;

		while (kRunning) {
			currentTime = glfwGetTime();
			deltaTime = currentTime - lastTime;
			lastTime = currentTime;

			glViewport(0, 0, framebufferWidth, framebufferHeight);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			std::stringstream stream;
			stream << HB_VERSION_FULL << '\n';
			stream << deltaTime << '\n';
			stream << static_cast<int>(1.0 / deltaTime) << '\n';
			stream << framebufferWidth << 'x' << framebufferHeight << '\n';
			stream << contentScaleX << 'x' << contentScaleY;

			std::string str = stream.str();

			float width = static_cast<float>(framebufferWidth) / contentScaleX;
			float height = static_cast<float>(framebufferHeight) / contentScaleY;
			nvgBeginFrame(mVg, width, height, fmaxf(contentScaleX, contentScaleY));

			nvgTextAlign(mVg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
			nvgTextBox(mVg, 8, 8, width - 16, str.c_str(), nullptr);

			nvgEndFrame(mVg);

			mWindow.swapBuffers();
		}

		nvgDeleteGL3(mVg);

		hyperbeetle::Window().makeContextCurrent();

		glfwPostEmptyEvent();
	}

	int framebufferWidth, framebufferHeight;
	float contentScaleX, contentScaleY;

	hyperbeetle::Window mWindow;
	NVGcontext* mVg = nullptr;
};

int main(int argc, char* argv[]) {
	Application application;
	application.runMainThread();
}

#ifdef HB_ENTRY_WINMAIN
#include <Windows.h> // WINAPI, WinMain, _In_, _In_opt_, HINSTANCE
#include <stdlib.h> // __argc, __argv
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
	return main(__argc, __argv);
}
#endif