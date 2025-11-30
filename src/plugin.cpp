#include "_ts_SKSEFunctions.h"
#include "Hooks.h"
#include "ControlsManager.h"

namespace FCSE {
    namespace Interface {
        int GetFCSEPluginVersion(RE::StaticFunctionTag*) {
            return 1;
        }

        bool FCSEFunctions(RE::BSScript::Internal::VirtualMachine * a_vm){
            a_vm->RegisterFunction("GetFCSEPluginVersion", "_ts_FCSE_PapyrusFunctions", GetFCSEPluginVersion);
            return true;
        }
    } // namespace Interface
} // namespace FCSE

/******************************************************************************************/
void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	// Try requesting APIs at multiple steps to try to work around the SKSE messaging bug
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		break;
	case SKSE::MessagingInterface::kPostLoad:
		break;
	case SKSE::MessagingInterface::kPostPostLoad:
		break;
	case SKSE::MessagingInterface::kPreLoadGame:
		break;
	case SKSE::MessagingInterface::kPostLoadGame:
     if (auto input = RE::BSInputDeviceManager::GetSingleton()) {
        log::info("FCSE - {}: BSInputDeviceManager available", __func__);
        input->AddEventSink(&FCSE::ControlsManager::GetSingleton());
    } else {
        log::warn("FCSE - {}: BSInputDeviceManager not available", __func__);
    }   
	case SKSE::MessagingInterface::kNewGame:
		break;
	}
}
/******************************************************************************************/
SKSEPluginInfo(
    .Version = Plugin::VERSION,
    .Name = Plugin::NAME,
    .Author = "Staalo",
    .RuntimeCompatibility = SKSE::PluginDeclaration::RuntimeCompatibility(SKSE::VersionIndependence::AddressLibrary),
    .MinimumSKSEVersion = { 2, 2, 3 } // or 0 if you want to support all
)

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* skse) {
    long logLevel = _ts_SKSEFunctions::GetValueFromINI(nullptr, 0, "LogLevel:Log", "SKSE/Plugins/IntuitiveDragonRideControl.ini", 3L);
    bool isLogLevelValid = true;
    if (logLevel < 0 || logLevel > 6) {
        logLevel = 2L; // info
        isLogLevelValid = false;
    }

	_ts_SKSEFunctions::InitializeLogging(static_cast<spdlog::level::level_enum>(logLevel));
    if (!isLogLevelValid) {
        log::warn("FCSE - {}: LogLevel in INI file is invalid. Defaulting to info level.", __func__);
    }
    log::info("FCSE - {}: FCSE Plugin version: {}", __func__, FCSE::Interface::GetFCSEPluginVersion(nullptr));

    Init(skse);
    auto messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", MessageHandler)) {
		return false;
	}

    if (!SKSE::GetPapyrusInterface()->Register(FCSE::Interface::FCSEFunctions)) {
        log::error("FCSE - {}: Failed to register Papyrus functions.", __func__);
        return false;
    } else {
        log::info("FCSE - {}: Registered Papyrus functions", __func__);
    }

    SKSE::AllocTrampoline(64);
    
    log::info("FCSE - {}: Calling Install Hooks", __func__);

    Hooks::Install();

    return true;
}
