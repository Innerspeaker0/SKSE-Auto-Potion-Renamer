#pragma once

#include "SettingsLoader.h"

namespace Hooks {
    class AlchemyRenamer {
    private:

        const static int DEFAULT_MIN_POTENCY = 5;
        const static int DEFAULT_MAX_POTENCY = 100;

        static constexpr const char* romanNumerals[] = {"I",    "II",  "III",  "IV",    "V",   "VI",   "VII",
                                                        "VIII", "IX",  "X",    "XI",    "XII", "XIII", "XIV",
                                                        "XV",   "XVI", "XVII", "XVIII", "XIX", "XX"};

        // Include a space after each name to allow for the empty name.
        static constexpr const char* potencyNames[] = {"Weak ", "", "Potent ", "Virulent ", "Deadly "};

        static void RenameAlchemyItem(RE::TESDataHandler* a_dataHandler, RE::AlchemyItem* a_alchemyItem);

        static std::string GetFormattedName(RE::AlchemyItem* a_alchemyItem, std::string_view inputName);

        static bool PotionsMatch(const Settings::SettingsLoader::CustomPotion& customPotion, RE::AlchemyItem* skyrimPotion);

        inline static REL::Relocation<decltype(&AlchemyRenamer::RenameAlchemyItem)> _AddForm;

    public:
        AlchemyRenamer() = delete;

        static void SetUpHook();

        static float EstimatePotency(const RE::Effect& costliestEffect, Settings::SettingsLoader::PotencyMap potencyMap);

    };
}