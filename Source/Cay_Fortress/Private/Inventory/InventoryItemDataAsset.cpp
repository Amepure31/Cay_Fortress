// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/InventoryItemDataAsset.h"

EAmmoType GetExpectedAmmoTypeForWeaponClass(const EWeaponClass WeaponClass)
{
	switch (WeaponClass)
	{
	case EWeaponClass::Pistol:
		return EAmmoType::Pistol;
	case EWeaponClass::SMG:
	case EWeaponClass::Rifle:
	case EWeaponClass::LMG:
		return EAmmoType::Rifle;
	case EWeaponClass::Shotgun:
		return EAmmoType::Shotgun;
	case EWeaponClass::Sniper:
		return EAmmoType::Sniper;
	default:
		return EAmmoType::Other;
	}
}
