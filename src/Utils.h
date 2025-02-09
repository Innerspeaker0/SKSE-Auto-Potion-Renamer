#pragma once

namespace Utils {
	
    inline std::string GetHexString(int number) {
        std::stringstream stream;
        stream << std::setw(8) << std::setfill('0') << std::uppercase << std::hex << number;

        return stream.str();
    }

    // REQUIRES PO3'S TWEAKS
    inline std::string GetFormEditorID(const RE::TESForm* a_form) {
        using _GetFormEditorID = const char* (*)(std::uint32_t);

        static auto tweaks = GetModuleHandle(L"po3_Tweaks");
        static auto func = reinterpret_cast<_GetFormEditorID>(GetProcAddress(tweaks, "GetFormEditorID"));
        if (func) {
            return func(a_form->formID);
        }
        return {};
    }

    inline auto MakeHook(REL::ID a_id, std::ptrdiff_t a_offset = 0) {
        return REL::Relocation<std::uintptr_t>(a_id, a_offset);
    }
}