// Win32
#ifdef _WIN32
#	include <Windows.h>
#endif

// OpenGL and GLFW
#include <glad/gl.h>
#include <GLFW/glfw3.h>

// Engine
#include "hb_state.hpp"
#include "hb_shader.hpp"
#include "hb_io.hpp"
#include "hb_log.hpp"
#include "hb_transform.hpp"
#include "hb_window.hpp"
#include "hb_shader_preprocessor.hpp"
#include "hb_rdoc.hpp"

// Glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

// Assimp
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

// Logger
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/base_sink.h>

// Profiler
#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>

// Gui
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <misc/cpp/imgui_stdlib.h>

#include <ImGuizmo.h>

#define NANOVG_GL3_IMPLEMENTATION
//#define NVG_NO_STB
#include <nanovg.h>
#include <nanovg_gl.h>

// Standard
#include <vector>
#include <fstream>
#include <iterator>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_set>
#include <filesystem>
#include <bit>
#include <sstream>
#include <mutex>
#include <memory>
#include <functional>

// Misc
#include <lua.hpp>
#include "stb_image.h"
#include "miniaudio.h"
#include "renderdoc_app.h"
#include "expected.hpp"

[[nodiscard]] void* operator new(std::size_t count) {
	void* ptr = malloc(count);
	if (ptr == nullptr) throw std::bad_alloc();
	TracyAlloc(ptr, count);
	return ptr;
}

void operator delete(void* ptr) noexcept {
	TracyFree(ptr);
	free(ptr);
}

float map(float value, float min1, float max1, float min2, float max2) {
	return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

class Shader final {
public:
	struct CreateInfo final {
		unsigned int type;
		std::string_view source;
	};
public:
	constexpr Shader() noexcept = default;
	Shader(CreateInfo const& info) {
		char const* data = info.source.data();
		int size = info.source.size();

		mHandle = glCreateShader(info.type);
		glShaderSource(mHandle, 1, &data, &size);
		glCompileShader(mHandle);

		int infoLogLength;
		glGetShaderiv(mHandle, GL_INFO_LOG_LENGTH, &infoLogLength);

		if (infoLogLength > 0) { 

			std::vector<GLchar> infoLog;
			infoLog.resize(infoLogLength);
			glGetShaderInfoLog(mHandle, infoLogLength, nullptr, infoLog.data());

			spdlog::error(infoLog.data());
		}
	}
	Shader(Shader const&) = delete;
	Shader& operator=(Shader const&) = delete;
	inline Shader(Shader&& other) noexcept { *this = std::move(other); }
	Shader& operator=(Shader&& other) noexcept {
		std::swap(mHandle, other.mHandle);
		return *this;
	}
	~Shader() noexcept {
		if (mHandle)
			glDeleteShader(mHandle);
	}

	inline unsigned int Handle() const { return mHandle; }
private:
	unsigned int mHandle = 0;
};

GLuint loadTextureCubemap(std::string_view resource, GLenum wrap) {
	auto data = hyperbeetle::LoadResourceBlob(resource).value();

	int x, y;
	unsigned char* pixels = stbi_load_from_memory(data.Data<unsigned char>(), data.Size(), &x, &y, nullptr, 4); // +x, -x, +y, -y, +z, -z
	if (!pixels) return 0;

	assert((y / x == 6) && "Height divided by the width should be 6, otherwise the size of the image doesnt map to a cubemap");

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

	size_t stride = x * x * 4;

	for(int i = 0; i < 6; ++i)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, x, x, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels + i * stride);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrap);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, wrap);

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	if (GLAD_GL_KHR_debug) {
		glObjectLabel(GL_TEXTURE, texture, resource.size(), resource.data());
	}

	stbi_image_free(pixels);

	return texture;
}

GLuint loadTextureImage(std::string_view resource, GLenum wrap) {
	auto data = hyperbeetle::LoadResourceBlob(resource).value();

	int x, y;
	unsigned char* pixels = stbi_load_from_memory(data.Data<unsigned char>(), data.Size(), &x, &y, nullptr, 4);
	if (!pixels) return 0;

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, wrap);

	if (GLAD_GL_KHR_debug) {
		glObjectLabel(GL_TEXTURE, texture, resource.size(), resource.data());
	}

	stbi_image_free(pixels);

	return texture;
}

// https://stackoverflow.com/a/5888676
size_t split(const std::string& txt, std::vector<std::string>& strs, char ch)
{
	size_t pos = txt.find(ch);
	size_t initialPos = 0;
	strs.clear();

	// Decompose statement
	while (pos != std::string::npos) {
		strs.push_back(txt.substr(initialPos, pos - initialPos));
		initialPos = pos + 1;

		pos = txt.find(ch, initialPos);
	}

	// Add the last one
	strs.push_back(txt.substr(initialPos, std::min(pos, txt.size()) - initialPos + 1));

	return strs.size();
}

void LoadImGui(GLFWwindow* window) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	// io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330 core");
}

struct VertexNew {
	glm::vec3 position;
	glm::vec2 texCoord;
	glm::vec3 normal;
};

struct Mesh final {
	unsigned int vao = 0, vbo = 0, ebo = 0;
	unsigned int count = 0;

	void SubmitDrawCall() {
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
	}
};

void DebugUiLog() {
	if (ImGui::Begin("Log")) {
		for (auto& line : hyperbeetle::GetLogs())
			ImGui::TextUnformatted(line.c_str());

		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);
	}
	ImGui::End();
}

rd::expected<Mesh, std::string> LoadMeshI(std::filesystem::path const& resource) {
	auto expectedData = hyperbeetle::LoadResourceBlob(resource);

	if (!expectedData.has_value())
		return rd::unexpected(expectedData.error());
	
	const auto& data = expectedData.value();

	auto postProcessingSteps = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_GenUVCoords;
	std::string extension = resource.extension().generic_string();

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFileFromMemory(data.Data(), data.Size(), postProcessingSteps, extension.c_str());

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		return rd::unexpected("Invalid model");

	if(scene->mNumMeshes != 1)
		return rd::unexpected("Unsupported model, Consider joining meshes");

	aiMesh* meshData = scene->mMeshes[0];

	std::vector<VertexNew> vertices;
	std::vector<unsigned int> indices;

	vertices.reserve(meshData->mNumVertices);

	for (unsigned int i = 0; i < meshData->mNumVertices; ++i) {
		vertices.push_back(VertexNew
			{
				.position = std::bit_cast<glm::vec3>(meshData->mVertices[i]),
				.texCoord = glm::vec2(std::bit_cast<glm::vec3>(meshData->mTextureCoords[0][i])),
				.normal = std::bit_cast<glm::vec3>(meshData->mNormals[i])
			});
	}

	indices.reserve((size_t)meshData->mNumFaces * 3);

	for (unsigned int i = 0; i < meshData->mNumFaces; ++i)
	{
		aiFace& face = meshData->mFaces[i];

		// only push triangles
		if (face.mNumIndices != 3) continue;

		indices.push_back(face.mIndices[0]);
		indices.push_back(face.mIndices[1]);
		indices.push_back(face.mIndices[2]);
	}

	Mesh mesh;

	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(decltype(vertices)::value_type), vertices.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &mesh.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(decltype(indices)::value_type), indices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexNew), (void*)(uintptr_t)offsetof(VertexNew, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexNew), (void*)(uintptr_t)offsetof(VertexNew, texCoord));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VertexNew), (void*)(uintptr_t)offsetof(VertexNew, normal));

	mesh.count = (unsigned int)indices.size();

	return mesh;
}

namespace {
	void OpenGLMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param)
	{
		// API PERFORMANCE MEDIUM 131219
		// Program/Shader state performance warning
		// x is being recompiled based on GL state
		if (id == 131218) return;

		spdlog::error(message);

#ifdef _WIN32
		if (severity == GL_DEBUG_SEVERITY_HIGH) __debugbreak();
#endif
	}
}

void BeginGuiFrame() {
	ZoneScoped;
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
}

void EndGuiFrame() {
	ZoneScoped;
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void SetupKhrDebug() {
	if (!GLAD_GL_KHR_debug) return;
	ZoneScoped;
	spdlog::debug("KHR_debug is enabled");
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(&OpenGLMessageCallback, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
}

struct Pack {
	std::string id;
	std::string nspace;
};

struct Level {
	Pack* origin;

	std::string id;
	std::string level_name;
	int difficulty;
	std::string description;
	std::string author;
	float bpm;
};

struct CustomGameContent {
	std::unordered_map<std::string, Pack> packs;
	std::unordered_map<std::string, Level> levels;
};

static CustomGameContent kGameContent;

CustomGameContent LoadPacks() {
	
	CustomGameContent content;

	spdlog::info("Searching for packs...");
	for (auto& entry : std::filesystem::directory_iterator("pack")) {
		std::string pack_id = entry.path().filename().generic_string();
		spdlog::info("\tid: {}", pack_id);
		Pack pack;

		auto fileref = entry.path() / "pack.lua";

		lua_State* L = luaL_newstate();
		luaL_dofile(L, fileref.generic_string().c_str());
		lua_getglobal(L, "namespace");
		pack.nspace = lua_tostring(L, -1);
		lua_pop(L, 1);
		lua_close(L);

		spdlog::info("\tnamespace: {}", pack.nspace);

		pack.id = pack_id;

		content.packs.emplace(std::make_pair(pack_id, pack));
	}

	spdlog::info("Searching for levels...");
	for (auto& [k, v] : content.packs) {
		std::string path = std::format("pack/{}/level", k);

		for (auto& entry : std::filesystem::directory_iterator(path)) {
			Level level;
			level.origin = &v;
			level.id = v.nspace + '.' + entry.path().stem().generic_string();

			auto fileref = entry.path().generic_string();
			lua_State* L = luaL_newstate();
			luaL_dofile(L, fileref.c_str());
			lua_getglobal(L, "level_name");
			level.level_name = lua_tostring(L, -1);
			lua_getglobal(L, "difficulty");
			level.difficulty = lua_tointeger(L, -1);
			lua_getglobal(L, "description");
			level.description = lua_tostring(L, -1);
			lua_getglobal(L, "author");
			level.author = lua_tostring(L, -1);
			lua_getglobal(L, "bpm");
			level.bpm = lua_tonumber(L, -1);
			lua_pop(L, 5);
			lua_close(L);

			spdlog::info("----------------");
			spdlog::info("\tid: {}", level.id);
			spdlog::info("\torigin: {}", k);
			spdlog::info("\tlevel_name: {}", level.level_name);
			spdlog::info("\tdifficulty: {}", level.difficulty);
			spdlog::info("\tdescription: {}", level.description);
			spdlog::info("\tauthor: {}", level.author);
			spdlog::info("\tbpm: {}", level.bpm);
			content.levels.emplace(std::make_pair(level.id, level));
		}
	}

	return content;
}

class Application final {
public:
	void Start() {
		mStateManager.SetUserPtr(this);

		Init();

		double currentTime, lastTime = glfwGetTime();
		
		while (mRunning) {
			currentTime = glfwGetTime();
			mDeltaTime = currentTime - lastTime;
			lastTime = currentTime;

			mLastKeys = mKeys;
			hyperbeetle::Window::PollEvents();

			BeginGuiFrame();

			while (!deferredEvents.empty()) {
				auto fn = deferredEvents.front();
				deferredEvents.pop_front();
				fn();
			}
			
			Update();

			EndGuiFrame();

			mWindow.SwapBuffers();

			TracyGpuCollect;
			FrameMark;
		}

		Destroy();
	}
	
	void Init() {
		mWindow = hyperbeetle::Window({ .width = 1280, .height = 720, .title = "HyperBeetle by AnthoFoxo" });

		TracyGpuContext;

		SetupKhrDebug();

		glfwSetWindowUserPointer(mWindow, &mKeys);
		glfwSetKeyCallback(mWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			auto* keys = reinterpret_cast<std::unordered_set<int>*>(glfwGetWindowUserPointer(window));
			if (action == GLFW_REPEAT) return;
			if (action == GLFW_PRESS) keys->insert(key);
			if (action == GLFW_RELEASE) keys->erase(key);
		});

		LoadImGui(mWindow);

		ma_engine_init(nullptr, &engine);

		vg = nvgCreateGL3(NVG_ANTIALIAS);

		// Start lua script
		L = luaL_newstate();
		luaL_openlibs(L);
		luaL_dofile(L, "scripts/config.lua");

		lua_getglobal(L, "Font");
		std::string fontsource = lua_tostring(L, -1);
		lua_pop(L, 1);

		fontdata = hyperbeetle::LoadResourceBlob(fontsource).value();
		int font_id = nvgCreateFontMem(vg, "default", fontdata.Data<unsigned char>(), fontdata.Size(), false);
		if (font_id == -1) spdlog::error("Failed to load font");
		nvgFontFaceId(vg, font_id);

		glDepthFunc(GL_LEQUAL);	
	}

	bool showDebugUi = false;

	void Update() {	
		if (glfwWindowShouldClose(mWindow)) mRunning = false;

		if (mKeys.contains(GLFW_KEY_F5) && !mLastKeys.contains(GLFW_KEY_F5))
			showDebugUi = !showDebugUi;

		if (mStateManager.Get()->UseDebugGui()) {
			ImGui::DockSpaceOverViewport();
			showDebugUi = true;
		}

		if (showDebugUi) {
			if (ImGui::BeginMainMenuBar()) {
				if (ImGui::BeginMenu("Debug")) {
					if (ImGui::MenuItem("Close debug UI", "F5", nullptr, !mStateManager.Get()->UseDebugGui())) {
						showDebugUi = false;
					}

					ImGui::EndMenu();
				}

				ImGui::EndMainMenuBar();
			}

			hyperbeetle::DebugUiRenderdoc();
			DebugUiLog();
			ImGui::ShowDemoWindow();
		}

		mStateManager.Update();	
	}

	void Destroy() {
		nvgDeleteGL3(vg);

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		ma_engine_uninit(&engine);

		lua_close(L);
	}

	lua_State* L;
	std::deque<std::function<void()>> deferredEvents;
	bool mRunning = true;
	double mDeltaTime = 1.0;
	std::unordered_set<int> mKeys;
	std::unordered_set<int> mLastKeys;
	hyperbeetle::Window mWindow;
	NVGcontext* vg;
	hyperbeetle::Blob fontdata;
	ma_engine engine;
	hyperbeetle::StateManager mStateManager;
};


struct RenderTarget {
	int width = -1, height = -1;
	GLuint framebuffer = 0;
	GLuint color = 0;
	GLuint depth = 0;

	void Bind() {
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glViewport(0, 0, width, height);
	}

	void Delete() {

		if (framebuffer != 0) {
			glDeleteFramebuffers(1, &framebuffer);
			glDeleteTextures(1, &color);
			glDeleteRenderbuffers(1, &depth);
		}
	}
};

RenderTarget CreateRenderTarget(int width, int height) {
	RenderTarget target;

	target.width = width;
	target.height = height;

	glGenFramebuffers(1, &target.framebuffer);
	glGenTextures(1, &target.color);
	glGenRenderbuffers(1, &target.depth);

	glBindRenderbuffer(GL_RENDERBUFFER, target.depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

	glBindTexture(GL_TEXTURE_2D, target.color);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.color, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, target.depth);

	return target;
}

class EditorState final : public hyperbeetle::State {
public:
	bool mWireframe = false;

	ImGuizmo::OPERATION currentOp = ImGuizmo::OPERATION::TRANSLATE;

	Mesh trackMesh;

	hyperbeetle::ShaderProgram program;
	hyperbeetle::ShaderProgram skyboxProgram;
	hyperbeetle::ShaderProgram cubicProgram;
	hyperbeetle::ShaderProgram glitchProgram;

	GLuint skybox_vao, skybox_vbo, skybox_ebo;

	hyperbeetle::Transform targetTrackView;
	hyperbeetle::Transform currentTrackView;

	std::vector<hyperbeetle::Transform> objtransforms;
	std::vector<hyperbeetle::Transform> transforms;

	hyperbeetle::Transform viewTransform;
	GLuint skyboxCubemap;

	// 240 bpm // thumper 480
	int valOffset = 0;
	double bpmTimer = 0;

	bool ridingTrack = false;

	RenderTarget target0;
	RenderTarget target1;

	void CreateRenderbuffer(int width, int height) {
		if (target0.width != width || target0.height != height) {
			target0.Delete();
			target0 = CreateRenderTarget(width, height);
		}

		if (target1.width != width || target1.height != height) {
			target1.Delete();
			target1 = CreateRenderTarget(width, height);
		}
	}

	void Init() {
		ZoneScoped;

		// Load and generate transforms for the track
		{
			std::string track_turn_string = hyperbeetle::LoadResourceString("stage/track_turn.txt").value();

			std::vector<std::string> initSplit;
			split(track_turn_string, initSplit, ';');
			objtransforms.resize(std::stoi(initSplit[0]));

			for (auto& val : objtransforms) {
				val.Rotate(glm::radians(1.25f), glm::vec3(1, 0, 0));
				val.Translate(glm::vec3(0, 0, -20));
			}

			track_turn_string = initSplit[1];
			split(track_turn_string, initSplit, ',');

			std::vector<std::string> map;

			for (auto& val : initSplit) {
				map.clear();
				split(val, map, ':');
				objtransforms[std::stoi(map[0])].Reset();
				objtransforms[std::stoi(map[0])].Rotate(glm::radians(std::stof(map[1])), glm::vec3(0, 1, 0));
				objtransforms[std::stoi(map[0])].Rotate(glm::radians(3.0f), glm::vec3(1, 0, 0));
				objtransforms[std::stoi(map[0])].Translate(glm::vec3(0, 0, -10));
			}

			// Apply transformations along track length

			hyperbeetle::Transform current;
			transforms.emplace_back(current);

			for (int i = 0; i < objtransforms.size(); ++i) {

				auto intp = current;

				if (i > 0) {
					auto transNext = hyperbeetle::Transform();
					transNext.Set(current.Get() * objtransforms[i - 1].Get());

					intp = hyperbeetle::Transform::InterpolateLinear(current, transNext, 0.5f);
				}

				current.Set(current.Get() * objtransforms[i].Get()); // Transform by track

				transforms.emplace_back(intp);

				transforms.emplace_back(current); // Deuplate is the control point, ignored
			}
		}

		trackMesh = LoadMeshI("meshes/track_test.obj").value();

		{
			GLfloat vertices[] = {
				// Front face
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,

				// Back face
				-1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
			};

			GLuint indices[] = {
				0, 2, 1,  // Front face
				2, 0, 3,
				4, 5, 6,  // Back face
				6, 7, 4,
				0, 4, 7,  // Left face
				7, 3, 0,
				1, 6, 5,  // Right face
				6, 1, 2,
				3, 6, 2,  // Top face
				6, 3, 7,
				0, 1, 5,  // Bottom face
				5, 4, 0,
			};

			// Create Vertex Array Object
			glGenVertexArrays(1, &skybox_vao);
			glBindVertexArray(skybox_vao);

			// Create Vertex Buffer Object
			glGenBuffers(1, &skybox_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, skybox_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			// Create Element Buffer Object
			glGenBuffers(1, &skybox_ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox_ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

			// Set up vertex attributes
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
			glEnableVertexAttribArray(0);
		}

		{
			hyperbeetle::ShaderPreprocessor shader_engine;
			shader_engine.AddShaderInclude("common", hyperbeetle::LoadResourceString("shaders/common.glsl").value());

			auto LoadViaShaderEngine = [](hyperbeetle::ShaderPreprocessor& engine, std::string_view resource) -> hyperbeetle::ShaderProgram {

				auto sources = engine.Process(hyperbeetle::LoadResourceString(resource).value());

				Shader vertShader({ .type = GL_VERTEX_SHADER, .source = sources.vert });
				Shader fragShader({ .type = GL_FRAGMENT_SHADER, .source = sources.frag });

				return hyperbeetle::ShaderProgram(vertShader.Handle(), fragShader.Handle());
			};

			program = LoadViaShaderEngine(shader_engine, "shaders/track.glsl");
			skyboxProgram = LoadViaShaderEngine(shader_engine, "shaders/skybox.glsl");
			cubicProgram = LoadViaShaderEngine(shader_engine, "shaders/tonemap_cubic_distort.glsl");
			glitchProgram = LoadViaShaderEngine(shader_engine, "shaders/glitch.glsl");
		}


		lua_State* L = Get().GetUserPtr<Application>().L;
		lua_getglobal(L, "Cubemap");
		std::string cubemapresource = lua_tostring(L, -1);
		lua_pop(L, 1);

		skyboxCubemap = loadTextureCubemap(cubemapresource, GL_CLAMP_TO_EDGE);
	}

	bool UseDebugGui() override { return true; };

	void Update() override {
		ZoneScoped;

		auto& keys = Get().GetUserPtr<Application>().mKeys;

		if (ImGui::Begin("Colormap Vals")) {
			ImGui::DragFloat2("Offset", glm::value_ptr(offset), 0.01f);
			ImGui::DragFloat("Distortion", &distort, 0.1f);
			ImGui::DragFloat("CubicDistortion", &cubicDistortion, 0.1f);
			ImGui::DragInt("Inset", &inset);

			ImGui::DragFloat2("uBlack_InvRange_InvGamma", glm::value_ptr(uBlack_InvRange_InvGamma), 0.01f);
			ImGui::DragFloat3("uInvGammas", glm::value_ptr(uInvGammas), 0.01f);
			ImGui::DragFloat3("uOutputRanges", glm::value_ptr(uOutputRanges), 0.01f);
			ImGui::DragFloat3("uOutputBlacks", glm::value_ptr(uOutputBlacks), 0.01f);

		}
		ImGui::End();

		if (keys.contains(GLFW_KEY_LEFT_CONTROL)) currentOp = ImGuizmo::OPERATION::ROTATE_Z;
		else currentOp = ImGuizmo::OPERATION::TRANSLATE;

		bool inputEnabled = glfwGetMouseButton(Get().GetUserPtr<Application>().mWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

		if (inputEnabled) {
			glm::vec3 movement{};
			if (keys.contains(GLFW_KEY_A)) --movement.x;
			if (keys.contains(GLFW_KEY_D)) ++movement.x;
			if (keys.contains(GLFW_KEY_LEFT_SHIFT)) --movement.y;
			if (keys.contains(GLFW_KEY_SPACE)) ++movement.y;
			if (keys.contains(GLFW_KEY_W)) --movement.z;
			if (keys.contains(GLFW_KEY_S)) ++movement.z;

			float rotz = 0.0f;
			if (keys.contains(GLFW_KEY_Q)) --rotz;
			if (keys.contains(GLFW_KEY_E)) ++rotz;

			viewTransform.Rotate(glm::radians(rotz), glm::vec3(0, 0, 1));

			if (glm::length2(movement) > 0.0f) {
				movement = glm::normalize(movement) * 50.0f;
				viewTransform.Set(glm::translate(viewTransform.Get(), movement * static_cast<float>(Get().GetUserPtr<Application>().mDeltaTime)));
			}

			glm::vec2 mouseDelta = { ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y };

			// Mouse movement
			if (glm::length2(mouseDelta) > 0.0f) {
				glm::vec2 delta = mouseDelta * -0.3f;

				glm::vec4 up = viewTransform.GetInverse() * glm::vec4(0, 1, 0, 0);

				//ZviewTransform.Rotate(glm::radians(delta.x), glm::vec3(up));
				viewTransform.Rotate(glm::radians(delta.x), glm::vec3(0, 1, 0));
				viewTransform.Rotate(glm::radians(delta.y), glm::vec3(1, 0, 0));
			}
		}

		if (ImGui::Begin("Debug")) {
			ImGui::Checkbox("Wireframe", &mWireframe);

			if (ImGui::Button("Ride Track")) {
				ridingTrack = true;
				bpmTimer = 0;
				valOffset = 0;
				currentTrackView.Reset();
				targetTrackView.Reset();
			}
		}
		ImGui::End();

		glm::mat4 viewMatrix = viewTransform.GetInverse();

		// Manually set view matrix when riding track
		if (ridingTrack) {
			if (Get().GetUserPtr<Application>().mKeys.contains(GLFW_KEY_ESCAPE)) ridingTrack = false;

			//bpmTimer += ImGui::GetIO().DeltaTime * (480.0f / 60.0f);
			bpmTimer += ImGui::GetIO().DeltaTime * (85.0f / 60.0f);

			// ValOffset into control point
			int controlPointStart = valOffset * 2;

			if (bpmTimer >= 1) {
				bpmTimer = 0;

				if (valOffset % 8 == 0) {
					//ma_sound* snd = new ma_sound;
					//ma_sound_init_from_file(&engine, "thump2.wav", 0, nullptr, nullptr, snd);
					//ma_sound_start(snd);
				}

				++valOffset;
			}

			if (controlPointStart + 2 >= transforms.size()) {
				ridingTrack = false;
			}
			else
			{
				targetTrackView = hyperbeetle::Transform::InterpolateQuadratic(transforms[controlPointStart], transforms[controlPointStart + 1], transforms[controlPointStart + 2], bpmTimer);
				targetTrackView.Translate(glm::vec3(0, 0.5f, 5));

				// 3 unity above track relative
				//mixed.Translate(glm::vec3(0, 3, 5));

				// mix between trackviews

				currentTrackView = hyperbeetle::Transform::InterpolateLinear(currentTrackView, targetTrackView, 0.5f);

				viewMatrix = currentTrackView.GetInverse();
			}
		}

		//glm::mat4 projection = glm::perspectiveFov<float>(glm::radians(90.0f), w, h, 0.1f, 1024.0f);
		glm::mat4 projection = glm::infinitePerspective(glm::radians(90.0f), (float)target0.width / (float)target0.height, 0.1f);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });

		if (ImGui::Begin("Viewport")) {

			ImVec2 contentRegion = ImGui::GetContentRegionAvail();
			CreateRenderbuffer(contentRegion.x, contentRegion.y);

			ImVec2 min = ImGui::GetWindowContentRegionMin();
			ImVec2 max = ImGui::GetWindowContentRegionMax();

			ImGui::Image((void*)target1.color, ImVec2(target0.width, target0.height), { 0, 1 }, { 1, 0 });

			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(min.x + ImGui::GetWindowPos().x, min.y + ImGui::GetWindowPos().y, max.x - min.x, max.y - min.y);

			if (Get().GetUserPtr<Application>().mKeys.contains(GLFW_KEY_Z)) {
				for (int i = 0; i < transforms.size(); ++i) {
					glm::mat4 mat = transforms[i].Get();
					ImGuizmo::SetID(i);
					if (ImGuizmo::Manipulate(glm::value_ptr(viewMatrix), glm::value_ptr(projection), currentOp, ImGuizmo::MODE::LOCAL, glm::value_ptr(mat))) {
						transforms[i].Set(mat);
					}
				}
			}
		}
		ImGui::End();

		ImGui::PopStyleVar();

		// Draw viewport
		{
			target1.Bind();

			glClearColor(0.0f, 0.0f, 0.3f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glEnable(GL_SCISSOR_TEST);
			glScissor(inset, inset, target1.width - inset * 2, target1.height - inset * 2);

			// Draw skybox
			glActiveTexture(GL_TEXTURE0 + 0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemap);
			skyboxProgram.Bind();

			skyboxProgram.UniformMat4f("uView", viewMatrix);
			skyboxProgram.UniformMat4f("uProjection", projection);
			skyboxProgram.Uniform1f("uTime", fmodf(glfwGetTime(), 1.0f));
			skyboxProgram.Uniform1i("uReflectionSampler", 0);
			skyboxProgram.Uniform1i("uNoiseSampler", 1);

			glDisable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glBindVertexArray(skybox_vao);
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

			glDisable(GL_CULL_FACE);

			program.Bind();
			program.UniformMat4f("uView", viewMatrix);
			program.UniformMat4f("uProjection", projection);

			glActiveTexture(GL_TEXTURE0 + 0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemap);

			if (mWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			glBindVertexArray(trackMesh.vao);

			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			{
				ZoneScopedN("Draw track");
				for (int i = 0; i < transforms.size() - 2; i += 2) {

					program.UniformMat4f("ws0", transforms[i + 0].Get());
					program.UniformMat4f("ws1", transforms[i + 1].Get());
					program.UniformMat4f("ws2", transforms[i + 2].Get());
					trackMesh.SubmitDrawCall();
				}
			}

			glDisable(GL_CULL_FACE);

			if (mWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			glDisable(GL_SCISSOR_TEST);

			// Post effects
			///// Apply glitch shader
			glDisable(GL_DEPTH_TEST);

			unsigned int vao;
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			target0.Bind();
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glitchProgram.Bind();
			glActiveTexture(GL_TEXTURE0 + 0);
			glBindTexture(GL_TEXTURE_2D, target1.color);

			glitchProgram.Uniform1f("shake_power", 0.03f);
			glitchProgram.Uniform1f("shake_rate", 0.2f);
			glitchProgram.Uniform1f("shake_speed", 5.0f);
			glitchProgram.Uniform1f("shake_block_size", 30.5f);
			glitchProgram.Uniform1f("shake_color_rate", 0.01f);
			glitchProgram.Uniform1f("uTime", glfwGetTime());
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			// Do cubic and distort

			target1.Bind();
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/////
			cubicProgram.Bind();
			glActiveTexture(GL_TEXTURE0 + 0);
			glBindTexture(GL_TEXTURE_2D, target0.color);

			cubicProgram.Uniform1f("uDistortion", distort);
			cubicProgram.Uniform1f("uCubicDistortion", cubicDistortion);
			cubicProgram.Uniform2f("uOffset", offset);
			cubicProgram.Uniform2f("uBlack_InvRange_InvGamma", uBlack_InvRange_InvGamma);
			cubicProgram.Uniform3f("uInvGammas", uInvGammas);
			cubicProgram.Uniform3f("uOutputRanges", uOutputRanges);
			cubicProgram.Uniform3f("uOutputBlacks", uOutputBlacks);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			glEnable(GL_DEPTH_TEST);

			glBindVertexArray(0);
			glDeleteVertexArrays(1, &vao);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			int w, h;
			glfwGetFramebufferSize(Get().GetUserPtr<Application>().mWindow, &w, &h);
			glViewport(0, 0, w, h);
		}
	}

	glm::vec2 offset = glm::vec2(0.0f, 0.0f);
	float distort = -1.3f;
	float cubicDistortion = 0.5;
	int inset = 1;
	glm::vec2 uBlack_InvRange_InvGamma = glm::vec2(0.07843, 1.08511);
	glm::vec3 uInvGammas = glm::vec3(1.00, 1.00, 1.42857);
	glm::vec3 uOutputRanges = glm::vec3(0.81922, 0.96353, 0.92941);
	glm::vec3 uOutputBlacks = glm::vec3(0.18078, 0.03647, 0.07059);
};

class MenuState : public hyperbeetle::State {
public:
	void Init() override {
		lua_State* L = Get().GetUserPtr<Application>().L;
		lua_getglobal(L, "Lang");
		lua_getfield(L, -1, "en_us");
		lua_getfield(L, -1, "title");
		title = lua_tostring(L, -1);
		lua_pop(L, 3);
	}

	~MenuState() {
	}

	std::string title;

	struct MenuOption {
		std::string name;
		bool has_func;
		std::function<void()> invoke = nullptr;
	};

	std::array<MenuOption, 4> options = {
		MenuOption("Play", false, nullptr),
		MenuOption("Options", false, nullptr),
		MenuOption("Editor", true, std::bind(&MenuState::OpenEditor, this)),
		MenuOption("Quit", true, std::bind(&MenuState::Quit, this))
	};

	void OpenEditor() {
		Get().GetUserPtr<Application>().deferredEvents.push_back([&]() {
			Get().Set<EditorState>();
			});
	}

	void Quit() {
		Get().GetUserPtr<Application>().mRunning = false;
	}

	void Update() override {

		int w, h;
		glfwGetFramebufferSize(Get().GetUserPtr<Application>().mWindow, &w, &h);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, w, h);
		glClearColor(0.2f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		NVGcontext* vg = Get().GetUserPtr<Application>().vg;
		nvgBeginFrame(vg, w, h, 1);

		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgFillColor(vg, { 1, 0.0f, 0.0f, 1 });
		nvgFontSize(vg, 96);
		nvgText(vg, w / 2, h / 7 * 1, title.data(), title.data() + title.size());

		float scale = map(glm::sin((float)glfwGetTime() * 5.f), -1, 1, 0, 1);

		static int selected = 0;

		auto& keys = Get().GetUserPtr<Application>().mKeys;
		auto& lastKeys = Get().GetUserPtr<Application>().mLastKeys;

		if (keys.contains(GLFW_KEY_SPACE) && !lastKeys.contains(GLFW_KEY_SPACE)) {
			if (options[selected].has_func) {
				options[selected].invoke();
				ma_engine_play_sound(&Get().GetUserPtr<Application>().engine, "audio/select1.ogg", nullptr);
			}
			else {
				ma_engine_play_sound(&Get().GetUserPtr<Application>().engine, "audio/locked.ogg", nullptr);
			}
		}

		if (keys.contains(GLFW_KEY_W) && !lastKeys.contains(GLFW_KEY_W)) {

			int prev = selected;

			--selected;
			if (selected < 0) selected = 0;
			if (selected >= options.size()) selected = options.size() - 1;

			// cursor didnt move, error sound
			if (prev == selected) {
				ma_engine_play_sound(&Get().GetUserPtr<Application>().engine, "audio/locked.ogg", nullptr);
			}
			else {
				ma_engine_play_sound(&Get().GetUserPtr<Application>().engine, "audio/select2.ogg", nullptr);
			}
		}

		if (keys.contains(GLFW_KEY_S) && !lastKeys.contains(GLFW_KEY_S)) {

			int prev = selected;

			++selected;
			if (selected < 0) selected = 0;
			if (selected >= options.size()) selected = options.size() - 1;

			// cursor didnt move, error sound
			if (prev == selected) {
				ma_engine_play_sound(&Get().GetUserPtr<Application>().engine, "audio/locked.ogg", nullptr);
			}
			else {
				ma_engine_play_sound(&Get().GetUserPtr<Application>().engine, "audio/select2.ogg", nullptr);
			}
		}

		for (int i = 0; i < options.size(); ++i) {
			MenuOption& option = options[i];

			if(i == selected)
				nvgFontSize(vg, map(scale, 0, 1, 48, 64));
			else
				nvgFontSize(vg, 56);

			nvgFillColor(vg, { 1, 0.0f, 0.0f, 1 });
			nvgText(vg, w / 2, h / 7 * (3 + i), option.name.data(), option.name.data() + option.name.size());
		}

		nvgEndFrame(vg);
	}
};

class LogoState : public hyperbeetle::State {
public:
	void Init() override {
		ma_engine_play_sound(&Get().GetUserPtr<Application>().engine, "audio/intro1.ogg", nullptr);

		hyperbeetle::ShaderPreprocessor shader_engine;
		shader_engine.AddShaderInclude("common", hyperbeetle::LoadResourceString("shaders/common.glsl").value());

		auto LoadViaShaderEngine = [](hyperbeetle::ShaderPreprocessor& engine, std::string_view resource) -> hyperbeetle::ShaderProgram {

			auto sources = engine.Process(hyperbeetle::LoadResourceString(resource).value());

			Shader vertShader({ .type = GL_VERTEX_SHADER, .source = sources.vert });
			Shader fragShader({ .type = GL_FRAGMENT_SHADER, .source = sources.frag });

			return hyperbeetle::ShaderProgram(vertShader.Handle(), fragShader.Handle());
		};

		glitchProgram = LoadViaShaderEngine(shader_engine, "shaders/glitch.glsl");

		glGenVertexArrays(1, &vao);
		
		ma_sound_init_from_file(&Get().GetUserPtr<Application>().engine, "audio/glitch.mp3", 0, nullptr, nullptr, &sound);
		ma_sound_set_volume(&sound, 0.0f);
		ma_sound_set_looping(&sound, true);
		ma_sound_start(&sound);

		lua_State* L = Get().GetUserPtr<Application>().L;

		lua_getglobal(L, "Lang");
		lua_getfield(L, -1, "en_us");
		lua_getfield(L, -1, "title");
		title = lua_tostring(L, -1);
		lua_pop(L, 3);
	}

	~LogoState() {
		ma_sound_uninit(&sound);
	}

	void Update() override {
		timer += Get().GetUserPtr<Application>().mDeltaTime * 0.2f;

		if (timer >= 1) {
			Get().GetUserPtr<Application>().deferredEvents.push_back([&]() {
				Get().Set<MenuState>();
			});
		}

		int w, h;
		glfwGetFramebufferSize(Get().GetUserPtr<Application>().mWindow, &w, &h);

		if (target.width != w || target.height != h) {
			target.Delete();
			target = CreateRenderTarget(w, h);
		}

		target.Bind();
		glClearColor(0.0f, 0.1f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		NVGcontext* vg = Get().GetUserPtr<Application>().vg;
		nvgBeginFrame(vg, w, h, 1);

		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgFillColor(vg, { 1, 0.3f, 0.1f, 1 });
		nvgFontSize(vg, std::lerp<float>((float)h / 10.0f, (float)h / 5.0f, timer));
		nvgText(vg, w / 2, h / 2, title.data(), title.data() + title.size());

		nvgEndFrame(vg);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, w, h);
		glClearColor(0.0f, 0.1f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDisable(GL_DEPTH_TEST);

		glBindVertexArray(vao);
		glitchProgram.Bind();
		glitchProgram.Uniform1f("shake_power", 0.03f);
		glitchProgram.Uniform1f("shake_rate", 0.2f);
		glitchProgram.Uniform1f("shake_speed", 5.0f);
		glitchProgram.Uniform1f("shake_block_size", 30.5f);
		glitchProgram.Uniform1f("shake_color_rate", 0.01f);
		glitchProgram.Uniform1f("uTime", glfwGetTime());

		float truncated = trunc((float)glfwGetTime() * 5.0f);

		float enabled = random(truncated) < 0.2f;

		glitchProgram.Uniform1f("enable_shift", enabled);

		ma_sound_set_volume(&sound, enabled * 0.1f);

		float enable_shift = float();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, target.color);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	float random(float seed) {
		return glm::fract(543.2543 * sin(glm::dot(glm::vec2(seed, seed), glm::vec2(3525.46, -54.3415))));
	}

	ma_sound sound;
	std::string title;
	double timer = 0;
	hyperbeetle::ShaderProgram glitchProgram;
	RenderTarget target;
	unsigned int vao;
};


void AppMain() {
	TracySetProgramName("HyperBeetle");
	hyperbeetle::SetupLogger();
	spdlog::info("Starting engine");
	hyperbeetle::AttachRenderdoc();

	kGameContent = LoadPacks();

	{
		Application app;
		app.deferredEvents.push_back([&app]() { app.mStateManager.Set<LogoState>(); });
		app.Start();
	}

	spdlog::shutdown();
}

int main(int, char**) {
	AppMain();
	return 0;
}