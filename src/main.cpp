#include <iostream>
#include <sstream>
#include <string>

void ListenerThing(F4SE::MessagingInterface::Message* a_thing)
{
	if (a_thing->type == F4SE::MessagingInterface::kGameDataReady) {
		if (auto dataHandler = RE::TESDataHandler::GetSingleton(); dataHandler) {
			for (auto currentNPC : dataHandler->GetFormArray<RE::TESNPC>()) {
				if (currentNPC->HasPCLevelMult()) {
					currentNPC->actorData.actorBaseFlags.reset(RE::ACTOR_BASE_DATA::Flag::kPCLevelMult);
					if (currentNPC->actorData.calcLevelMin < currentNPC->actorData.calcLevelMax) {
						currentNPC->actorData.level = currentNPC->actorData.calcLevelMax;
					} else {
						currentNPC->actorData.level = currentNPC->actorData.calcLevelMin;
					}
				}
			}
		} else {
			logger::warn("Unable to access DataHandler! Nothing will be changed...");
		}
	}
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= "GLXRM_ScalingFlagRemover.log"sv;
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::warn);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);

	logger::info("{} v{}", Version::PROJECT, Version::NAME);

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = "GLXRM_ScalingFlagRemover";
	a_info->version = 1;

	if (a_f4se->IsEditor()) {
		logger::critical("loaded in editor");
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162) {
		logger::critical("unsupported runtime v{}", ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);

	F4SE::GetMessagingInterface()->RegisterListener(ListenerThing);

	return true;
}
