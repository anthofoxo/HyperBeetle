#include "hb_rdoc.hpp"

#ifdef _WIN32
#	include <Windows.h>
#	include <shlobj_core.h>
#	include <shellapi.h>
#endif

#include <renderdoc_app.h>

#include <format>
#include <string>
#include <filesystem>
#include <sstream>

#include <tracy/Tracy.hpp>

#include <imgui.h>

namespace hyperbeetle {
	namespace {
		RENDERDOC_API_1_1_2* sRdocApi = nullptr;

		void AttachDylib() {
			ZoneScoped;
#ifdef _WIN32
			HMODULE dylib = nullptr;
			if (!dylib) dylib = ::GetModuleHandleA("renderdoc.dll");
#if defined(HE_DEBUG) || defined(HE_RELEASE)
			if (!dylib) dylib = ::LoadLibraryA("renderdoc.dll");
			if (!dylib) {
				CHAR pf[MAX_PATH];
				::SHGetSpecialFolderPathA(nullptr, pf, CSIDL_PROGRAM_FILES, false);
				dylib = ::LoadLibraryA(std::format("{}/RenderDoc/renderdoc.dll", pf).c_str());
			}
#endif
			if (!dylib) return;
			
			pRENDERDOC_GetAPI RenderdocGetApi = (pRENDERDOC_GetAPI)::GetProcAddress(dylib, "RENDERDOC_GetAPI");
			RenderdocGetApi(eRENDERDOC_API_Version_1_1_2, (void**)&sRdocApi);
#else
			void* dylib = nullptr;
			if (!dylib) dylib = ::dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD);
#if defined(HE_DEBUG) || defined(HE_RELEASE)
			if (!dylib) dylib = ::dlopen("librenderdoc.so", RTLD_NOW);
#endif
			if (!dylib) return;

			pRENDERDOC_GetAPI RenderdocGetApi = (pRENDERDOC_GetAPI)::dlsym(dylib, "RENDERDOC_GetAPI");
			RenderdocGetApi(eRENDERDOC_API_Version_1_1_2, (void**)&sRdocApi);
#endif
		}
	}

	void AttachRenderdoc() {
		ZoneScoped;
		AttachDylib();
		if (!sRdocApi) return;

		// F2 to capture frame, PrintScreen / F12 are commonly used for screenshots
		RENDERDOC_InputButton key = eRENDERDOC_Key_F2;
		sRdocApi->SetCaptureKeys(&key, 1);

		// Disable overlay, we provide an ImGui window instead
		sRdocApi->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None);

		// Save captures relative to our working directory
		std::stringstream ss;
		ss << "./debug/rdoc_" << time(NULL);
		sRdocApi->SetCaptureFilePathTemplate(ss.str().c_str());
	}

	void DebugUiRenderdoc() {
		if (!sRdocApi) return;
		ZoneScoped;

		if (ImGui::Begin("RenderDoc")) {
			ImGui::TextUnformatted("RenderDoc is attached.");
			ImGui::TextUnformatted("F2 to capture.");
			const char* tem = sRdocApi->GetCaptureFilePathTemplate();
			ImGui::Text("%s %s%s", "Captures are saved to", tem, "_*.rdc");

			int major, minor, patch;
			sRdocApi->GetAPIVersion(&major, &minor, &patch);
			ImGui::LabelText("Version", "%d.%d.%d", major, minor, patch);

			if (!sRdocApi->IsTargetControlConnected())
				ImGui::TextColored({ 1, 1, 0, 1 }, "%s", "Note: RenderDoc UI isn't currently connected.");

			auto numCaptures = sRdocApi->GetNumCaptures();

			if (ImGui::Button("Capture"))
				sRdocApi->TriggerCapture();

			if (numCaptures > 0) {
				if (ImGui::CollapsingHeader("Captures")) {

					for (auto i = 0; i < numCaptures; ++i) {

						ImGui::PushID(i);

						uint64_t timestamp;
						uint32_t pathlength;
						sRdocApi->GetCapture(i, nullptr, &pathlength, &timestamp);
						std::string path;
						path.resize(pathlength);
						sRdocApi->GetCapture(i, path.data(), nullptr, nullptr);
#ifdef _WIN32

						if (ImGui::Button(path.data())) {
							std::string pathToOpen = std::filesystem::absolute(path.data()).generic_string();
							ShellExecuteA(0, 0, pathToOpen.data(), 0, 0, SW_SHOW);
						}
#else
						ImGui::TextUnformatted(path.data());
#endif
						ImGui::PopID();
					}

				}
			}
		}
		ImGui::End();
	}
}