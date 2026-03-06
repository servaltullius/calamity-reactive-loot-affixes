#pragma once

#include <exception>
#include <string_view>
#include <utility>

namespace CalamityAffixes
{
	template <class LoggerConfigurator, class ErrorHandler>
	[[nodiscard]] bool ConfigurePluginLogger(
		LoggerConfigurator&& a_configureLogger,
		ErrorHandler&& a_onError)
	{
		try {
			std::forward<LoggerConfigurator>(a_configureLogger)();
			return true;
		} catch (const std::exception& e) {
			std::forward<ErrorHandler>(a_onError)(e.what());
		} catch (...) {
			std::forward<ErrorHandler>(a_onError)("unknown exception");
		}

		return false;
	}
}
