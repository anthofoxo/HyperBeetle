#pragma once

#include <vector>
#include <string>

namespace hyperbeetle {
	void SetupLogger();
	std::vector<std::string>& GetLogs();
}