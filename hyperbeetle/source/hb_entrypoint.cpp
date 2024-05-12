#include "hb_window.hpp"

#include <atomic>
#include <thread>

extern "C" int glfwWindowShouldClose(GLFWwindow* window);
extern "C" void glfwWaitEvents(void);
extern "C" void glfwPostEmptyEvent(void);

static std::atomic_bool kRunning = true;

#define HB_VERSION "v0.0.1-a.1+" __DATE__ " " __TIME__

int main(int argc, char* argv[]) {
	hyperbeetle::Window window({ .width = 1280, .height = 720, .title = "HyperBeetle " HB_VERSION });

	std::jthread thread = std::jthread([]() {
		while (kRunning) {
		}
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