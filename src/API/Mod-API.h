#pragma once

/*******************************************************************
* STANDING STEALTH - API
* Do not forget to include this source file to your project!
*******************************************************************/

/* How to create a hook to the API and use it:
SKSE::GetMessagingInterface()->RegisterListener([](MessagingInterface::Message* message) 
{
    switch (message->type) 
    {
        case MessagingInterface::kPostLoadGame:
        case MessagingInterface::kNewGame:
        {
            if (auto* apiInterface = static_cast<STST_API::Interface*>(STST_API::GetAPI())) {
				auto apiVersion = apiInterface->GetVersion().string(".");
				logger::info("Standing Stealth API v{} registered successfully.", apiVersion);
			} else {
				logger::warn("Standing Stealth API not found.");
			}
        }
        break;
    }
});
*/

namespace STST_API
{
	inline void* g_Interface = nullptr;

	enum class InterfaceVersion : uint8_t
	{
		V1,
		Latest = V1
	};

    class Interface_V1
    {
    public:
		virtual ~Interface_V1() = default;
		
		using IniValue = std::variant<bool, int, float, std::string>;

		virtual REL::Version GetVersion() noexcept = 0;

		virtual IniValue GetIniValue(const std::string& key_section, const IniValue& defaultValue = {}) noexcept = 0;

		virtual bool SetIniValue(const std::string& key_section, const IniValue& value) noexcept = 0;

		virtual bool IsStealthAttack(const RE::HitData hitData) noexcept = 0;
		
		virtual bool IsStealthAttack(RE::Actor* victim, RE::Actor* attacker, RE::TESObjectWEAP* weapon) noexcept = 0;
    };

	using Interface = Interface_V1;

	using _RequestPluginAPI = void* (*)(InterfaceVersion version, const char* pluginName, REL::Version pluginVersion);

    inline void* GetAPI(InterfaceVersion version = InterfaceVersion::Latest)
    {
        if (g_Interface) return g_Interface;

        const auto handle = GetModuleHandleA("StandingStealth.dll");
        if (!handle) return nullptr;

        const auto request = reinterpret_cast<_RequestPluginAPI>(GetProcAddress(handle, "RequestPluginAPI"));
        if (!request) return nullptr;

        const auto plugin = SKSE::PluginDeclaration::GetSingleton();
        g_Interface = request(version, std::string(plugin->GetName()).c_str(), plugin->GetVersion());

        return g_Interface;
    }
}
