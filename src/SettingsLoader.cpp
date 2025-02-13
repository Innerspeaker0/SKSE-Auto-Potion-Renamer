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

        // Temporary map to link descriptor names to their indices during loading
        std::map<std::string, int> descriptorNameMap = {};
        int lastIndex = 0;

        ReadSettingsFile(jsonPath.string() + "\\UserSettings.json", descriptorNameMap, lastIndex);

        for (const auto& entry : std::filesystem::directory_iterator(jsonPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json" && entry.path().filename() != "UserSettings.json") {
                ReadPotionFile(entry.path(), descriptorNameMap, lastIndex);
            }
        }

        int categoryCount = 0;
        for (const auto& pair : descriptorNameMap) {
            if (descriptors[pair.second].size() == 0) {
                logger::warn(
                    "Warning: Descriptors not found for category \"{}\", '{{}}' will be stripped from potion names",
                    pair.first);
            } else {
                categoryCount++;
            }
            logger::trace("\"{}\" is at index {}", pair.first, pair.second);
        }
        logger::info("{} descriptor categories loaded", categoryCount);
    }

    void SettingsLoader::ReadPotionFile(const std::filesystem::path& jsonPath,
                                        std::map<std::string, int>& descriptorNameMap,
                                        int& descriptorIndex) {
        try {
            Json::Value root = GetJson(jsonPath);

            // Read potion definitions
            if (!root["potions"] || !root["potions"].isArray()) {
                logger::error("Failed to read \"potions\" option");
            } else if (root["potions"].size() > MAX_POTIONS) {
                logger::error("Failed to read \"potions\" option - exceeded maximum");
            } else {
                ReadPotionsIn(root["potions"], descriptorNameMap, descriptorIndex);
            }

            // Read effect potencies
            if (!root["effectPotencies"]) {
                logger::info("\"effectPotencies\" not found, will use default values");
            } else if (root["effectPotencies"].size() > MAX_POTENCIES) {
                logger::error("Failed to read \"effectPotencies\" option - exceeded maximum");
            } else {
                ReadPotenciesIn(root["effectPotencies"]);
            }

            // Read descriptor definitions
            if (root["descriptors"]) {
                if (root["descriptors"].size() > MAX_DESCRIPTORS) {
                    logger::error("Failed to read \"descriptors\" option - exceeded maximum");
                } else {
                    ReadDescriptorsIn(root["descriptors"], descriptorNameMap, descriptorIndex);
                }
            }

        } catch (std::exception& e) {
            logger::error("Encountered an error while parsing file {}: {}", jsonPath.filename().string(), e.what());
        }
    }

    void SettingsLoader::ReadSettingsFile(const std::filesystem::path& jsonPath,
                                          std::map<std::string, int>& descriptorNameMap,
                                          int& descriptorIndex) {
        try {
            Json::Value settingsRoot = GetJson(jsonPath);

            if (settingsRoot["useRomanNumerals"] && settingsRoot["useRomanNumerals"].isBool()) {
                useRomanNumerals = settingsRoot["useRomanNumerals"].asBool();
                logger::info("Read useRomanNumerals={}", useRomanNumerals ? "True" : "False");
            } else {
                logger::error("Failed to read useRomanNumerals option");
            }

            if (settingsRoot["descriptors"]) {
                ReadDescriptorsIn(settingsRoot["descriptors"], descriptorNameMap, descriptorIndex);
            } else {
                logger::warn("Descriptor definitions not found in UserSettings.json");
            }
        } catch (std::exception& e) {
            logger::error("Encountered an error while parsing file {}: {}", jsonPath.filename().string(), e.what());
        }
    }

    void SettingsLoader::ReadPotionsIn(const Json::Value& potionsRoot, std::map<std::string, int>& descriptorNameMap,
                                       int& descriptorIndex) {
        for (const auto& potion : potionsRoot) {
            DescriptorFormat format;
            std::string name = "ERRORNAME";
            int parsedIDs[MAX_EFFECTS] = {};
            int effectCount = 0;

            // Read name
            if (potion["name"].isString()) {
                name = potion["name"].asString();
                int open = std::count(name.begin(), name.end(), '{');
                int close = std::count(name.begin(), name.end(), '}');
                if (open != close || open > 1 || (open > 0 && name.at(name.find('{') + 1) != '}')) {
                    // Check for names that cannot be formatted correctly (and would cause a crash if used)
                    logger::error(
                        "Could not read potion name - name must contain 1 or fewer '{{}}' and no unbalanced braces");
                    continue;
                }

                // Precalculate descriptor formatting
                if (name.starts_with('{')) {
                    format = After;
                } else if (name.ends_with('}')) {
                    format = Before;
                } else {
                    format = Both;
                }
            } else {
                logger::error("Could not read potion name");
                continue;
            }

            // Read effects
            const auto& effectFormIDs = potion["effects"];
            if (effectFormIDs.isArray()) {
                effectCount = effectFormIDs.size();
                if (effectCount > MAX_EFFECTS || effectCount < 2) {
                    logger::error(
                        "Error loading effects - each potion must have between 2 and {} effects, skipping \"{}\"...",
                        MAX_EFFECTS, name);
                    continue;
                }
                bool problemLoadingEffects = false;
                for (int i = 0; i < effectCount; i++) {
                    RE::TESForm* form = nullptr;
                    const std::string id = effectFormIDs[i].asString();
                    // Load FormIDs in the form "Plugin.esp|BEEF0" or editor IDs in the form "AlchResistFrost"
                    switch (std::count(id.begin(), id.end(), '|')) {
                        case 0:  // Probably an editor ID
                            form = RE::TESForm::LookupByEditorID(id);
                            break;
                        case 1:  // Probably a form ID
                            form = GetFormFromIdentifier(id);
                            break;
                        default:  // Probably an error (caught by following 'if')
                            break;
                    }
                    if (!form) {
                        logger::error("Failed to load ID \"{}\", skipping \"{}\"...", effectFormIDs[i].asString(), name);
                        problemLoadingEffects = true;
                        break;
                    } else if (!form->Is(RE::FormType::MagicEffect)) {
                        logger::error(
                            "Form {} was loaded as \"{}\", which is not an alchemy effect, skipping \"{}\"...",
                            Utils::GetHexString(form->formID), Utils::GetFormEditorID(form), name);
                        problemLoadingEffects = true;
                        break;
                    } else {
                        parsedIDs[i] = form->formID;
                    }
                }

                if (problemLoadingEffects) {
                    continue;
                }            
            } else {
                logger::error("Could not read \"{}\" effects", name);
                continue;
            }

            // Read descriptors
            int potionDescriptorIndex = -1;
            if (potion["descriptor"]) {
                const std::string descriptorName = potion["descriptor"].asString();
                if (descriptorNameMap.contains(descriptorName)) {
                    // Look up the descriptor index
                    potionDescriptorIndex = descriptorNameMap[descriptorName];
                } else {  // We have found a potion defined before the descriptor it is referencing
                    descriptors.push_back({});
                    potionDescriptorIndex = descriptorIndex;
                    descriptorNameMap[potion["descriptor"].asString()] = descriptorIndex++;
                }
            } else {
                logger::warn("\"{}\" was missing \"descriptor\" field, '{{}}' will be removed.", name);
            }

            CustomPotion customPotion = {parsedIDs, name, format, effectCount, potionDescriptorIndex};
            potions.push_back(customPotion);

            std::string infoFormIDString = "";
            for (auto id : customPotion.effectIDs) infoFormIDString += id == 0 ? "" : (Utils::GetHexString(id) + ", ");
            logger::info("\"{}\" has been read with effect formIDs: {}", customPotion.name, infoFormIDString);
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

    void SettingsLoader::ReadPotenciesIn(const Json::Value& effectsRoot) {
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
                    logger::warn("\"{}\" did not have numeric min/max values, skipping", potencyName);
                    continue;
                }
            } else {
                logger::warn("{} did not have {} field, ignoring", potencyName,
                             effectsRoot[potencyName].isMember("min") ? "max" : "min");
                continue;
            }
        }
    }

    void SettingsLoader::ReadDescriptorsIn(const Json::Value& descriptorsRoot,
        std::map<std::string, int>& descriptorNameMap, int& descriptorIndex) {
        logger::trace("Reading descriptors...");
        for (const auto& categoryName : descriptorsRoot.getMemberNames()) {
            RE::BSTArray<std::string> categoryDescriptors = {};
            for (const auto& descriptor : descriptorsRoot[categoryName]) {
                categoryDescriptors.push_back(descriptor.asString());
            }
            if (!descriptorNameMap.contains(categoryName)) {
                // Add new descriptor category
                descriptors.push_back(categoryDescriptors);
                descriptorNameMap[categoryName] = descriptorIndex++;
                logger::info("Read descriptor category \"{}\"", categoryName);
            } else if (descriptors[descriptorNameMap[categoryName]].size() == 0) { 
                // Descriptor category name has been inferred from potion defintion
                descriptors[descriptorNameMap[categoryName]] = categoryDescriptors;
                logger::info("Filled descriptor category \"{}\"", categoryName);
            } else {  
                // Otherwise, do nothing so that the first definition loaded (from UserSettings.json) takes priority
                logger::info("Ignoring duplicate definition for \"{}\" category", categoryName);
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