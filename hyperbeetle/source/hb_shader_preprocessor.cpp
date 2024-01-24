#include "hb_shader_preprocessor.hpp"

#include <boost/regex.hpp>
#include <sstream>

namespace hyperbeetle
{
	// Allows a regex replace where the replacement can change depending on the match
	// See ProcessTokens/ProcessIncludes
	template<class BidirIt, class Traits, class CharT, class UnaryFunction>
	static std::basic_string<CharT> RegexReplace(BidirIt first, BidirIt last,
		const boost::basic_regex<CharT, Traits>& re, UnaryFunction f)
	{
		std::basic_string<CharT> s;

		typename boost::match_results<BidirIt>::difference_type positionOfLastMatch = 0;
		auto endOfLastMatch = first;

		auto callback = [&](const boost::match_results<BidirIt>& match)
			{
				auto positionOfThisMatch = match.position(static_cast<boost::match_results<BidirIt>::size_type>(0));
				auto diff = positionOfThisMatch - positionOfLastMatch;

				auto startOfThisMatch = endOfLastMatch;
				std::advance(startOfThisMatch, diff);

				s.append(endOfLastMatch, startOfThisMatch);
				s.append(f(match));

				auto lengthOfMatch = match.length(0);

				positionOfLastMatch = positionOfThisMatch + lengthOfMatch;

				endOfLastMatch = startOfThisMatch;
				std::advance(endOfLastMatch, lengthOfMatch);
			};

		boost::regex_iterator<BidirIt> begin(first, last, re), end;
		std::for_each(begin, end, callback);

		s.append(endOfLastMatch, last);

		return s;
	}

	template<class Traits, class CharT, class UnaryFunction>
	static std::string RegexReplace(const std::string& s, const boost::basic_regex<CharT, Traits>& re, UnaryFunction f)
	{
		return RegexReplace(s.cbegin(), s.cend(), re, f);
	}

	// Small utility to get a value from a map, if it doesn't exist, use a default value instead
	template<class K, class V>
	static V GetValueOr(const std::unordered_map<K, V>& map, const std::type_identity_t<K>& key, const std::type_identity_t<V>& opt)
	{
		auto it = map.find(key);
		if (it == map.end()) return opt;
		return it->second;
	}

	// All the regexs uses
	static const boost::regex sRegexToken("%\\{(\\w+)\\}");
	static const boost::regex sRegexIn("^in\\s+(\\w+)\\s+(\\w+)\\s*=\\s*([0-9]+)\\s*;");
	static const boost::regex sRegexOut("^out\\s+(\\w+)\\s+(\\w+)\\s*=\\s*([0-9]+)\\s*;");
	static const boost::regex sRegexVary("^varying\\s+(\\w+)\\s+(\\w+)\\s*;");
	static const boost::regex sRegexFunc("^\\s*\\[\\[\\s*(\\w+)\\s*\\]\\]\\s*(\\w+\\s+\\w+\\s*\\(.*?\\)[\\S\\s]*?{)");
	static const boost::regex sRegexInclude("^\\s*#\\s*include\\s+<\\s*(\\w+)\\s*>");

	// Adds #line x pragmas to every line, allows accurate error locations after preprocessing occurs
	static std::string ProcessLinePragmas(const std::string& source)
	{
		std::stringstream out;
		std::stringstream in(source);

		int index = 1;

		for (std::string line; std::getline(in, line);)
			out << "#line " << index++ << '\n' << line << '\n';

		return out.str();
	}

	// Recusivly handle includes
	static std::string ProcessIncludes(const std::string& source, const std::unordered_map<std::string, std::string>& table)
	{
		bool ran_at_least_once = false;

		std::string processed = source;

		int recurse_count = 0;

		do
		{
			ran_at_least_once = false;
			processed = RegexReplace(processed, sRegexInclude, [&table, &ran_at_least_once](const boost::smatch& match) -> std::string
				{
					ran_at_least_once = true;
					return GetValueOr(table, match[1], std::string("// " + match[0].str() + " // Include not found"));
				});

			++recurse_count;

			if (recurse_count >= 8) break;

		} while (ran_at_least_once);

		return processed;
	}

	// Replace all tokens with their matches
	static std::string ProcessTokens(const std::string& source, const std::unordered_map<std::string, std::string>& table)
	{
		return RegexReplace(source, sRegexToken, [&table](const boost::smatch& match) -> std::string
			{
				return GetValueOr(table, match[1], "");
			});
	}

	// Replace in/out/varying variables with their correct match for the shader type
	static std::string ProcessInOutVarying(std::string source, bool vert)
	{
		if (vert)
		{
			source = boost::regex_replace(source, sRegexOut, "");
			source = boost::regex_replace(source, sRegexIn, "layout(location = $3) in $1 $2;");
			source = boost::regex_replace(source, sRegexVary, "out $1 $2;");
		}
		else
		{
			source = boost::regex_replace(source, sRegexOut, "layout(location = $3) out $1 $2;");
			source = boost::regex_replace(source, sRegexIn, "");
			source = boost::regex_replace(source, sRegexVary, "in $1 $2;");
		}

		return source;
	}

	// Remove function attributes or entire functions depending on attributes
	static std::string ProcessFunctions(std::string source, const std::string& attribute_removal)
	{
		boost::smatch match;
		while (boost::regex_search(source, match, sRegexFunc))
		{
			// This function does not need removed, but the attribute does
			if (match[1] != attribute_removal)
			{
				std::string prefix = source.substr(0, match.position());
				std::string f = match[2];
				std::string suffix = match.suffix();
				source = source.substr(0, match.position()) + match[2].str() + match.suffix().str();
				continue;
			}

			// This function needs removed, find matching close bracket

			int level = 0;
			size_t ending_pos = std::string::npos;
			std::string search = match.suffix();

			for (size_t i = 0; i < search.size(); ++i)
			{
				char c = search[i];

				// Another open bracket, we must match this first, keep track of it
				if (c == '{')
				{
					++level;
					continue;
				}

				// Closing bracket, make sure all internal brackets are matched first
				if (c == '}')
				{
					// All brackets matched, we are done
					if (level == 0)
					{
						ending_pos = i;
						break;
					}

					// Brackets not all matched, decrease level, keep iterating
					--level;
					continue;
				}
			}

			// Update source code without the interfering function
			std::string prefix = source.substr(0, match.position());
			std::string suffix = search.substr(ending_pos + 1);
			source = prefix + suffix;
		}
		return source;
	}

	// Insert defines into the top of the file
	static std::string ProcessDefines(std::string source, const std::unordered_set<std::string>& set)
	{
		for (const auto& define : set)
			source = std::string("#define ") + define + std::string("\n") + source;

		return source;
	}

	// Look at the #line pragmas, if matching lines are found, remove them to shorten the source file length
	static std::string ProcessLinePragmaRemoval(const std::string& source)
	{
		std::stringstream out;
		std::stringstream in(source);

		int index = 1;

		for (std::string line; std::getline(in, line); )
		{
			bool write = true;

			if (line.starts_with("#line "))
			{
				int pos = std::stoi(line.substr(6));

				if (pos == index)
					write = false;
				else
					index = pos - 1;
			}

			if (write)
			{
				out << line << '\n';
				index++;
			}
		}

		return out.str();
	}

	ShaderPreprocessor::Sources ShaderPreprocessor::Process(std::string_view source)
	{
		const std::string define_string = "#define ";

		Sources sources;

		sources.vert = std::string(source);
		sources.vert = ProcessLinePragmas(sources.vert);
		sources.vert = ProcessIncludes(sources.vert, mIncludes);
		sources.vert = ProcessTokens(sources.vert, mTokens);
		sources.vert = ProcessInOutVarying(sources.vert, true);
		sources.vert = ProcessFunctions(sources.vert, mFragmentAttribute);
		sources.vert = define_string + mVertexDefine + '\n' + sources.vert;
		sources.vert = ProcessDefines(sources.vert, mDefines);
		sources.vert = mPragma + '\n' + sources.vert;
		sources.vert = ProcessLinePragmaRemoval(sources.vert);

		sources.frag = std::string(source);
		sources.frag = ProcessLinePragmas(sources.frag);
		sources.frag = ProcessIncludes(sources.frag, mIncludes);
		sources.frag = ProcessTokens(sources.frag, mTokens);
		sources.frag = ProcessInOutVarying(sources.frag, false);
		sources.frag = ProcessFunctions(sources.frag, mVertexAttribute);
		sources.frag = define_string + mFragmentDefine + '\n' + sources.frag;
		sources.frag = ProcessDefines(sources.frag, mDefines);
		sources.frag = mPragma + '\n' + sources.frag;
		sources.frag = ProcessLinePragmaRemoval(sources.frag);

		mTokens.clear();
		mDefines.clear();

		return sources;
	}
}