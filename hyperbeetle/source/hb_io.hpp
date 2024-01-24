#pragma once

#include <filesystem>
#include <expected.hpp>
#include "hb_blob.hpp"

namespace hyperbeetle {
	[[nodiscard]] rd::expected<Blob, std::string> LoadResourceBlob(std::filesystem::path const& resource);
	[[nodiscard]] rd::expected<std::string, std::string> LoadResourceString(std::filesystem::path const& resource);
}