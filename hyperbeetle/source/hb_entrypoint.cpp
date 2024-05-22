#include "hb_window.hpp"

#include <atomic>
#include <thread>
#include <string_view>
#include <iostream>
#include <sstream>
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>
#include <vector>
#include <fstream>
#include <mutex>
#include <string>

#include <yaml-cpp/yaml.h>

#include <entt/entt.hpp>

#include <glad/gl.h>

#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <GLFW/glfw3.h>

#define HB_VERSION "v0.0.1-a.4+" __DATE__ " " __TIME__
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

struct AudioEngine final {

	void init(std::string_view preferredDevice) {
		if (ma_context_init(nullptr, 0, nullptr, &mContext) != MA_SUCCESS) {
			// Error.
		}

		ma_device_info* pPlaybackInfos;
		ma_uint32 playbackCount;
		if (ma_context_get_devices(&mContext, &pPlaybackInfos, &playbackCount, nullptr, nullptr) != MA_SUCCESS) {
			// Error.
		}

		bool isDeviceChosen = false;
		ma_uint32 chosenPlaybackDeviceIndex = 0;

		for (ma_uint32 iDevice = 0; iDevice < playbackCount; iDevice += 1) {
			if (std::string_view(pPlaybackInfos[iDevice].name) == preferredDevice) {
				chosenPlaybackDeviceIndex = iDevice;
				isDeviceChosen = true;
			}
		}

		if (!isDeviceChosen) {
			for (ma_uint32 iDevice = 0; iDevice < playbackCount; iDevice += 1) {
				if (pPlaybackInfos[iDevice].isDefault) {
					chosenPlaybackDeviceIndex = iDevice;
					isDeviceChosen = true;
					break;
				}
			}
		}

		mDeviceName = pPlaybackInfos[chosenPlaybackDeviceIndex].name;

		ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
		deviceConfig.playback.pDeviceID = &pPlaybackInfos[chosenPlaybackDeviceIndex].id;
		deviceConfig.pUserData = &mEngine;

		deviceConfig.dataCallback = [](ma_device* pDevice, void* pOutput, void const* pInput, ma_uint32 frameCount) {
			ma_engine_read_pcm_frames(static_cast<ma_engine*>(pDevice->pUserData), pOutput, frameCount, nullptr);
		};

		if (ma_device_init(&mContext, &deviceConfig, &mDevice) != MA_SUCCESS) {
			// Error.
		}

		ma_engine_config engineConfig = ma_engine_config_init();
		engineConfig.pDevice = &mDevice;

		if (ma_engine_init(&engineConfig, &mEngine) != MA_SUCCESS) {
			// Error.
		}
	}

	void uninit() {
		ma_engine_uninit(&mEngine);
		ma_device_uninit(&mDevice);
		ma_context_uninit(&mContext);
	}

	std::string mDeviceName;
	ma_context mContext;
	ma_device mDevice;
	ma_engine mEngine;
};

struct EventKey final {
	GLFWwindow* window;
	int key, scancode, action, mods;
};

#include <renderdoc_app.h>

static RENDERDOC_API_1_0_0* kRdocApi = nullptr;

#ifdef _WIN32
#	include <Windows.h>

void setupRenderdoc() {

	HMODULE module = GetModuleHandleA("renderdoc");
	bool manuallyAttached = module != nullptr;
#ifndef HB_DIST
	if (module == nullptr) module = LoadLibraryA("C:/Program Files/RenderDoc/renderdoc.dll");
#endif
	if (module == nullptr) return;
	
	pRENDERDOC_GetAPI getApi = reinterpret_cast<pRENDERDOC_GetAPI>(GetProcAddress(module, "RENDERDOC_GetAPI"));
	if (getApi == nullptr) return;

	int ret = getApi(eRENDERDOC_API_Version_1_0_0, reinterpret_cast<void**>(&kRdocApi));
	if (!ret) return;
	if (!kRdocApi) return;

	kRdocApi->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None);
	kRdocApi->SetCaptureOptionU32(eRENDERDOC_Option_DebugOutputMute, false);
	kRdocApi->SetCaptureOptionU32(eRENDERDOC_Option_APIValidation, true);
	kRdocApi->SetCaptureOptionU32(eRENDERDOC_Option_CaptureCallstacks, true);
	kRdocApi->SetCaptureOptionU32(eRENDERDOC_Option_VerifyBufferAccess, true);
	kRdocApi->SetCaptureOptionU32(eRENDERDOC_Option_RefAllResources, true);

	int major, minor, patch;
	kRdocApi->GetAPIVersion(&major, &minor, &patch);
	std::cout << "Requested 1.0.0, Got " << major << "." << minor << "." << patch << "\n";

	if (!manuallyAttached) {
		RENDERDOC_InputButton keys[] = { eRENDERDOC_Key_F10 };
		kRdocApi->SetCaptureKeys(keys, 1);
	}
}
#else
void setupRenderdoc() {}
#endif

#if 0
struct StateManager;

struct State {
	State() = default;
	virtual ~State() = default;
	State(State const&) = delete;
	State& operator=(State const&) = delete;
	State(State&&) = delete;
	State& operator=(State&&) = delete;

	virtual void update() = 0;
};

struct StateManager final {
	std::vector<std::unique_ptr<State>> mStates;

	template<class T, class... Args, std::enable_if_t<std::is_base_of_v<State, T>, bool> = true>
	void push(Args&&... args) {
		mStates.push_back(std::make_unique<T>(std::forward<Args>(args)...));
	}

	void update() {
		for (auto& state : mStates)
			state->update();
	}
};
#endif

struct MenuState final {
	void init();
	void update();
	void destroy();

	void onKey(EventKey const& e);

	bool mPerformAction = false;
	int mSelectedOption = 0;
};

struct Application final {
	void runMainThread();
	void runRenderThread();

	int mFramebufferWidth, mFramebufferHeight;
	float mContentScaleX, mContentScaleY;

	float mUiWidth, mUiHeight;

	double mDeltaTime = 1.;

	hyperbeetle::Window mWindow;
	AudioEngine mAudioEngine;
	NVGcontext* mVg = nullptr;

	std::mutex mDispatcherMtx;
	entt::dispatcher mDispatcher{};

	MenuState mState;
};

Application& getApplication();

void Application::runMainThread() {
	setupRenderdoc();

	// Read config
	std::string configuredAudioDevice = "";
	try {
		YAML::Node config = YAML::LoadFile("config.yaml");
		configuredAudioDevice = config["audioDevice"].as<std::string>("");
	}
	catch (YAML::BadFile const& e) {}

	mWindow = hyperbeetle::Window({ .width = 1280, .height = 720, .title = "HyperBeetle" });

	mAudioEngine.init(configuredAudioDevice);

	glfwSetWindowUserPointer(mWindow.handle(), this);

	glfwGetWindowContentScale(mWindow.handle(), &mContentScaleX, &mContentScaleY);
	glfwGetFramebufferSize(mWindow.handle(), &mFramebufferWidth, &mFramebufferHeight);

	glfwSetKeyCallback(mWindow.handle(), [](GLFWwindow* window, int key, int scancode, int action, int mods) {
		auto& application = *static_cast<Application*>(glfwGetWindowUserPointer(window));
		std::lock_guard lck{ application.mDispatcherMtx };
		application.mDispatcher.enqueue<EventKey>(window, key, scancode, action, mods);
	});

	glfwSetFramebufferSizeCallback(mWindow.handle(), [](GLFWwindow* window, int width, int height) {
		auto& application = *static_cast<Application*>(glfwGetWindowUserPointer(window));
		application.mFramebufferWidth = width;
		application.mFramebufferHeight = height;
	});

	glfwSetWindowContentScaleCallback(mWindow.handle(), [](GLFWwindow* window, float xscale, float yscale) {
		auto& application = *static_cast<Application*>(glfwGetWindowUserPointer(window));
		application.mContentScaleX = xscale;
		application.mContentScaleY = yscale;
	});

	std::jthread thread = std::jthread(&Application::runRenderThread, this);

	while (kRunning) {
		glfwWaitEvents();

		if (glfwWindowShouldClose(mWindow.handle()))
			kRunning = false;
	}

	thread.join();

	mAudioEngine.uninit();
}

void Application::runRenderThread() {
	mWindow.makeContextCurrent();

	if (!gladLoadGL(&glfwGetProcAddress))
		kRunning = false;

	if (GLAD_GL_KHR_debug) {
		glEnable(GL_DEBUG_OUTPUT);
#ifndef HB_DIST
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif
		glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
		glDebugMessageCallback(&message_callback, nullptr);
	}

	mVg = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_ANTIALIAS);

	int id = nvgCreateFont(mVg, "notosans-regular", "NotoSans-Regular.ttf");
	if (id != -1)
		nvgFontFaceId(mVg, id);

	unsigned int vao;
	glGenVertexArrays(1, &vao);

	glClearColor(0, 0, 0, 0);

	mState.init();

	double lastTime = glfwGetTime();
	double currentTime;

	while (kRunning) {
		{
			std::lock_guard lck{ mDispatcherMtx };
			mDispatcher.update();
		}

		currentTime = glfwGetTime();
		mDeltaTime = currentTime - lastTime;
		lastTime = currentTime;

		mUiWidth = static_cast<float>(mFramebufferWidth) / mContentScaleX;
		mUiHeight = static_cast<float>(mFramebufferHeight) / mContentScaleY;
		nvgBeginFrame(mVg, mUiWidth, mUiHeight, fmaxf(mContentScaleX, mContentScaleY));

		mState.update();

		nvgEndFrame(mVg);
		
		mWindow.swapBuffers();
	}

	mState.destroy();

	glDeleteVertexArrays(1, &vao);

	nvgDeleteGL3(mVg);

	hyperbeetle::Window().makeContextCurrent();

	glfwPostEmptyEvent();
}

void MenuState::onKey(EventKey const& e) {
	auto& application = getApplication();

	if (e.action != GLFW_PRESS) return;

	if (e.key == GLFW_KEY_ENTER)
		mPerformAction = true;

	if (e.key == GLFW_KEY_DOWN) {
		++mSelectedOption;
		ma_engine_play_sound(&application.mAudioEngine.mEngine, "cursor1.ogg", nullptr);
	}

	if (e.key == GLFW_KEY_UP) {
		--mSelectedOption;
		ma_engine_play_sound(&application.mAudioEngine.mEngine, "cursor1.ogg", nullptr);
	}
}

void MenuState::init() {
	auto& application = getApplication();
	std::lock_guard lck{ application.mDispatcherMtx };
	application.mDispatcher.sink<EventKey>().connect<&MenuState::onKey>(this);
}

void MenuState::update() {
	auto& application = getApplication();

	glViewport(0, 0, application.mFramebufferWidth, application.mFramebufferHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	std::stringstream stream;
	stream << HB_VERSION_FULL << '\n';
	stream << application.mDeltaTime << '\n';
	stream << static_cast<int>(1.0 / application.mDeltaTime) << '\n';
	stream << application.mFramebufferWidth << 'x' << application.mFramebufferHeight << '\n';
	stream << application.mContentScaleX << 'x' << application.mContentScaleY << '\n';
	stream << application.mAudioEngine.mDeviceName;

	if (kRdocApi) {
		stream << "\nRenderdoc attached";
	}

	std::string str = stream.str();

	

	nvgTextAlign(application.mVg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
	nvgTextBox(application.mVg, 8, 8, application.mUiWidth - 16, str.c_str(), nullptr);


	nvgTextAlign(application.mVg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

	struct MenuOption final {
		std::string text;
		std::function<void()> action;
	};

	static int menuId = 0;
	std::vector<MenuOption> options;

	auto populateMenuOptions = [&](std::vector<MenuOption>& options, int id) {
		if (id == 0) {
			options.emplace_back("Play", []() {});
			options.emplace_back("Options", [&]() {
				menuId = 1;
				mSelectedOption = 0;
				});
			options.emplace_back("Quit", [&]() { kRunning = false; });
		}

		if (id == 1) {
			ma_device_info* pPlaybackInfos;
			ma_uint32 playbackCount;
			if (ma_context_get_devices(&application.mAudioEngine.mContext, &pPlaybackInfos, &playbackCount, nullptr, nullptr) != MA_SUCCESS) {
				// Error.
			}

			for (ma_uint32 iDevice = 0; iDevice < playbackCount; ++iDevice) {

				options.emplace_back(pPlaybackInfos[iDevice].name, [&]() {
					application.mAudioEngine.uninit();
					application.mAudioEngine.init(options[mSelectedOption].text);

					// Save choice

					YAML::Node config = YAML::Node();

					try {
						config = YAML::LoadFile("config.yaml");
					}
					catch (YAML::BadFile const& e) {}

					config["audioDevice"] = options[mSelectedOption].text;

					std::ofstream myfile;
					myfile.open("config.yaml", std::ios::binary | std::ios::out);
					myfile << config;
					myfile.close();
					});
			}


			options.emplace_back("Back", [&]() {
				mSelectedOption = 0;
				menuId = 0;
			});
		}
	};

	populateMenuOptions(options, menuId);

	{
		nvgFontSize(application.mVg, std::min(application.mUiHeight / options.size() / 2, 64.0f));

		if (mSelectedOption < 0) mSelectedOption = 0;
		if (mSelectedOption >= options.size()) mSelectedOption = options.size() - 1;

		for (int i = 0; i < options.size(); ++i) {
			if (i == mSelectedOption) nvgFillColor(application.mVg, nvgRGBf(1, 0, 0));
			else nvgFillColor(application.mVg, nvgRGBf(1, 1, 1));

			nvgText(application.mVg, application.mUiWidth / 2, application.mUiHeight / (options.size() + 1) * (i + 1), options[i].text.c_str(), nullptr);
		}

		if (mPerformAction) {
			options[mSelectedOption].action();
			ma_engine_play_sound(&application.mAudioEngine.mEngine, "select1.ogg", nullptr);
			mPerformAction = false;
		}
	}
}

void MenuState::destroy() {
	auto& application = getApplication();
	std::lock_guard lck{ application.mDispatcherMtx };
	application.mDispatcher.disconnect(this);
}

static Application* kApplication;

Application& getApplication() {
	return *kApplication;
}

int main(int argc, char* argv[]) {
	Application application;
	kApplication = &application;
	application.runMainThread();
}

#ifdef HB_ENTRY_WINMAIN
#include <Windows.h> // WINAPI, WinMain, _In_, _In_opt_, HINSTANCE
#include <stdlib.h> // __argc, __argv
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
	return main(__argc, __argv);
}
#endif