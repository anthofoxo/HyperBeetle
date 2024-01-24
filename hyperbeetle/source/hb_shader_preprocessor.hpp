#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <string_view>

namespace hyperbeetle
{
	// TODO: Needs refactoring
	class ShaderPreprocessor final {
	public:
		struct Sources final {
			std::string vert;
			std::string frag;
		};
	public:
		inline void SetShaderPragma(std::string_view pragma) { mPragma = pragma; }
		inline void SetVertexDefine(std::string_view define) { mVertexDefine = define; }
		inline void SetFragmentDefine(std::string_view define) { mFragmentDefine = define; }
		inline void SetVertexAttribute(std::string_view attribute) { mVertexAttribute = attribute; }
		inline void SetFragmentAttribute(std::string_view attribute) { mFragmentAttribute = attribute; }
		inline void AddShaderInclude(const std::string& name, const std::string& source) { mIncludes[name] = source; }
		inline void RemoveShaderInclude(const std::string& name) { mIncludes.erase(name); }
		inline void AddDefine(const std::string& define) { mDefines.emplace(define); }
		inline void AddToken(const std::string& key, const std::string& value) { mTokens[key] = value; }
		Sources Process(std::string_view source);

	private:
		std::string mPragma = "#version 330 core";
		std::string mVertexDefine = "FE_VERT";
		std::string mFragmentDefine = "FE_FRAG";
		std::string mVertexAttribute = "vert";
		std::string mFragmentAttribute = "frag";
		std::unordered_map<std::string, std::string> mIncludes;
		std::unordered_map<std::string, std::string> mTokens;
		std::unordered_set<std::string> mDefines;
	};
}