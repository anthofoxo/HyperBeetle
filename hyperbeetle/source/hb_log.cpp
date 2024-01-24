#include "hb_log.hpp"

#include <tracy/Tracy.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/base_sink.h>

namespace hyperbeetle {
	namespace {
		std::vector<std::string> kLogs;

		template<typename Mutex>
		class EngineSink : public spdlog::sinks::base_sink<Mutex> {
		protected:
			void sink_it_(const spdlog::details::log_msg& msg) override {
				spdlog::memory_buf_t formatted;
				spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
				std::string str = fmt::to_string(formatted);
				kLogs.push_back(str);
				TracyMessage(str.data(), str.size());
			}

			void flush_() override {}
		};
	}

	void SetupLogger() {
		ZoneScoped;
		std::vector<spdlog::sink_ptr> sinks;
		sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
		sinks.push_back(std::make_shared<EngineSink<std::mutex>>());
		auto combined_logger = std::make_shared<spdlog::logger>("hyperbeetle", begin(sinks), end(sinks));
		spdlog::set_default_logger(combined_logger);
		spdlog::set_level(spdlog::level::trace);
	}

	std::vector<std::string>& GetLogs() {
		return kLogs;
	}
}