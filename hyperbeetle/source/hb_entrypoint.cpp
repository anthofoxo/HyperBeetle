#include "hb_window.hpp"

#include <atomic>
#include <thread>
#include <string_view>
#include <iostream>

#include <glad/gl.h>

// Avoid including entirty of the glfw headers
extern "C" int glfwWindowShouldClose(GLFWwindow* window);
extern "C" void glfwWaitEvents(void);
extern "C" void glfwPostEmptyEvent(void);
extern "C" typedef void (*hbglproc)(void);
extern "C" hbglproc glfwGetProcAddress(const char* procname);

#define HB_VERSION "v0.0.1-a.2+" __DATE__ " " __TIME__

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

int main(int argc, char* argv[]) {
	hyperbeetle::Window window({ .width = 1280, .height = 720, .title = "HyperBeetle " HB_VERSION });

	std::jthread thread = std::jthread([&]() {
		window.makeContextCurrent();

		if (!gladLoadGL(&glfwGetProcAddress))
			kRunning = false;

		if (GLAD_GL_KHR_debug) {
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
			glDebugMessageCallback(&message_callback, nullptr);
		}

		glClearColor(0, 0, 0, 0);

		while (kRunning) {
			glClear(GL_COLOR_BUFFER_BIT);
			window.swapBuffers();
		}

		hyperbeetle::Window().makeContextCurrent();

		glfwPostEmptyEvent();
	});

	while (kRunning) {
		glfwWaitEvents();
		
		if (glfwWindowShouldClose(window.handle()))
			kRunning = false;
	}

	thread.join();
}

#ifdef HB_ENTRY_WINMAIN
#include <Windows.h> // WINAPI, WinMain, _In_, _In_opt_, HINSTANCE
#include <stdlib.h> // __argc, __argv
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
	return main(__argc, __argv);
}
#endif