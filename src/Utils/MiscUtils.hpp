#pragma once

#include "DataHandler.hpp"

class MiscUtils
{
	public:

	template <typename T = RE::TESObjectREFR, typename HandleT>
	static T* ResolveHandle(const HandleT& handle)
	{
		auto ptr = handle ? handle.get() : nullptr;
		if (!ptr) return nullptr;

		return ptr->As<T>();
	}

	static REL::Version GetPluginVersion(const char* a_moduleName)
	{
		const auto handle = GetModuleHandleA(a_moduleName);
		if (!handle) return REL::Version{};

		char path[MAX_PATH]{};
		if (!GetModuleFileNameA(handle, path, MAX_PATH)) return REL::Version{};

		DWORD dummy = 0;
		const DWORD size = GetFileVersionInfoSizeA(path, &dummy);
		if (size == 0) return REL::Version{};

		std::vector<std::byte> data(size);
		if (!GetFileVersionInfoA(path, 0, size, data.data())) return REL::Version{};

		VS_FIXEDFILEINFO* fileInfo = nullptr;
		UINT fileInfoLen = 0;
		if (!VerQueryValueA(data.data(), "\\", reinterpret_cast<LPVOID*>(&fileInfo), &fileInfoLen)) return REL::Version{};
		if (!fileInfo) return REL::Version{};

		return REL::Version{
			HIWORD(fileInfo->dwFileVersionMS),
			LOWORD(fileInfo->dwFileVersionMS),
			HIWORD(fileInfo->dwFileVersionLS),
			LOWORD(fileInfo->dwFileVersionLS)
		};
	}

	template<typename T>
	static T GetGameSetting(const std::string& settingName, const T& defaultValue = T{})
	{
		auto* gsc = RE::GameSettingCollection::GetSingleton();
		if (!gsc) return defaultValue;

		auto* setting = gsc->GetSetting(settingName.c_str());
		if (!setting) {
			logger::warn("GetGameSetting: setting \"{}\" not found", settingName);
			return defaultValue;
		}

		using SettingType = RE::Setting::Type;
		switch (setting->GetType()) {
			case SettingType::kBool: if constexpr (std::is_same_v<T, bool>) return setting->data.b; break;
			case SettingType::kFloat: if constexpr (std::is_same_v<T, float>) return setting->data.f; break;
			case SettingType::kInteger: if constexpr (std::is_same_v<T, int32_t>) return setting->data.i; break;
			case SettingType::kUnsignedInteger: if constexpr (std::is_same_v<T, uint32_t>) return setting->data.u; break;
			case SettingType::kString:
				if constexpr (std::is_same_v<T, std::string>) {
					return (setting->data.s && !IsBadReadPtr(setting->data.s, 1)) ? std::string(setting->data.s) : defaultValue;
				}
				break;
			default: break;
		}

		return defaultValue;
	}
};
