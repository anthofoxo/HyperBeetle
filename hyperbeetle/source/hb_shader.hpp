#pragma once

#include <unordered_map>
#include <string>

#include <glm/glm.hpp>

#include <glad/gl.h>

namespace hyperbeetle {

	class ShaderProgram final {
	public:
		struct UniformInfo final {
			GLint location;
			GLsizei count;
			GLenum type;
		};
	public:
		ShaderProgram() = default;
		ShaderProgram(unsigned int vert, unsigned int frag);

		void Uniform1i(std::string_view name, int v0) const;
		void Uniform1f(std::string_view name, float v0) const;
		void Uniform2f(std::string_view name, glm::vec2 const& v0) const;
		void Uniform3f(std::string_view name, glm::vec3 const& v0) const;

		void Uniform4f(std::string_view name, glm::vec4 const& v0) const;
		void UniformMat4f(std::string_view name, glm::mat4 const& v0) const;

		void Bind() const;
		int GetLocation(std::string_view name) const;
	private:
		struct MultiStringHash final
		{
			using hash_type = std::hash<std::string_view>;
			using is_transparent = void;

			std::size_t operator()(const char* str) const { return hash_type{}(str); }
			std::size_t operator()(std::string_view str) const { return hash_type{}(str); }
			std::size_t operator()(std::string const& str) const { return hash_type{}(str); }
		};

		unsigned int mHandle = 0;
		std::unordered_map<std::string, UniformInfo, MultiStringHash, std::equal_to<>> mUniforms;
	};
}