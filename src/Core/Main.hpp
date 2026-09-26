#pragma once

#include "DataHandler.hpp"
#include "SettingsIni.hpp"

#include "Utils/PredictiveDamageUtils.hpp"

namespace ModCore
{
	class Main
	{
	public:
		static bool IsStealthAttack(RE::Actor* victim, RE::Actor* attacker, RE::TESObjectWEAP* weapon) noexcept
		{
			if (!SettingsIni::bGeneral_Enabled || !victim || !attacker) return false;
			if (victim->IsDead() || !victim->Is3DLoaded()) return false;
			if (victim->RequestDetectionLevel(attacker) > 0) return false;
			if (!CanApplyToActor(attacker)) return false;

			const auto weaponType = weapon ? weapon->GetWeaponType() : RE::WeaponTypes::kHandToHandMelee;
			if (!CanApplyToWeapon(weaponType)) return false;

			return true;
		}

		static void Process(RE::Actor* victim, RE::Actor* attacker, RE::HitData& hitData)
		{
			if (hitData.flags.any(RE::HitData::Flag::kSneakAttack)) return;
			if (!IsStealthAttack(victim, attacker, hitData.weapon)) return;

			hitData.flags.set(RE::HitData::Flag::kSneakAttack);
			hitData.sneakAttackBonus = MiscUtils::GetGameSetting<float>("fCombatSneakAttackBonusMult", 100.0f);

			const float sneakDamageMult = PredictiveDamageUtils::GetSneakAttackMult(attacker, victim, hitData.weapon);
			hitData.bonusHealthDamageMult = sneakDamageMult;
			hitData.physicalDamage *= sneakDamageMult;
			hitData.targetedLimbDamage *= sneakDamageMult;
			hitData.totalDamage *= sneakDamageMult;
			hitData.resistedPhysicalDamage *= sneakDamageMult;
			hitData.reflectedDamage *= sneakDamageMult;
		}

	private:
		static bool CanApplyToActor(RE::Actor* actor)
		{
			if (actor->IsPlayerRef()) return SettingsIni::bActors_ApplyOnPlayer;
			if (actor->IsPlayerTeammate()) return SettingsIni::bActors_ApplyOnFollowers;
			return SettingsIni::bActors_ApplyOnNPCs;
		}

		static bool CanApplyToWeapon(RE::WEAPON_TYPE weaponType)
		{
			switch (weaponType) {
				case RE::WeaponTypes::kHandToHandMelee:
					return SettingsIni::bWeaponTypes_ApplyOnHandToHand;
				case RE::WeaponTypes::kOneHandSword:
				case RE::WeaponTypes::kOneHandAxe:
				case RE::WeaponTypes::kOneHandMace:
					return SettingsIni::bWeaponTypes_ApplyOnOneHanded;
				case RE::WeaponTypes::kTwoHandSword:
				case RE::WeaponTypes::kTwoHandAxe:
					return SettingsIni::bWeaponTypes_ApplyOnTwoHanded;
				case RE::WeaponTypes::kOneHandDagger:
					return SettingsIni::bWeaponTypes_ApplyOnDaggers;
				case RE::WeaponTypes::kBow:
				case RE::WeaponTypes::kCrossbow:
					return SettingsIni::bWeaponTypes_ApplyOnRanged;
				default:
					return false;
			}
		}
	};
};
