#pragma once

namespace Settings {
	class SettingsLoader {
    private:
        static const char MAX_POTIONS = 15;
        static const char MAX_POTENCIES = 31;
        static const int MAX_EFFECTS = 4;

        bool ReadPotionFile(const std::filesystem::path& jsonPath);

        bool ReadSettingsFile(const std::filesystem::path& jsonPath);

        void ReadPotionsIn(const Json::Value& potionsRoot);

        void ReadEffectsIn(const Json::Value& effectsRoot);

        Json::Value GetJson(const std::filesystem::path& jsonPath);

        // Source: https://github.com/Exit-9B/AlchemyPlus/blob/main/src/Settings/UserSettings.cpp
        RE::TESForm* GetFormFromIdentifier(const std::string& a_identifier);

    public: 
        struct CustomPotion {
            int effectIDs[MAX_EFFECTS] = {};
            std::string name;
            int effectCount = 0;

            CustomPotion() = default;

            CustomPotion(std::initializer_list<int> effectIDs, std::string_view p_name) : name(p_name) {
                effectCount = effectIDs.size();
                std::copy(effectIDs.begin(), effectIDs.end(), this->effectIDs);
            };

            CustomPotion(int effectIDs[MAX_EFFECTS], std::string_view p_name, int p_effectCount)
                : name(p_name), effectCount(p_effectCount) {
                for (int i = 0; i < effectCount; i++) {
                    this->effectIDs[i] = effectIDs[i];
                }
            };
        };

        static SettingsLoader* GetSingleton();

        void LoadSettings();

        using PotionArray = RE::BSTArray<CustomPotion>;

        PotionArray potions = {};

        using PotencyMap = std::map<std::string, std::map<std::string, float>>;

        PotencyMap potencies = {};

        bool useRomanNumerals = false;
	};
}