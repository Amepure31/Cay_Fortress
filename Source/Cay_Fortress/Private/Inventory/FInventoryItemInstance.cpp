// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/InventoryComponent.h"

FWeaponItemStats UInventoryItemInstance::GetEffectiveWeaponStats() const
{
	if (!ItemData) return FWeaponItemStats();
	const FWeaponItemStats& Base = ItemData->ItemData.WeaponStats;
	const FWeaponStatOverrides& Ov = WeaponStatOverrides;
	FWeaponItemStats Result = Base;
	if (Ov.bOverrideDamage)          Result.Damage = Ov.Damage;
	if (Ov.bOverrideFireRate)        Result.FireRate = Ov.FireRate;
	if (Ov.bOverrideMagazineCapacity) Result.MagazineCapacity = Ov.MagazineCapacity;
	if (Ov.bOverrideHorizontalRecoil) Result.HorizontalRecoil = Ov.HorizontalRecoil;
	if (Ov.bOverrideVerticalRecoil)   Result.VerticalRecoil = Ov.VerticalRecoil;
	if (Ov.bOverrideRange)            Result.Range = Ov.Range;
	if (Ov.bOverrideReloadTime)       Result.ReloadTime = Ov.ReloadTime;
	if (Ov.bOverrideHipFireAccuracy)  Result.HipFireAccuracy = Ov.HipFireAccuracy;
	if (Ov.bOverrideADSAccuracy)      Result.ADSAccuracy = Ov.ADSAccuracy;
	if (Ov.bOverrideHandling)         Result.Handling = Ov.Handling;
	return Result;
}

void UInventoryItemInstance::EnsureContainerInventory(int32 InGridWidth, int32 InGridHeight)
{
	if (!ItemData || !ItemData->ItemData.ArmorStats.bIsContainer)
		return;

	if (!ContainerInventory)
	{
		ContainerInventory = NewObject<UInventoryComponent>(this);
		if (ContainerInventory)
		{
			ContainerInventory->bRejectContainerItems = true;
			ContainerInventory->InitializeGrid(FMath::Max(1, InGridWidth), FMath::Max(1, InGridHeight));
		}
	}
	else if (ContainerInventory->GridWidth != InGridWidth || ContainerInventory->GridHeight != InGridHeight)
	{
		ContainerInventory->InitializeGrid(FMath::Max(1, InGridWidth), FMath::Max(1, InGridHeight));
	}
}
