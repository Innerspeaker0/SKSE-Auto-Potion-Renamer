#include "AlchemyRenamer.h"

#include "logger.h"
#include "Utils.h"

namespace Hooks {
    void AlchemyRenamer::RenameAlchemyItem(RE::TESDataHandler* a_dataHandler, RE::AlchemyItem* a_alchemyItem) {
        _AddForm(a_dataHandler, a_alchemyItem);

        auto potions = Settings::SettingsLoader::GetSingleton()->potions;

        if (a_alchemyItem->effects.size() > 1) {  // Don't rename 1-effect potions
            logger::trace("Checking through all {} potions", potions.size());
            for (auto& potion : potions) {
                if (PotionsMatch(potion, a_alchemyItem)) {
                    a_alchemyItem->fullName = GetFormattedName(a_alchemyItem, potion.name, potion.format, potion.descriptorIndex);
                    break;
                }
            }
        }

        return;
    }

    std::string AlchemyRenamer::GetFormattedName(RE::AlchemyItem* a_alchemyItem, std::string_view inputName,
                                                 Settings::SettingsLoader::DescriptorFormat format, int descriptorCategory) {
        auto costliestEffect = a_alchemyItem->GetCostliestEffectItem();

        auto settings = Settings::SettingsLoader::GetSingleton();

        float potency = EstimatePotency(*costliestEffect, settings->potencies);

        // Numerals and descriptors are placed in different positions
        if (settings->useRomanNumerals) {
            const char* numeral = romanNumerals[(int)(potency * 19)];
            return std::vformat(inputName, std::make_format_args("")) + " "s + (std::string)numeral;
        } else {
            using DFormat = Settings::SettingsLoader::DescriptorFormat;

            if (descriptorCategory == -1 || settings->descriptors[descriptorCategory].size() == 0) {
                return format == DFormat::Both ? std::vformat(inputName, std::make_format_args(" "))
                                               : std::vformat(inputName, std::make_format_args(""));
            } else {
                const RE::BSTArray<std::string>& potencyNames = settings->descriptors[descriptorCategory];
                const int index = (int)(potency * (potencyNames.size() - 1));

                const std::string& descriptor =
                    (potencyNames[index] == "") ? (format == DFormat::Both ? " " : "") : 
                                               ((format == DFormat::Before) ? " " + potencyNames[index]
                                             : ((format == DFormat::After)  ? potencyNames[index] + " "
                                                                            : " " + potencyNames[index] + " "));

                return std::vformat(inputName, std::make_format_args(descriptor));
            }
        }
    }

    bool AlchemyRenamer::PotionsMatch(const Settings::SettingsLoader::CustomPotion& customPotion, RE::AlchemyItem* skyrimPotion) {
        if (!skyrimPotion) {  // Check for null pointer
            logger::warn("Skyrim potion was null");
            return false;
        }
        if (customPotion.effectCount != skyrimPotion->effects.size()) {
            logger::trace("Expected {} effects for \"{}\", found {} in the Skyrim potion", customPotion.effectCount,
                          customPotion.name, skyrimPotion->effects.size());
            return false;
        }
        for (auto& effect : skyrimPotion->effects) {
            int formID = effect->baseEffect->GetFormID();
            bool matches = false;
            for (int i = 0; i < customPotion.effectCount; i++) {
                if (formID == customPotion.effectIDs[i]) {
                    matches = true;
                    break;
                }
            }
            if (!matches) {
                logger::trace("Did not find a match for formID '{}' in \"{}\"", Utils::GetHexString(formID),
                              customPotion.name);
                return false;
            }
        }
        logger::trace("Found match with potion \"{}\"", customPotion.name);
        return true;
    }

    void AlchemyRenamer::SetUpHook() {
        // Hook to game's CreateFromEffects method
        const auto gameHook = Utils::MakeHook(REL::ID(36179), 0x16F);

        auto& trampoline = SKSE::GetTrampoline();
        _AddForm = trampoline.write_call<5>(gameHook.address(), &RenameAlchemyItem);
    }

    float AlchemyRenamer::EstimatePotency(const RE::Effect& costliestEffect, Settings::SettingsLoader::PotencyMap potencyMap) {
        float workingMin = DEFAULT_MIN_POTENCY;
        float workingMax = DEFAULT_MAX_POTENCY;
        std::string name;
        if (potencyMap.contains(name = Utils::GetFormEditorID(costliestEffect.baseEffect)) ||
            potencyMap.contains(name = costliestEffect.baseEffect->GetName())) {
            // We guarantee that every potencyMap entry has min and max during parsing
            workingMin = potencyMap[name]["min"];
            workingMax = potencyMap[name]["max"];
        } else {
            logger::warn("Did not find potency entry for {} [{}]", name, Utils::GetFormEditorID(costliestEffect.baseEffect));
        }

        using EffectFlag = RE::EffectSetting::EffectSettingData::Flag;

        if (costliestEffect.baseEffect->data.flags & EffectFlag::kPowerAffectsMagnitude) {
            return std::clamp((costliestEffect.effectItem.magnitude - workingMin) / (workingMax - workingMin), 0.0f,
                              1.0f);
        } else if (costliestEffect.baseEffect->data.flags & EffectFlag::kPowerAffectsDuration) {
            return std::clamp((costliestEffect.effectItem.duration - workingMin) / (workingMax - workingMin), 0.0f,
                              1.0f);
        } else {  // Neither magnitude nor duration are affected - hopefully this is not possible
            logger::trace("Neither magnitude nor duration are affected by power - defaulting to 0.5 potency");
            return 0.5f;
        }
    }
}