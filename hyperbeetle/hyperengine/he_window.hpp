#pragma once

#include <utility> // std::move, std::swap

extern "C" typedef struct GLFWwindow GLFWwindow;

namespace hyperengine {
	class Window final {
	public:
		struct CreateInfo final {
			int width, height;
			char const* title;
		};
	public:
		static void PollEvents();

		constexpr Window() noexcept = default;
		Window(CreateInfo const& info);
		Window(Window const&) = delete;
		Window& operator=(Window const&) = delete;
		inline Window(Window&& other) noexcept { *this = std::move(other); }
		Window& operator=(Window&& other) noexcept;
		~Window() noexcept;

		void SwapBuffers() const;

		inline GLFWwindow* Handle() const { return mHandle; }
		operator GLFWwindow*() const { return mHandle; }

		inline bool Valid() const { return mHandle; }
		explicit operator bool() const { return mHandle; }
	private:
		GLFWwindow* mHandle = nullptr;
	};
}