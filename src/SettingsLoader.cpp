#include "SettingsLoader.h"
#include "logger.h"
#include "Utils.h"

namespace Settings {

    SettingsLoader* SettingsLoader::GetSingleton() { 
        static SettingsLoader singleton{};
        return &singleton;
    }

    void SettingsLoader::LoadSettings() {
        std::filesystem::path jsonPath = std::filesystem::current_path();
        jsonPath += "\\Data\\SKSE\\Plugins\\AutoPotionRenamer";
        bool readSettings = false;
        for (const auto& entry : std::filesystem::directory_iterator(jsonPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                if (!readSettings && entry.path().filename() == "UserSettings.json") {
                    readSettings = ReadSettingsFile(entry.path());
                } else {
                    ReadPotionFile(entry.path());
                }
            }
        }
    }

    bool SettingsLoader::ReadPotionFile(const std::filesystem::path& jsonPath) {
        bool readAnything = false;
        try {
            Json::Value root = GetJson(jsonPath);

            if (!root["potions"] || !root["potions"].isArray()) {
                logger::error("Failed to read \"potions\" option");
            } else if (root["potions"].size() > MAX_POTIONS) {
                logger::error("Failed to read \"potions\" option - exceeded maximum");
            } else {
                ReadPotionsIn(root["potions"]);
                readAnything = true;
            }

            if (!root["effectPotencies"]) {
                logger::info("\"effectPotencies\" not found, will use default values");
            } else {
                ReadEffectsIn(root["effectPotencies"]);
                readAnything = true;
            }

        } catch (std::exception& e) {
            logger::error("Encountered an error while parsing file {}: {}", jsonPath.string(), e.what());
            readAnything = false;
        }
        return readAnything;
    }

    bool SettingsLoader::ReadSettingsFile(const std::filesystem::path& jsonPath) {
        bool readAnything = false;
        try {
            Json::Value settingsRoot = GetJson(jsonPath);

            if (settingsRoot["useRomanNumerals"] && settingsRoot["useRomanNumerals"].isBool()) {
                useRomanNumerals = settingsRoot["useRomanNumerals"].asBool();
                logger::info("Read useRomanNumerals={}", useRomanNumerals ? "True" : "False");
                readAnything = true;
            } else {
                logger::error("Failed to read useRomanNumerals option");
            }
        } catch (std::exception& e) {
            logger::error("Encountered an error while parsing file {}: {}", jsonPath.string(), e.what());
            readAnything = false;
        }
        return readAnything;
    }

    void SettingsLoader::ReadPotionsIn(const Json::Value& potionsRoot) {
        for (const auto& potion : potionsRoot) {
            const auto& effectFormIDs = potion["effectFormIDs"];

            if (effectFormIDs.isArray() && effectFormIDs.size() <= MAX_EFFECTS) {
                const int effectCount = effectFormIDs.size();
                int parsedIDs[MAX_EFFECTS] = {};
                bool problemLoadingEffects = false;
                for (int i = 0; i < effectCount; i++) {
                    const auto form = GetFormFromIdentifier(effectFormIDs[i].asString());
                    if (!form || !form->Is(RE::FormType::MagicEffect)) {
                        logger::error("Failed to load formID \"{}\", skipping potion", effectFormIDs[i].asString());
                        problemLoadingEffects = true;
                    } else {
                        parsedIDs[i] = form->formID;
                    }
                }

                if (problemLoadingEffects) {
                    continue;
                }

                if (potion["name"].isString()) {
                    CustomPotion customPotion = {parsedIDs, potion["name"].asString(), effectCount};
                    potions.push_back(customPotion);

                    std::string infoFormIDString = "";
                    for (auto id : customPotion.effectIDs)
                        infoFormIDString += id == 0 ? "" : (Utils::GetHexString(id) + ", ");
                    logger::info("\"{}\" has been read with effect formIDs: {}", customPotion.name,
                                    infoFormIDString);

                } else {
                    logger::error("Could not read potion name");
                    continue;
                }
            } else {
                logger::error("Could not read potion effects");
                continue;
            }
        }

        logger::info("Successfully loaded {} potions", potions.size());
    }

    // Source: https://github.com/Exit-9B/AlchemyPlus/blob/main/src/Settings/UserSettings.cpp
    RE::TESForm* SettingsLoader::GetFormFromIdentifier(const std::string& a_identifier) {
        std::istringstream ss{a_identifier};
        std::string plugin, id;

        std::getline(ss, plugin, '|');
        std::getline(ss, id);
        RE::FormID rawFormID;
        std::istringstream(id) >> std::hex >> rawFormID;

        const auto dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            logger::error("Data handler not found");
        }
        return dataHandler->LookupForm(rawFormID, plugin);
    }

    void SettingsLoader::ReadEffectsIn(const Json::Value& effectsRoot) {
        // Duplicate entries are overwritten
        for (const auto& potencyName : effectsRoot.getMemberNames()) {
            const auto& potency = effectsRoot[potencyName];
            if (potency.isMember("min") && potency.isMember("max")) {
                if (potency["min"].isNumeric() && potency["max"].isNumeric()) {
                    std::map<std::string, float> innerMap = {};
                    innerMap["min"] = potency["min"].asFloat();
                    innerMap["max"] = potency["max"].asFloat();
                    potencies[potencyName] = innerMap;
                    logger::info("Loaded \"{}\" with min potency {} and max potency {}", potencyName, innerMap["min"],
                                 innerMap["max"]);
                } else {
                    logger::warn("{} did not have numeric min/max values, skipping", potencyName);
                    continue;
                }
            } else {
                logger::warn("{} did not have {} field, ignoring", potencyName,
                             effectsRoot[potencyName].isMember("min") ? "max" : "min");
                continue;
            }
        }
    }

    Json::Value SettingsLoader::GetJson(const std::filesystem::path& jsonPath) {
        logger::info("Reading {} ...", jsonPath.filename().string());
        std::ifstream reader;
        reader.open(jsonPath);

        if (!reader.is_open()) {
            logger::error("Failed to open {}", jsonPath.filename().string());
            throw std::runtime_error("Failed to open file");
        }

        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errs;

        if (!Json::parseFromStream(builder, reader, &root, &errs)) {
            logger::error("Json error: {}", errs);
            throw std::runtime_error(errs);
        }

        reader.close();
        return root;
    }
}