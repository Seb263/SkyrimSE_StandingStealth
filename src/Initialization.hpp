#pragma once

#include "DataHandler.hpp"
#include "SettingsIni.hpp"

#include "Core/Main.hpp"

#include "Utils/TimeUtils.hpp"

#include "API/Mod-API.h"
#include "API/CIF-API.h"

namespace ModData
{
	class DataHandler
	{
	public:
		bool preLoaded = false;
		bool postLoaded = false;
		bool postLoadedAlternate = false;

		static DataHandler* GetSingleton()
		{
			static DataHandler singleton;
			return &singleton;
		}

		void PreLoadData()
		{
			if (preLoaded) return;
			preLoaded = true;

			TESdataHandler = RE::TESDataHandler::GetSingleton();

			LoadPluginsForms();
		}

		void PostLoadData()
		{
			if (postLoaded) return;
			postLoaded = true;

			if (!LoadCIFApi()) return;
			
			ModData::CIF_API_Interface->RegisterPreHitCallback((std::string)ModData::MOD_NAME, 0, [](const std::string&, RE::HitData& hitData) {
				auto* victim = MiscUtils::ResolveHandle<RE::Actor>(hitData.target);
				auto* attacker = MiscUtils::ResolveHandle<RE::Actor>(hitData.aggressor);

				ModCore::Main::Process(victim, attacker, hitData);
			});
		}

		void PostLoadDataAlternate()
		{
			if (postLoadedAlternate) return;
			postLoadedAlternate = true;

			TimeUtils::DoWhile(100ms, [](TimeUtils::CallResult result, std::chrono::nanoseconds) {
				if (TimeUtils::IsEnd(result)) return true;

				auto player = RE::PlayerCharacter::GetSingleton();
				if (player && player->Is3DLoaded() && player->GetParentCell() && player->GetParentCell()->IsAttached()) {
					GetSingleton()->PostLoadData();
					return false;
				}

				return true;
			}, true);
		}

	private:
		static inline void LoadPluginsForms()
		{
			logger::info("Loading Plugins Froms Data...");

			for (const auto& formInfo : pluginForms) {
				*formInfo.formPtr = TESdataHandler->LookupForm(formInfo.formID, formInfo.pluginName.data());
				if (!*formInfo.formPtr && !formInfo.optional) {
					REPORT_AND_FAIL("ERROR: Form \"{}\" not found in \"{}\".", formInfo.pluginName, formInfo.name, formInfo.pluginName);
				}
			}

			logger::info("Loading Plugins Froms Data: DONE");
		}

		static inline bool LoadCIFApi()
		{
			using namespace ModData;

			constexpr REL::Version kRequiredVersion{ 2, 0, 0, 0 };
			const auto dllVersion = MiscUtils::GetPluginVersion("CoreImpactFramework.dll");
			const bool versionOk = dllVersion != REL::Version{} && dllVersion >= kRequiredVersion;

			auto* apiInterface = versionOk ? static_cast<CIF_API::Interface*>(CIF_API::GetAPI()) : nullptr;

			if (!apiInterface) {
				logger::error("Core Impact Framework API not found or version insufficient.");

				const std::string title = fmt::format("{}: Missing Requirement", MOD_NAME);
				const std::string msg_box = fmt::format(
					"The Core Impact Framework version {} or higher is required to run {}.\n\n"
					"Would you like to close the game and open the download page?",
					kRequiredVersion.string("."), MOD_NAME);

				if (REX::W32::MessageBoxA(nullptr, msg_box.c_str(), title.c_str(), MB_ICONWARNING | MB_YESNO) == IDYES) {
					::ShellExecuteA(nullptr, "open", "https://www.seb263.fr/short-url/cif-v2", nullptr, nullptr, SW_SHOWNORMAL);
					REX::W32::TerminateProcess(REX::W32::GetCurrentProcess(), EXIT_FAILURE);
				}
				return false;
			}

			CIF_API_Interface = apiInterface;
			logger::info("Core Impact Framework API v{} registered successfully.", apiInterface->GetVersion().string("."));
			
			return true;
		}
	};
}
