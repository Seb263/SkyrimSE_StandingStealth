#pragma once

// ============================================================================
// Custom replacement for the ActiveEffect::AdjustForPerks() method.
//
// The goal here is to compute the theoretical real magnitude/damage that a hit would deal
// taking into account every multiplicative and additive layer that the engine normally applies,
// without mutating the ActiveEffect/HitData object itself.
// ============================================================================

#include "DataHandler.hpp"

#include "Utils/MiscUtils.hpp"

class PredictiveDamageUtils
{
public:

	static float GetSneakAttackMult(RE::Actor* a_attacker, RE::Actor* a_victim, RE::TESObjectWEAP* a_weapon)
	{
		if (!a_attacker) return 1.0f;

		float baseSneakMult = 2.0f;
		if (a_weapon) {
			switch (a_weapon->GetWeaponType()) {
				case RE::WEAPON_TYPE::kHandToHandMelee:
					baseSneakMult = MiscUtils::GetGameSetting<float>("fCombatSneakHandMult", 2.0f);
					break;
				case RE::WEAPON_TYPE::kOneHandSword:
					baseSneakMult = MiscUtils::GetGameSetting<float>("fCombatSneak1HSwordMult", 2.0f);
					break;
				case RE::WEAPON_TYPE::kOneHandDagger:
					baseSneakMult = MiscUtils::GetGameSetting<float>("fCombatSneak1HDaggerMult", 2.0f);
					break;
				case RE::WEAPON_TYPE::kOneHandAxe:
					baseSneakMult = MiscUtils::GetGameSetting<float>("fCombatSneak1HAxeMult", 2.0f);
					break;
				case RE::WEAPON_TYPE::kOneHandMace:
					baseSneakMult = MiscUtils::GetGameSetting<float>("fCombatSneak1HMaceMult", 2.0f);
					break;
				case RE::WEAPON_TYPE::kTwoHandSword:
					baseSneakMult = MiscUtils::GetGameSetting<float>("fCombatSneak2HSwordMult", 2.0f);
					break;
				case RE::WEAPON_TYPE::kTwoHandAxe:
					baseSneakMult = MiscUtils::GetGameSetting<float>("fCombatSneak2HAxeMult", 2.0f);
					break;
				case RE::WEAPON_TYPE::kBow:
					baseSneakMult = MiscUtils::GetGameSetting<float>("fCombatSneakBowMult", 2.0f);
					break;
				case RE::WEAPON_TYPE::kCrossbow:
					baseSneakMult = MiscUtils::GetGameSetting<float>("fCombatSneakCrossbowMult", 2.0f);
					break;
				case RE::WEAPON_TYPE::kStaff:
					baseSneakMult = MiscUtils::GetGameSetting<float>("fCombatSneakStaffMult", 2.0f);
					break;
				default:
					baseSneakMult = MiscUtils::GetGameSetting<float>("fCombatSneakHandMult", 2.0f);
					break;
			}
		} else {
			baseSneakMult = MiscUtils::GetGameSetting<float>("fCombatSneakHandMult", 2.0f);
		}

		void* args[3] = {
			static_cast<void*>(a_attacker),
			static_cast<void*>(a_weapon),
			static_cast<void*>(a_victim)
		};

		const float perkSneakMult = AccumulateEntryPointMult(
			a_attacker, RE::BGSEntryPoint::ENTRY_POINT::kModSneakAttackMult,
			std::span(args), baseSneakMult);

		const float finalSneakMult = baseSneakMult * perkSneakMult;

		TRACE("[GetSneakAttackMult] attacker={} weapon={} victim={} base={:.4f} perkMult={:.4f} -> {:.4f}",
			a_attacker->GetName(), a_weapon ? a_weapon->GetName() : "null",
			a_victim ? a_victim->GetName() : "null", baseSneakMult, perkSneakMult, finalSneakMult);

		return finalSneakMult;
	}

private:

	static float AccumulateEntryPointMult(RE::Actor* a_perkOwner, RE::BGSEntryPoint::ENTRY_POINT a_entryPoint,
		std::span<void*> a_condArgs, float a_currentMagnitude)
	{
		if (!a_perkOwner) return 1.0f;

		float additiveMult = 0.0f;
		float multiplicative = 1.0f;
		bool hasSetTo = false;
		float setValue = 0.0f;

		struct Visitor : RE::PerkEntryVisitor
		{
			std::span<void*> condArgs;
			float currentMagnitude;
			float& additiveMult;
			float& multiplicative;
			bool& hasSetTo;
			float& setValue;

			Visitor(std::span<void*> a_args, float a_curMag, float& a_add, float& a_mult, bool& a_hasSet, float& a_setVal)
				: condArgs(a_args), currentMagnitude(a_curMag),
				 additiveMult(a_add), multiplicative(a_mult), hasSetTo(a_hasSet), setValue(a_setVal)
			{}

			RE::BSContainer::ForEachResult Visit(RE::BGSPerkEntry* a_perkEntry) override
			{
				if (!a_perkEntry) return RE::BSContainer::ForEachResult::kContinue;

				auto* entryPoint = static_cast<RE::BGSEntryPointPerkEntry*>(a_perkEntry);
				const auto* perk = entryPoint->perk;
				if (!perk) return RE::BSContainer::ForEachResult::kContinue;

				if (!entryPoint->CheckConditionFilters(static_cast<int>(condArgs.size()), condArgs.data()))
					return RE::BSContainer::ForEachResult::kContinue;

				const auto* fnData = entryPoint->functionData;
				if (!fnData) return RE::BSContainer::ForEachResult::kContinue;

				const auto fnType = entryPoint->entryData.function.get();

				if (fnType == RE::BGSEntryPointPerkEntry::Function::kSetValue) {
					if (const auto* oneVal = skyrim_cast<const RE::BGSEntryPointFunctionDataOneValue*>(fnData)) {
						hasSetTo = true;
						setValue = oneVal->data;
						TRACE("[PerkAccum] perk={:08X} : SetValue={:.4f}", perk->formID, setValue);
					}
				} else if (fnType == RE::BGSEntryPointPerkEntry::Function::kAddValue) {
					if (const auto* oneVal = skyrim_cast<const RE::BGSEntryPointFunctionDataOneValue*>(fnData); oneVal && currentMagnitude > 0.0f) {
						additiveMult += oneVal->data / currentMagnitude;
						TRACE("[PerkAccum] perk={:08X} : AddValue={:.4f} (ratio={:.4f})", perk->formID, oneVal->data, oneVal->data / currentMagnitude);
					}
				} else if (fnType == RE::BGSEntryPointPerkEntry::Function::kMultiplyValue) {
					if (const auto* oneVal = skyrim_cast<const RE::BGSEntryPointFunctionDataOneValue*>(fnData)) {
						multiplicative *= oneVal->data;
						TRACE("[PerkAccum] perk={:08X} : MultiplyValue={:.4f}", perk->formID, oneVal->data);
					}
				}

				return RE::BSContainer::ForEachResult::kContinue;
			}
		};

		Visitor visitor(a_condArgs, a_currentMagnitude, additiveMult, multiplicative, hasSetTo, setValue);
		a_perkOwner->ForEachPerkEntry(a_entryPoint, visitor);

		if (hasSetTo) return a_currentMagnitude > 0.0f ? setValue / a_currentMagnitude : 1.0f;

		const float result = (1.0f + additiveMult) * multiplicative;
		return std::max(result, 0.0f);
	}
};
