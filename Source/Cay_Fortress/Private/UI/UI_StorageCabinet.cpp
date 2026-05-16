#include "UI/UI_StorageCabinet.h"
#include "BaseBuilding/StorageCabinetActor.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/InventoryComponent.h"
#include "Alex_PlayerCharacter.h"
#include "Alex_PlayerController.h"
#include "EngineUtils.h"

void UUI_StorageCabinet::BindStorageCabinet(AStorageCabinetActor* InCabinet)
{
	BoundCabinet = InCabinet;
	if (InCabinet)
	{
		BindContainerInventory(InCabinet->GetInventoryComponent());
		UpdateInventory();
	}
	BP_OnCabinetBound();
}

bool UUI_StorageCabinet::TryUpgradeCabinet()
{
	if (!BoundCabinet) return false;
	AAlex_PlayerController* PC = GetOwningPlayer<AAlex_PlayerController>();
	if (!PC || !PC->GetPawn()) return false;
	if (BoundCabinet->UpgradeGrid(PC->GetPawn()))
	{
		UpdateInventory();
		PC->RefreshPlayerInventoryUI();
		return true;
	}
	return false;
}

bool UUI_StorageCabinet::CanUpgradeCabinet() const
{
	if (!BoundCabinet) return false;
	AAlex_PlayerController* PC = GetOwningPlayer<AAlex_PlayerController>();
	if (!PC || !PC->GetPawn()) return false;

	AAlex_PlayerCharacter* Player = Cast<AAlex_PlayerCharacter>(PC->GetPawn());
	if (!Player) return false;

	TArray<UInventoryComponent*> Sources;
	if (UInventoryComponent* PlayerInv = Player->FindComponentByClass<UInventoryComponent>())
		Sources.Add(PlayerInv);
	if (UInventoryComponent* CabInv = BoundCabinet->GetInventoryComponent())
		Sources.Add(CabInv);
	for (TActorIterator<AStorageCabinetActor> It(PC->GetWorld()); It; ++It)
	{
		if (*It == BoundCabinet) continue;
		if (UInventoryComponent* CabInv = (*It)->GetInventoryComponent())
			Sources.Add(CabInv);
	}

	return BoundCabinet->CanUpgrade(Sources);
}

int32 UUI_StorageCabinet::GetUpgradeLevel() const { return BoundCabinet ? BoundCabinet->GetUpgradeLevel() : 0; }
int32 UUI_StorageCabinet::GetMaxUpgradeLevel() const { return BoundCabinet ? BoundCabinet->GetMaxUpgradeLevel() : 0; }

void UUI_StorageCabinet::RequestClose()
{
	AAlex_PlayerController* PC = GetOwningPlayer<AAlex_PlayerController>();
	if (PC) PC->CloseStorageCabinetUI(true);
}
