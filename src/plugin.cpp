#include "logger.h"
#include "AlchemyRenamer.h"
#include "SettingsLoader.h"

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    SKSE::AllocTrampoline(28);

    SetupLog();
    logger::info("Logging started. Setting up hook...");
    
    Hooks::AlchemyRenamer::SetUpHook();

    // Ensure that the DataHandler is ready before loading settings so that it can be used to look up formIDs
    SKSE::GetMessagingInterface()->RegisterListener([](auto msg) {
        switch (msg->type) {
            case SKSE::MessagingInterface::kDataLoaded: {
                logger::info("Loading settings...");
                Settings::SettingsLoader::GetSingleton()->LoadSettings();
            } break;
        }
    });

    return true;
}
