#pragma once

namespace Settings {
	class SettingsLoader {
    private:
        static const char MAX_POTIONS = 15;
        static const char MAX_POTENCIES = 31;
        static const int MAX_EFFECTS = 4;

        void ReadPotionsIn(const Json::Value& potionsRoot, std::map<std::string, int>& descriptorNameMap,
                           int& descriptorIndex);

        void ReadEffectsIn(const Json::Value& effectsRoot);

        void ReadDescriptorsIn(const Json::Value& descriptorsRoot, std::map<std::string, int>& descriptorNameMap,
                               int& descriptorIndex);

        Json::Value GetJson(const std::filesystem::path& jsonPath);

        // Source: https://github.com/Exit-9B/AlchemyPlus/blob/main/src/Settings/UserSettings.cpp
        RE::TESForm* GetFormFromIdentifier(const std::string& a_identifier);

    public: 
        enum DescriptorFormat {
            Before,
            After,
            Both
        };

        struct CustomPotion {
            int effectIDs[MAX_EFFECTS] = {};
            std::string name;
            DescriptorFormat format = Both;
            int effectCount = 0;
            int descriptorIndex = -1;

            CustomPotion() = default;

            CustomPotion(int effectIDs[MAX_EFFECTS], std::string_view p_name, DescriptorFormat p_format, int p_effectCount, int p_descriptorIndex = -1)
                : name(p_name), format(p_format), effectCount(p_effectCount), descriptorIndex(p_descriptorIndex) {
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

        using DescriptorMap = RE::BSTArray<RE::BSTArray<std::string>>;
        
        DescriptorMap descriptors = {};

        bool useRomanNumerals = false;

    private: 
        
        void ReadPotionFile(const std::filesystem::path& jsonPath, std::map<std::string, int>& descriptorNameMap,
                            int& descriptorIndex);

        void ReadSettingsFile(const std::filesystem::path& jsonPath, std::map<std::string, int>& descriptorNameMap,
                              int& descriptorIndex);
	};
}