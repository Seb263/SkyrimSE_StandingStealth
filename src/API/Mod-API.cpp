#include "API/Mod-API.h"

#include "Core/Main.hpp"

#include "Utils/MiscUtils.hpp"

namespace STST_API
{
    class Impl_V1 : public Interface_V1
    {
    public:
        static Impl_V1* GetSingleton() noexcept
        {
            static Impl_V1 instance;
            return &instance;
        }

        REL::Version GetVersion() noexcept override
        {
			const auto plugin{ SKSE::PluginDeclaration::GetSingleton() };
			const auto version{ plugin->GetVersion() };

			return version;
        }

		IniValue GetIniValue(const std::string& key_section, const IniValue& defaultValue) noexcept override
		{
			return SettingsIni::SettingsManager::GetSingleton().GetValueVariant(key_section).value_or(defaultValue);
		}

		bool SetIniValue(const std::string& key_section, const IniValue& value) noexcept override
		{
			return std::visit([&](auto&& val) {
				return SettingsIni::SettingsManager::GetSingleton().SetValue(key_section, val);
			}, value);
		}

		bool IsStealthAttack(const RE::HitData hitData) noexcept override
		{
			auto* victim = MiscUtils::ResolveHandle<RE::Actor>(hitData.target);
			auto* attacker = MiscUtils::ResolveHandle<RE::Actor>(hitData.aggressor);

			return ModCore::Main::IsStealthAttack(victim, attacker, hitData.weapon);
		}

		bool IsStealthAttack(RE::Actor* victim, RE::Actor* attacker, RE::TESObjectWEAP* weapon) noexcept override
		{
			return ModCore::Main::IsStealthAttack(victim, attacker, weapon);
		}
    };
}

extern "C" DLLEXPORT void* SKSEAPI RequestPluginAPI(STST_API::InterfaceVersion version, const char* pluginName, REL::Version pluginVersion)
{
    if (!pluginName) {
        logger::error("STST_API::RequestPluginAPI called with a nullptr plugin name");
        return nullptr;
    }

    void* api = nullptr;

    switch (version)
    {
        case STST_API::InterfaceVersion::V1:
            api = STST_API::Impl_V1::GetSingleton();
            break;
        default:
            logger::warn("RequestPluginAPI called with invalid InterfaceVersion {}", static_cast<uint8_t>(version));
            return nullptr;
    }

    logger::info("RequestPluginAPI called: [InterfaceVersion:{}], [PluginName:{}], [PluginVersion:{}]",
		static_cast<uint8_t>(version) + 1, pluginName, pluginVersion.string("."));

    return api;
}
