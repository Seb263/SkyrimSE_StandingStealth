#pragma once

#include "DataHandler.hpp"
#include "SettingsIni.hpp"

#include "Core/Main.hpp"

#include "Utils/Debug.hpp"
#include "Utils/MiscUtils.hpp"

// Most of the content in this patch is based on the functionality of the MagicSneakAttacks plugin by powerof3.
// (I couldn't find a more elegant way to implement this patch without adding a new hook to CIF's API for 'CheckAddEffect', which is already handled.)
// It uses the same INI file and configuration variables to ensure seamless integration with the mod.
// https://github.com/powerof3/MagicSneakAttacks

// This compatibility patch will only work if the MagicSneakAttacks mod is installed.

namespace Compat::MagicSneakAttacks
{
	inline constexpr const char* kMsaIniPath = "Data/SKSE/Plugins/po3_MagicSneakAttacks.ini";

	inline bool installed = false;

	struct Config
	{
		// [Settings]
		bool hideNotifications = false;
		bool hideNotificationSound = false;
		float sneakAttackSkillUsageMagic = 2.5f;

		// [SneakAttackBaseMultipliers]
		float combatSneakMissileMult = 2.0f;
		float combatSneakLobberMult = 2.0f;
		float combatSneakBeamMult = 2.0f;
		float combatSneakFlamesMult = 2.0f;
		float combatSneakConeMult = 2.0f;
		float combatSneakBarrierMult = 2.0f;

		// [SpellSchools]
		bool alteration = true;
		bool conjuration = true;
		bool destruction = true;
		bool illusion = true;
		bool restoration = true;
		bool other = false;

		// [SpellTypes]
		bool staves = true;
		bool scrolls = true;
	};

	inline Config config{};

	[[nodiscard]] inline bool IsMsaLoaded() noexcept
	{
		return GetModuleHandleA("po3_MagicSneakAttacks.dll");
	}

	inline bool LoadMsaSettings()
	{
		Config cfg{}; // MSA defaults

		bool readStatus = false;

		if (std::filesystem::exists(kMsaIniPath)) {
			CSimpleIniA ini;
			ini.SetUnicode();

			if (ini.LoadFile(kMsaIniPath) >= 0) {
				const auto getBool = [&](const char* section, const char* key, bool& value) {
					value = ini.GetBoolValue(section, key, value);
				};
				const auto getFloat = [&](const char* section, const char* key, float& value) {
					value = static_cast<float>(ini.GetDoubleValue(section, key, value));
				};

				getBool("Settings", "bHideNotifications", cfg.hideNotifications);
				getBool("Settings", "bHideNotificationSound", cfg.hideNotificationSound);
				getFloat("Settings", "fSneakAttackSkillUsageMagic", cfg.sneakAttackSkillUsageMagic);

				getFloat("SneakAttackBaseMultipliers", "fCombatSneakMissileMult", cfg.combatSneakMissileMult);
				getFloat("SneakAttackBaseMultipliers", "fCombatSneakLobberMult", cfg.combatSneakLobberMult);
				getFloat("SneakAttackBaseMultipliers", "fCombatSneakBeamMult", cfg.combatSneakBeamMult);
				getFloat("SneakAttackBaseMultipliers", "fCombatSneakFlamesMult", cfg.combatSneakFlamesMult);
				getFloat("SneakAttackBaseMultipliers", "fCombatSneakConeMult", cfg.combatSneakConeMult);
				getFloat("SneakAttackBaseMultipliers", "fCombatSneakBarrierMult", cfg.combatSneakBarrierMult);

				getBool("SpellSchools", "bAlteration", cfg.alteration);
				getBool("SpellSchools", "bConjuration", cfg.conjuration);
				getBool("SpellSchools", "bDestruction", cfg.destruction);
				getBool("SpellSchools", "bIllusion", cfg.illusion);
				getBool("SpellSchools", "bRestoration", cfg.restoration);
				getBool("SpellSchools", "bOther", cfg.other);

				getBool("SpellTypes", "bStaves", cfg.staves);
				getBool("SpellTypes", "bScrolls", cfg.scrolls);

				readStatus = true;
			} else {
				logger::error("Magic Sneak Attacks compat: failed to load '{}'. MSA default values will be used.", kMsaIniPath);
			}
		} else {
			logger::warn("Magic Sneak Attacks compat: '{}' not found. MSA default values will be used.", kMsaIniPath);
		}

		config = cfg;
		return readStatus;
	}

	[[nodiscard]] inline float GetBaseSneakAttackMult(const RE::BGSProjectile* a_projectile)
	{
		if (a_projectile->IsMissile()) return config.combatSneakMissileMult;
		if (a_projectile->IsGrenade()) return config.combatSneakLobberMult;
		if (a_projectile->IsBeam()) return config.combatSneakBeamMult;
		if (a_projectile->IsFlamethrower()) return config.combatSneakFlamesMult;
		if (a_projectile->IsCone()) return config.combatSneakConeMult;
		if (a_projectile->IsBarrier()) return config.combatSneakBarrierMult;

		return 0.0f;
	}

	[[nodiscard]] inline bool IsSkillValid(RE::MagicItem* a_spell)
	{
		auto skill = a_spell->GetAssociatedSkill();
		if (a_spell->Is(RE::FormType::Scroll, RE::FormType::Enchantment)) {
			if (const auto effect = a_spell->GetCostliestEffectItem(); effect && effect->baseEffect) {
				skill = effect->baseEffect->GetMagickSkill();
			}
		}

		switch (skill) {
			case RE::ActorValue::kAlteration: return config.alteration;
			case RE::ActorValue::kConjuration: return config.conjuration;
			case RE::ActorValue::kDestruction: return config.destruction;
			case RE::ActorValue::kIllusion: return config.illusion;
			case RE::ActorValue::kRestoration: return config.restoration;
			default: return config.other;
		}
	}

	[[nodiscard]] inline bool IsSpellValid(RE::MagicItem* a_spell)
	{
		using namespace RE::MagicSystem;

		if (a_spell->GetCastingType() == CastingType::kConstantEffect || a_spell->HasKeywordString("MagicNoSneakAttack")) {
			return false;
		}

		switch (a_spell->GetSpellType()) {
			case SpellType::kScroll: return config.scrolls && IsSkillValid(a_spell);
			case SpellType::kStaffEnchantment: return config.staves && IsSkillValid(a_spell);
			default: return IsSkillValid(a_spell);
		}
	}

	[[nodiscard]] inline float GetStandingSneakAttackMult(RE::Actor* a_attacker, RE::Actor* a_target, RE::MagicItem* a_spell, const RE::BGSProjectile* a_projectile)
	{
		if (!a_attacker || !a_target || !a_spell || !a_projectile) return 0.0f;

		// Standing Stealth rules
		if (!ModCore::Main::IsStandingMagicStealthAttack(a_target, a_attacker)) return 0.0f;

		// MSA rules
		if (!IsSpellValid(a_spell)) return 0.0f;

		float sneakAttackMult = GetBaseSneakAttackMult(a_projectile);
		if (sneakAttackMult <= 1.0f) return 0.0f;

		RE::BGSEntryPoint::HandleEntryPoint(RE::BGSEntryPoint::ENTRY_POINT::kModSneakAttackMult, a_attacker, a_spell, a_target, std::addressof(sneakAttackMult));
		return sneakAttackMult;
	}

	struct AdjustActiveEffect
	{
		static void thunk(RE::ActiveEffect* a_this, float a_power, bool a_onlyHostile)
		{
			func(a_this, a_power, a_onlyHostile);

			const auto attacker = MiscUtils::GetValidReference<RE::Actor>(MiscUtils::ResolveHandle<RE::Actor>(a_this->GetCasterActor()));
			const auto target = MiscUtils::GetValidReference<RE::Actor>(a_this->GetVisualsTarget());
			const auto effect = a_this->GetBaseObject();
			const auto spell = a_this->spell;

			if (attacker && target && spell && effect && effect->IsHostile()) {
				if (const auto projectile = effect->data.projectileBase; projectile) {
					if (const auto mult = GetStandingSneakAttackMult(attacker, target, spell, projectile); mult > 1.0f) {
						a_this->magnitude *= mult; // Scales the effect magnitude
					}
				}
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	// Sneak flag, XP, notification, critical event, etc.
	struct SendHitEvent
	{
		static void thunk(RE::BSTEventSource<RE::TESHitEvent>& a_source, RE::TESHitEvent& a_event)
		{
			using HitFlag = RE::TESHitEvent::Flag;

			auto* aggressor = MiscUtils::GetValidReference<RE::Actor>(MiscUtils::ResolveHandle<RE::Actor>(a_event.cause));
			auto* target = MiscUtils::GetValidReference<RE::Actor>(MiscUtils::ResolveHandle<RE::Actor>(a_event.target));
			auto* source = RE::TESForm::LookupByID(a_event.source);
			auto* projectile = RE::TESForm::LookupByID<RE::BGSProjectile>(a_event.projectile);

			if (aggressor && target && source && projectile) {
				auto spell = source->As<RE::MagicItem>();
				if (!spell) {
					if (const auto weapon = source->As<RE::TESObjectWEAP>(); weapon && weapon->IsStaff()) {
						spell = weapon->formEnchanting;
					}
				}

				if (aggressor && target && spell && spell->IsHostile()) {
					if (const auto mult = GetStandingSneakAttackMult(aggressor, target, spell, projectile); mult > 1.0f) {
						a_event.flags.set(HitFlag::kSneakAttack);
						aggressor->UseSkill(RE::ActorValue::kSneak, config.sneakAttackSkillUsageMagic, spell);

						if (!config.hideNotifications) {
							const auto msgMain = MiscUtils::GetGameSetting<std::string>("sSuccessfulSneakAttackMain", "Sneak Attack Damage x");
							const auto msgEnd = MiscUtils::GetGameSetting<std::string>("sSuccessfulSneakAttackEnd", "");
							const auto sneakMessage = fmt::format("{}{:.1f}{}", msgMain, mult, msgEnd);
							RE::SendHUDMessage::ShowHUDMessage(sneakMessage.c_str(), config.hideNotificationSound ? nullptr : "UISneakAttack");
						}

						const RE::CriticalHit::Event event{ aggressor, nullptr, true };
						RE::CriticalHit::GetEventSource()->SendEvent(&event);
					}
				}
			}

			func(a_source, a_event);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	template <class T>
	inline bool WriteThunkCall(const char* a_name, std::uintptr_t a_src)
	{
		if (*reinterpret_cast<const std::uint8_t*>(a_src) != 0xE8) {
			logger::error("Magic Sneak Attacks compat: unexpected instruction at the '{}' hook site. Hook skipped.", a_name);
			return false;
		}

		SKSE::AllocTrampoline(1 << 8);
		auto& trampoline = SKSE::GetTrampoline();
		T::func = trampoline.write_call<5>(a_src, T::thunk);

		logger::info("Magic Sneak Attacks compat: hooked '{}'.", a_name);
		return true;
	}

	inline void Install()
	{
		if (installed) return;

		if (!SettingsIni::bGeneral_Enabled || !SettingsIni::bWeaponTypes_ApplyOnMagic) return;

		if (!IsMsaLoaded()) {
			logger::info("Magic Sneak Attacks not detected. Magic sneak attack compatibility is disabled.");
			return;
		}

		logger::info("Magic Sneak Attacks detected. Enabling magic sneak attacks while standing.");

		 // MagicTarget::CheckAddEffect
		REL::Relocation<std::uintptr_t> checkAddEffect{ REL::RelocationID(33763, 34547), REL::Relocate(0x4A3, 0x656, 0x427) };
		const bool hook1 = WriteThunkCall<AdjustActiveEffect>("Active Effect Adjust", checkAddEffect.address());

		// MagicTarget::AddTarget
		REL::Relocation<std::uintptr_t> addTarget{ REL::RelocationID(37832, 38786), REL::Relocate(0x1C3, 0x29B) };
		const bool hook2 = WriteThunkCall<SendHitEvent>("Magic Hit", addTarget.address());

		installed = hook1 || hook2;
	}
}
