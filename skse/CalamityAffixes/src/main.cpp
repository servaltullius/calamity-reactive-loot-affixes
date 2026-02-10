#include <filesystem>
#include <cstdint>

#include <RE/Skyrim.h>
#include <SKSE/Logger.h>
#include <SKSE/SKSE.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/Hooks.h"
#include "CalamityAffixes/Papyrus.h"
#include "CalamityAffixes/PrismaTooltip.h"
#include "CalamityAffixes/TrapSystem.h"

using namespace std::literals;

SKSEPluginInfo(
	.Version = REL::Version{ 1, 0, 0, 0 },
	.Name = "CalamityAffixes"sv,
	.Author = ""sv,
	.SupportEmail = ""sv,
	.StructCompatibility = SKSE::StructCompatibility::Independent,
	.RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary,
	.MinimumSKSEVersion = REL::Version{ 0, 0, 0, 0 }
)

namespace
{
	constexpr std::uint32_t kBuildSeq = 31;

	void SetupLogging()
	{
		const auto logDir = SKSE::log::log_directory();
		if (!logDir) {
			return;
		}

		auto path = *logDir / "CalamityAffixes.log";
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.string(), true);

		auto logger = std::make_shared<spdlog::logger>("", std::move(sink));
		spdlog::set_default_logger(std::move(logger));
		spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
		spdlog::set_level(spdlog::level::info);
		spdlog::flush_on(spdlog::level::info);
	}

	void OnSave(SKSE::SerializationInterface* a_intfc)
	{
		CalamityAffixes::EventBridge::GetSingleton()->Save(a_intfc);
	}

	void OnLoad(SKSE::SerializationInterface* a_intfc)
	{
		CalamityAffixes::EventBridge::GetSingleton()->Load(a_intfc);
	}

	void OnRevert(SKSE::SerializationInterface* a_intfc)
	{
		CalamityAffixes::EventBridge::GetSingleton()->Revert(a_intfc);
	}

	void OnFormDelete(RE::VMHandle a_handle)
	{
		CalamityAffixes::EventBridge::GetSingleton()->OnFormDelete(a_handle);
	}

	void OnMessage(SKSE::MessagingInterface::Message* a_message)
	{
		if (!a_message) {
			return;
		}

		switch (a_message->type) {
		case SKSE::MessagingInterface::kDataLoaded: {
			auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
				bridge->LoadConfig();
				bridge->Register();
				CalamityAffixes::Hooks::Install();
				CalamityAffixes::PrismaTooltip::Install();
				CalamityAffixes::TrapSystem::Install();

				if (auto* console = RE::ConsoleLog::GetSingleton()) {
					console->Print("CalamityAffixes: EventBridge registered.");
				}
			break;
		}
		default:
			break;
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	SetupLogging();
	SKSE::log::info("CalamityAffixes: build {} {} (seq={})", __DATE__, __TIME__, kBuildSeq);
	SKSE::AllocTrampoline(1 << 10);

	SKSE::log::info("CalamityAffixes: plugin loaded, waiting for kDataLoaded.");
	if (auto* serialization = SKSE::GetSerializationInterface()) {
		serialization->SetUniqueID('CAFX');
		serialization->SetSaveCallback(OnSave);
		serialization->SetLoadCallback(OnLoad);
		serialization->SetRevertCallback(OnRevert);
		serialization->SetFormDeleteCallback(OnFormDelete);
	}
	if (auto* messaging = SKSE::GetMessagingInterface()) {
		messaging->RegisterListener(OnMessage);
	}
	if (auto* papyrus = SKSE::GetPapyrusInterface()) {
		papyrus->Register(CalamityAffixes::Papyrus::Register);
	}
	return true;
}
