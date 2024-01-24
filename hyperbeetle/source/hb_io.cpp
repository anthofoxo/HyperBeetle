#include "hb_io.hpp"

#include <fstream>
#include <new>

#include <tracy/Tracy.hpp>
#include <spdlog/spdlog.h>

namespace hyperbeetle {
	[[nodiscard]] rd::expected<Blob, std::string> LoadResourceBlob(std::filesystem::path const& resource) {
		ZoneScoped;
		std::ifstream stream;
		stream.open(resource, std::ifstream::in | std::ios::binary);
		if (!stream) return rd::unexpected("Failed to open file stream");
		stream.seekg(0, std::ios::end);
		std::streampos length = stream.tellg();
		stream.seekg(0, std::ios::beg);
		Blob blob(new char[length], length, [](Blob& blob) noexcept { delete[] blob.Data(); });
		stream.read(blob.Data<char>(), blob.Size());
		spdlog::info("Loaded resource: {}", resource.generic_string());
		return blob;
	}

	[[nodiscard]] rd::expected<std::string, std::string> LoadResourceString(std::filesystem::path const& resource) {
		ZoneScoped;
		auto expectedBlob = LoadResourceBlob(resource);
		if (!expectedBlob.has_value()) return rd::unexpected(expectedBlob.error());
		Blob const& blob = expectedBlob.value();
		std::string string;
		string.assign(blob.Data<char>(), blob.Size());
		return string;
	}
}