#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Inventory/InventoryItemType.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/FInventoryGridCell.h"
#include "Inventory/FItemShapeMask.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Modules/ModuleManager.h"
#include "Engine/Blueprint.h"

namespace
{
static bool IsAmmoLikeForDataAsset(const UInventoryItemDataAsset* DA)
{
	if (!DA) return false;
	const FInventoryItemData& R = DA->ItemData;
	return R.ItemType == EInventoryItemType::Ammo || R.bCountAsAmmoForReserve;
}

static UInventoryItemDataAsset* ResolveInventoryItemDAFromLoadedObject(UObject* Obj)
{
	if (!Obj) return nullptr;
	if (UInventoryItemDataAsset* Direct = Cast<UInventoryItemDataAsset>(Obj)) return Direct;
	if (UBlueprint* BP = Cast<UBlueprint>(Obj))
	{
		if (UClass* Gen = BP->GeneratedClass)
			if (Gen->IsChildOf(UInventoryItemDataAsset::StaticClass()))
				return Cast<UInventoryItemDataAsset>(Gen->GetDefaultObject());
		return nullptr;
	}
	if (UClass* Cls = Cast<UClass>(Obj))
	{
		if (Cls->IsChildOf(UInventoryItemDataAsset::StaticClass()) && !Cls->HasAnyClassFlags(CLASS_Abstract))
			return Cast<UInventoryItemDataAsset>(Cls->GetDefaultObject());
	}
	return nullptr;
}

static UInventoryItemDataAsset* ResolveInventoryItemDAFromAssetData(const FAssetData& AD)
{
	return ResolveInventoryItemDAFromLoadedObject(AD.GetAsset());
}

static int32 SumStacksForDataAsset(const TArray<TObjectPtr<UInventoryItemInstance>>& ItemList, const UInventoryItemDataAsset* DA)
{
	int32 S = 0;
	for (const TObjectPtr<UInventoryItemInstance>& It : ItemList)
		if (It && It->ItemData == DA) S += FMath::Max(0, It->StackSize);
	return S;
}

static void MergeDiscoveredAmmoIntoMap(TMap<EAmmoType, TObjectPtr<UInventoryItemDataAsset>>& DefaultAmmoItemByType, UInventoryItemDataAsset* DA)
{
	if (!IsAmmoLikeForDataAsset(DA)) return;
	const EAmmoType AT = DA->ItemData.AmmoStats.AmmoType;
	if (TObjectPtr<UInventoryItemDataAsset>* Slot = DefaultAmmoItemByType.Find(AT))
	{
		if (!Slot->Get()) *Slot = DA;
	}
	else DefaultAmmoItemByType.Add(AT, DA);
}

static FFItemShapeMask MakeFullMask(int32 Width, int32 Height)
{
	FFItemShapeMask Mask;
	Mask.Width = FMath::Max(1, Width);
	Mask.Height = FMath::Max(1, Height);
	Mask.ShapeMaskData.Init(1, Mask.Width * Mask.Height);
	return Mask;
}

static FFItemShapeMask BuildMaskFromItemData(const FInventoryItemData& Data)
{
	const int32 SafeWidth = FMath::Max(1, Data.Width);
	const int32 SafeHeight = FMath::Max(1, Data.Height);
	if (Data.ShapeMaskData.Num() < SafeWidth * SafeHeight) return MakeFullMask(SafeWidth, SafeHeight);
	FFItemShapeMask Mask;
	Mask.Width = SafeWidth;
	Mask.Height = SafeHeight;
	Mask.ShapeMaskData = Data.ShapeMaskData;
	return Mask;
}

static FFItemShapeMask RotateMaskClockwise(const FFItemShapeMask& InMask)
{
	const int32 OldWidth = FMath::Max(1, InMask.Width);
	const int32 OldHeight = FMath::Max(1, InMask.Height);
	const int32 NewWidth = OldHeight;
	const int32 NewHeight = OldWidth;
	FFItemShapeMask Rotated;
	Rotated.Width = NewWidth;
	Rotated.Height = NewHeight;
	Rotated.ShapeMaskData.Init(0, NewWidth * NewHeight);
	for (int32 Y = 0; Y < OldHeight; ++Y)
		for (int32 X = 0; X < OldWidth; ++X)
			if (InMask.IsOccupied(X, Y))
			{
				const int32 RotatedX = OldHeight - 1 - Y;
				const int32 RotatedY = X;
				Rotated.ShapeMaskData[RotatedY * NewWidth + RotatedX] = 1;
			}
	return Rotated;
}

static bool IsFullRectangularMask(const FFItemShapeMask& Mask)
{
	const int32 Width = FMath::Max(1, Mask.Width);
	const int32 Height = FMath::Max(1, Mask.Height);
	for (int32 Y = 0; Y < Height; ++Y)
		for (int32 X = 0; X < Width; ++X)
			if (!Mask.IsOccupied(X, Y)) return false;
	return true;
}

static int32 GetRotationAttemptCount(const FFItemShapeMask& Mask)
{
	const int32 Width = FMath::Max(1, Mask.Width);
	const int32 Height = FMath::Max(1, Mask.Height);
	const int32 OccupiedCount = Mask.GetOccupiedCellCount();
	if (Width == 1 && Height == 1 && OccupiedCount <= 1) return 1;
	return IsFullRectangularMask(Mask) ? 2 : 4;
}
}

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	GridWidth = 6;
	GridHeight = 10;
	bIsInventoryOpen = false;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GridWidth <= 0) GridWidth = 6;
	if (GridHeight <= 0) GridHeight = 10;
	InitializeGrid(GridWidth, GridHeight);
	TryAutoDiscoverDefaultAmmoItemsIfNeeded();
}

void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UInventoryComponent::InitializeGrid(int32 InWidth, int32 InHeight)
{
	GridWidth = InWidth;
	GridHeight = InHeight;
	Grid.Empty();
	for (int32 Y = 0; Y < GridHeight; Y++)
		for (int32 X = 0; X < GridWidth; X++)
			Grid.Add(FFInventoryGridCell());
}

void UInventoryComponent::Clear()
{
	for (int32 Y = 0; Y < GridHeight; Y++)
		for (int32 X = 0; X < GridWidth; X++)
		{
			int32 Index = GetGridIndex(X, Y);
			if (Index >= 0 && Index < Grid.Num()) Grid[Index].Clear();
		}
	Items.Empty();
	NotifyInventoryChanged();
}

void UInventoryComponent::OpenInventory() { if (!bIsInventoryOpen) { bIsInventoryOpen = true; OnInventoryToggled.Broadcast(bIsInventoryOpen); } }
void UInventoryComponent::CloseInventory() { if (bIsInventoryOpen) { bIsInventoryOpen = false; OnInventoryToggled.Broadcast(bIsInventoryOpen); } }
void UInventoryComponent::ToggleInventory() { bIsInventoryOpen = !bIsInventoryOpen; OnInventoryToggled.Broadcast(bIsInventoryOpen); }

UInventoryItemInstance* UInventoryComponent::TryAddItemReturningInstance(class UInventoryItemDataAsset* ItemData, int32 StackSize)
{
	if (!ItemData) return nullptr;
	if (bRejectContainerItems && ItemData->ItemData.ArmorStats.bIsContainer) return nullptr;
	const FInventoryItemData& Data = ItemData->ItemData;

	if (Data.bCanStack)
	{
		for (UInventoryItemInstance* ExistingItem : Items)
		{
			if (ExistingItem && ExistingItem->ItemData == ItemData && ExistingItem->StackSize < Data.MaxStackSize)
			{
				int32 Space = Data.MaxStackSize - ExistingItem->StackSize;
				int32 AddAmount = FMath::Min(StackSize, Space);
				ExistingItem->StackSize += AddAmount;
				NotifyInventoryChanged();
				return ExistingItem;
			}
		}
	}

	FFItemShapeMask TryMask = BuildMaskFromItemData(Data);
	int32 TryWidth = FMath::Max(1, TryMask.Width);
	int32 TryHeight = FMath::Max(1, TryMask.Height);
	const int32 RotationAttempts = GetRotationAttemptCount(TryMask);

	for (int32 RotationStep = 0; RotationStep < RotationAttempts; ++RotationStep)
	{
		int32 SlotX = -1, SlotY = -1;
		if (FindSpaceForItem(TryWidth, TryHeight, TryMask, SlotX, SlotY))
			return AddItemWithShapeAtPosition(ItemData, SlotX, SlotY, StackSize, TryWidth, TryHeight, TryMask, RotationStep);
		if (RotationStep < RotationAttempts - 1)
		{
			TryMask = RotateMaskClockwise(TryMask);
			TryWidth = FMath::Max(1, TryMask.Width);
			TryHeight = FMath::Max(1, TryMask.Height);
		}
	}
	return nullptr;
}

bool UInventoryComponent::AddItem(class UInventoryItemDataAsset* ItemData, int32 StackSize)
{
	return TryAddItemReturningInstance(ItemData, StackSize) != nullptr;
}

bool UInventoryComponent::AddItemAtPosition(class UInventoryItemDataAsset* ItemData, int32 SlotX, int32 SlotY, int32 StackSize)
{
	if (!ItemData) return false;
	const FInventoryItemData& Data = ItemData->ItemData;
	FFItemShapeMask BaseMask = BuildMaskFromItemData(Data);
	return AddItemWithShapeAtPosition(ItemData, SlotX, SlotY, StackSize, BaseMask.Width, BaseMask.Height, BaseMask, 0) != nullptr;
}

class UInventoryItemInstance* UInventoryComponent::AddItemWithShapeAtPosition(
	class UInventoryItemDataAsset* ItemData, int32 SlotX, int32 SlotY, int32 StackSize,
	int32 Width, int32 Height, const FFItemShapeMask& ShapeMask, int32 RotationQuarterTurns,
	int32 Durability, int32 MaxDurability, bool bIsBound, FDateTime BindTime)
{
	if (!ItemData) return nullptr;
	if (bRejectContainerItems && ItemData->ItemData.ArmorStats.bIsContainer) return nullptr;
	const int32 SafeWidth = FMath::Max(1, Width);
	const int32 SafeHeight = FMath::Max(1, Height);
	if (!IsSpaceAvailable(SafeWidth, SafeHeight, ShapeMask, SlotX, SlotY)) return nullptr;

	UInventoryItemInstance* NewItem = NewObject<UInventoryItemInstance>(this);
	if (!NewItem) return nullptr;

	NewItem->SetItemData(ItemData);
	const FInventoryItemData& ItemRow = ItemData->ItemData;
	int32 FinalStackSize = FMath::Max(1, StackSize);
	if (ItemRow.bCanStack) FinalStackSize = FMath::Clamp(FinalStackSize, 1, FMath::Max(1, ItemRow.MaxStackSize));
	NewItem->StackSize = FinalStackSize;
	NewItem->Width = SafeWidth;
	NewItem->Height = SafeHeight;
	NewItem->ShapeMask = ShapeMask;
	NewItem->RotationQuarterTurns = ((RotationQuarterTurns % 4) + 4) % 4;
	NewItem->SlotX = SlotX;
	NewItem->SlotY = SlotY;
	NewItem->bIsBound = bIsBound;
	NewItem->BindTime = BindTime;

	if (MaxDurability >= 0) NewItem->MaxDurability = MaxDurability;
	if (Durability >= 0) NewItem->Durability = Durability;

	Items.Add(NewItem);
	OccupyGrid(NewItem, SlotX, SlotY);
	NotifyInventoryChanged();
	return NewItem;
}

bool UInventoryComponent::DetachItemInstance(class UInventoryItemInstance* ItemInstance)
{
	if (!ItemInstance) return false;
	if (!Items.Contains(ItemInstance))
	{
		return false;
	}
	ReleaseGrid(ItemInstance);
	Items.Remove(ItemInstance);
	ItemInstance->SlotX = -1;
	ItemInstance->SlotY = -1;
	NotifyInventoryChanged();
	return true;
}

bool UInventoryComponent::RemoveItem(class UInventoryItemInstance* ItemInstance)
{
	if (!ItemInstance) return false;
	ReleaseGrid(ItemInstance);
	Items.Remove(ItemInstance);
	ItemInstance->MarkAsGarbage();
	NotifyInventoryChanged();
	return true;
}

class UInventoryItemInstance* UInventoryComponent::RemoveItemAtPosition(int32 SlotX, int32 SlotY)
{
	UInventoryItemInstance* Item = GetItemAtPosition(SlotX, SlotY);
	if (Item) RemoveItem(Item);
	return Item;
}

bool UInventoryComponent::MoveItem(class UInventoryItemInstance* ItemInstance, int32 NewSlotX, int32 NewSlotY)
{
	if (!ItemInstance) return false;
	ReleaseGrid(ItemInstance);
	if (!IsSpaceAvailable(ItemInstance->Width, ItemInstance->Height, ItemInstance->ShapeMask, NewSlotX, NewSlotY, ItemInstance))
	{
		OccupyGrid(ItemInstance, ItemInstance->SlotX, ItemInstance->SlotY);
		return false;
	}
	ItemInstance->SlotX = NewSlotX;
	ItemInstance->SlotY = NewSlotY;
	OccupyGrid(ItemInstance, NewSlotX, NewSlotY);
	NotifyInventoryChanged();
	return true;
}

bool UInventoryComponent::MoveItemWithShape(class UInventoryItemInstance* ItemInstance, int32 NewSlotX, int32 NewSlotY,
	int32 NewWidth, int32 NewHeight, const FFItemShapeMask& NewShapeMask, int32 NewRotationQuarterTurns)
{
	if (!ItemInstance) return false;
	const int32 OldSlotX = ItemInstance->SlotX;
	const int32 OldSlotY = ItemInstance->SlotY;
	ReleaseGrid(ItemInstance);
	const int32 SafeWidth = FMath::Max(1, NewWidth);
	const int32 SafeHeight = FMath::Max(1, NewHeight);
	if (!IsSpaceAvailable(SafeWidth, SafeHeight, NewShapeMask, NewSlotX, NewSlotY, ItemInstance))
	{
		OccupyGrid(ItemInstance, OldSlotX, OldSlotY);
		return false;
	}
	ItemInstance->Width = SafeWidth;
	ItemInstance->Height = SafeHeight;
	ItemInstance->ShapeMask = NewShapeMask;
	ItemInstance->RotationQuarterTurns = ((NewRotationQuarterTurns % 4) + 4) % 4;
	ItemInstance->SlotX = NewSlotX;
	ItemInstance->SlotY = NewSlotY;
	OccupyGrid(ItemInstance, NewSlotX, NewSlotY);
	NotifyInventoryChanged();
	return true;
}

bool UInventoryComponent::IsSpaceAvailable(int32 Width, int32 Height, const FFItemShapeMask& ShapeMask, int32 SlotX, int32 SlotY, class UInventoryItemInstance* IgnoreItem) const
{
	for (int32 Y = 0; Y < Height; Y++)
	{
		for (int32 X = 0; X < Width; X++)
		{
			int32 GridX = SlotX + X;
			int32 GridY = SlotY + Y;
			if (!IsValidPosition(GridX, GridY)) return false;
			bool bShouldOccupy = true;
			if (ShapeMask.Width > 0 && ShapeMask.Height > 0) bShouldOccupy = ShapeMask.IsOccupied(X, Y);
			if (bShouldOccupy)
			{
				int32 Index = GetGridIndex(GridX, GridY);
				if (Index >= 0 && Index < Grid.Num())
					if (Grid[Index].bOccupied && Grid[Index].ItemInstance != IgnoreItem) return false;
			}
		}
	}
	return true;
}

bool UInventoryComponent::FindSpaceForItem(int32 Width, int32 Height, const FFItemShapeMask& ShapeMask, int32& OutSlotX, int32& OutSlotY) const
{
	for (int32 Y = 0; Y <= GridHeight - Height; Y++)
		for (int32 X = 0; X <= GridWidth - Width; X++)
			if (IsSpaceAvailable(Width, Height, ShapeMask, X, Y)) { OutSlotX = X; OutSlotY = Y; return true; }
	return false;
}

class UInventoryItemInstance* UInventoryComponent::GetItemAtPosition(int32 SlotX, int32 SlotY) const
{
	if (!IsValidPosition(SlotX, SlotY)) return nullptr;
	int32 Index = GetGridIndex(SlotX, SlotY);
	if (Index >= 0 && Index < Grid.Num()) return Grid[Index].ItemInstance;
	return nullptr;
}

bool UInventoryComponent::IsValidPosition(int32 SlotX, int32 SlotY) const { return SlotX >= 0 && SlotX < GridWidth && SlotY >= 0 && SlotY < GridHeight; }

void UInventoryComponent::OccupyGrid(class UInventoryItemInstance* ItemInstance, int32 SlotX, int32 SlotY)
{
	if (!ItemInstance) return;
	int32 Width = ItemInstance->Width;
	int32 Height = ItemInstance->Height;
	for (int32 Y = 0; Y < Height; Y++)
		for (int32 X = 0; X < Width; X++)
		{
			bool bShouldOccupy = true;
			if (ItemInstance->ShapeMask.Width > 0 && ItemInstance->ShapeMask.Height > 0)
				bShouldOccupy = ItemInstance->ShapeMask.IsOccupied(X, Y);
			if (bShouldOccupy)
			{
				int32 GridX = SlotX + X, GridY = SlotY + Y;
				int32 Index = GetGridIndex(GridX, GridY);
				if (Index >= 0 && Index < Grid.Num())
					Grid[Index].SetOccupied(ItemInstance, Width, Height, SlotX, SlotY);
			}
		}
}

void UInventoryComponent::ReleaseGrid(class UInventoryItemInstance* ItemInstance)
{
	if (!ItemInstance) return;
	int32 Width = ItemInstance->Width, Height = ItemInstance->Height;
	int32 SlotX = ItemInstance->SlotX, SlotY = ItemInstance->SlotY;
	for (int32 Y = 0; Y < Height; Y++)
		for (int32 X = 0; X < Width; X++)
		{
			bool bShouldRelease = true;
			if (ItemInstance->ShapeMask.Width > 0 && ItemInstance->ShapeMask.Height > 0)
				bShouldRelease = ItemInstance->ShapeMask.IsOccupied(X, Y);
			if (bShouldRelease)
			{
				int32 GridX = SlotX + X, GridY = SlotY + Y;
				int32 Index = GetGridIndex(GridX, GridY);
				if (Index >= 0 && Index < Grid.Num() && Grid[Index].ItemInstance == ItemInstance)
					Grid[Index].Clear();
			}
		}
}

int32 UInventoryComponent::GetGridIndex(int32 X, int32 Y) const { return Y * GridWidth + X; }

EAmmoType UInventoryComponent::GetReserveAmmoTypeForWeapon(const UInventoryItemDataAsset* WeaponDA) const
{
	if (const UInventoryItemDataAsset* AmmoDA = ResolveCompanionAmmoItemForWeapon(WeaponDA))
		return AmmoDA->ItemData.AmmoStats.AmmoType;
	if (WeaponDA && WeaponDA->ItemData.ItemType == EInventoryItemType::Weapon)
		return GetExpectedAmmoTypeForWeaponClass(WeaponDA->ItemData.WeaponStats.WeaponClass);
	return EAmmoType::Other;
}

int32 UInventoryComponent::GetTotalReserveRoundsForWeaponInInventory(const UInventoryItemDataAsset* WeaponDA) const
{
	if (!WeaponDA || WeaponDA->ItemData.ItemType != EInventoryItemType::Weapon) return 0;
	const EAmmoType MatchTy = GetReserveAmmoTypeForWeapon(WeaponDA);
	int32 Sum = 0;
	for (UInventoryItemInstance* Item : Items)
	{
		if (!Item || !Item->ItemData) continue;
		if (!IsAmmoLikeForDataAsset(Item->ItemData)) continue;
		if (Item->ItemData->ItemData.AmmoStats.AmmoType == MatchTy) Sum += FMath::Max(0, Item->StackSize);
	}
	return Sum;
}

int32 UInventoryComponent::GetTotalAmmoCountByAmmoType(const EAmmoType Type) const
{
	int32 Sum = 0;
	for (UInventoryItemInstance* Item : Items)
	{
		if (!Item || !Item->ItemData) continue;
		if (!IsAmmoLikeForDataAsset(Item->ItemData)) continue;
		if (Item->ItemData->ItemData.AmmoStats.AmmoType == Type) Sum += FMath::Max(0, Item->StackSize);
	}
	return Sum;
}

int32 UInventoryComponent::ConsumeAmmoFromInventoryByType(const EAmmoType Type, int32 Amount)
{
	if (Amount <= 0) return 0;
	int32 RemainingToTake = Amount;
	TArray<UInventoryItemInstance*> PendingRemove;
	for (UInventoryItemInstance* Item : Items)
	{
		if (!Item || !Item->ItemData) continue;
		if (!IsAmmoLikeForDataAsset(Item->ItemData)) continue;
		if (Item->ItemData->ItemData.AmmoStats.AmmoType != Type) continue;
		if (RemainingToTake <= 0) break;
		const int32 Take = FMath::Min(RemainingToTake, FMath::Max(0, Item->StackSize));
		Item->StackSize -= Take;
		RemainingToTake -= Take;
		if (Item->StackSize <= 0) PendingRemove.Add(Item);
	}
	const int32 Taken = Amount - RemainingToTake;
	for (UInventoryItemInstance* Item : PendingRemove)
	{
		ReleaseGrid(Item);
		Items.Remove(Item);
		Item->MarkAsGarbage();
	}
	if (Taken > 0) NotifyInventoryChanged();
	return Taken;
}

float UInventoryComponent::GetTotalCarriedWeight() const
{
	float Total = 0.0f;
	for (const TObjectPtr<UInventoryItemInstance>& Item : Items)
	{
		if (Item && Item->ItemData)
			Total += Item->ItemData->ItemData.Weight * FMath::Max(0, Item->StackSize);
	}
	return Total;
}

void UInventoryComponent::NotifyInventoryChanged() { OnInventoryChanged.Broadcast(); }

void UInventoryComponent::TryAutoDiscoverDefaultAmmoItemsIfNeeded()
{
	if (bDidAutoDiscoverDefaultAmmo) return;
	if (!FModuleManager::Get().IsModuleLoaded("AssetRegistry")) return;
	bDidAutoDiscoverDefaultAmmo = true;

	IAssetRegistry& Reg = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	TArray<FAssetData> NativeAssetList;
	Reg.GetAssetsByClass(UInventoryItemDataAsset::StaticClass()->GetClassPathName(), NativeAssetList, true);

	TArray<FAssetData> BlueprintAssetList;
	{
		FARFilter BpFilter;
		BpFilter.bRecursivePaths = true;
		BpFilter.PackagePaths.Add(FName(TEXT("/Game/Inventory/InventoryItemDataAsset/Ammo")));
		BpFilter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
		Reg.GetAssets(BpFilter, BlueprintAssetList);
	}

	TSet<FString> SeenAssetPaths;
	auto ProcessAssetList = [&](const TArray<FAssetData>& List)
	{
		for (const FAssetData& AD : List)
		{
			if (!AD.IsValid() || !AD.PackageName.ToString().StartsWith(TEXT("/Game"))) continue;
			const FString PathKey = AD.GetSoftObjectPath().ToString();
			if (SeenAssetPaths.Contains(PathKey)) continue;
			UInventoryItemDataAsset* const DA = ResolveInventoryItemDAFromAssetData(AD);
			if (!DA || !IsAmmoLikeForDataAsset(DA)) continue;
			SeenAssetPaths.Add(PathKey);
			MergeDiscoveredAmmoIntoMap(DefaultAmmoItemByType, DA);
		}
	};
	ProcessAssetList(NativeAssetList);
	ProcessAssetList(BlueprintAssetList);
}

UInventoryItemDataAsset* UInventoryComponent::ResolveCompanionAmmoItemForWeapon(const UInventoryItemDataAsset* WeaponDA) const
{
	const_cast<UInventoryComponent*>(this)->TryAutoDiscoverDefaultAmmoItemsIfNeeded();
	if (!WeaponDA) return nullptr;
	const FInventoryItemData& D = WeaponDA->ItemData;
	if (D.ItemType != EInventoryItemType::Weapon) return nullptr;
	if (D.CompanionAmmoItemOverride)
	{
		if (IsAmmoLikeForDataAsset(D.CompanionAmmoItemOverride)) return D.CompanionAmmoItemOverride;
		return nullptr;
	}
	const EAmmoType ExpectedType = GetExpectedAmmoTypeForWeaponClass(D.WeaponStats.WeaponClass);
	if (const TObjectPtr<UInventoryItemDataAsset>* Found = DefaultAmmoItemByType.Find(ExpectedType))
		return Found->Get();
	return nullptr;
}

bool UInventoryComponent::TryPlaceSingleNewStackIgnoringMerge(UInventoryItemDataAsset* ItemData, const int32 StackSize)
{
	if (!ItemData || StackSize <= 0) return false;
	const FInventoryItemData& Data = ItemData->ItemData;
	FFItemShapeMask TryMask = BuildMaskFromItemData(Data);
	int32 TryWidth = FMath::Max(1, TryMask.Width);
	int32 TryHeight = FMath::Max(1, TryMask.Height);
	const int32 RotationAttempts = GetRotationAttemptCount(TryMask);
	for (int32 RotationStep = 0; RotationStep < RotationAttempts; ++RotationStep)
	{
		int32 SlotX = -1, SlotY = -1;
		if (FindSpaceForItem(TryWidth, TryHeight, TryMask, SlotX, SlotY))
			return AddItemWithShapeAtPosition(ItemData, SlotX, SlotY, StackSize, TryWidth, TryHeight, TryMask, RotationStep) != nullptr;
		if (RotationStep < RotationAttempts - 1)
		{
			TryMask = RotateMaskClockwise(TryMask);
			TryWidth = FMath::Max(1, TryMask.Width);
			TryHeight = FMath::Max(1, TryMask.Height);
		}
	}
	return false;
}

int32 UInventoryComponent::GrantLooseAmmoAsSingleStacks(UInventoryItemDataAsset* AmmoDA, int32 RoundCount, bool bAllowStackMerge)
{
	if (!AmmoDA || RoundCount <= 0) return 0;
	const FInventoryItemData& Data = AmmoDA->ItemData;
	const int32 Cap = FMath::Min(RoundCount, 100000);
	if (Data.bCanStack && bAllowStackMerge)
	{
		int32 TotalPlaced = 0;
		int32 Remaining = Cap;
		while (Remaining > 0)
		{
			const int32 Before = SumStacksForDataAsset(Items, AmmoDA);
			if (!AddItem(AmmoDA, Remaining)) break;
			const int32 After = SumStacksForDataAsset(Items, AmmoDA);
			const int32 Delta = After - Before;
			if (Delta <= 0) break;
			TotalPlaced += Delta;
			Remaining -= Delta;
		}
		return TotalPlaced;
	}
	int32 Placed = 0;
	for (int32 i = 0; i < Cap; ++i)
	{
		if (!TryPlaceSingleNewStackIgnoringMerge(AmmoDA, 1)) break;
		Placed++;
	}
	return Placed;
}

void UInventoryComponent::TryGrantCompanionAmmoForWeaponLoose(UInventoryItemDataAsset* WeaponItemData)
{
	if (!WeaponItemData) return;
	const FInventoryItemData& D = WeaponItemData->ItemData;
	if (D.ItemType != EInventoryItemType::Weapon) return;
	if (!D.bGrantCompanionAmmoWhenAddedToInventory) return;
	UInventoryItemDataAsset* AmmoDA = ResolveCompanionAmmoItemForWeapon(WeaponItemData);
	if (!AmmoDA) return;
	const int32 Mag = FMath::Max(1, D.WeaponStats.MagazineCapacity);
	GrantLooseAmmoAsSingleStacks(AmmoDA, Mag);
}

bool UInventoryComponent::AddItemFromInventorySpawnerButton(UInventoryItemDataAsset* ItemData, const int32 StackSize)
{
	if (!ItemData) return false;
	UInventoryItemInstance* const Added = TryAddItemReturningInstance(ItemData, StackSize);
	if (!Added) return false;
	if (ItemData->ItemData.ItemType == EInventoryItemType::Weapon)
	{
		UInventoryItemDataAsset* AmmoDA = ResolveCompanionAmmoItemForWeapon(ItemData);
		const int32 MagCap = FMath::Max(1, ItemData->ItemData.WeaponStats.MagazineCapacity);
		if (!AmmoDA)
		{
			Added->WeaponMagazineAmmo = MagCap;
		}
		else
		{
			Added->WeaponMagazineAmmo = 0;
			GrantLooseAmmoAsSingleStacks(AmmoDA, MagCap);
		}
		NotifyInventoryChanged();
	}
	return true;
}

bool UInventoryComponent::TryUnloadWeaponMagazineToLooseAmmo(UInventoryItemInstance* WeaponInstance)
{
	if (!WeaponInstance || !WeaponInstance->ItemData) return false;
	if (WeaponInstance->ItemData->ItemData.ItemType != EInventoryItemType::Weapon) return false;
	int32 Count = FMath::Max(0, WeaponInstance->WeaponMagazineAmmo);
	if (Count <= 0) return false;
	UInventoryItemDataAsset* AmmoDA = ResolveCompanionAmmoItemForWeapon(WeaponInstance->ItemData);
	if (!AmmoDA) return false;
	const int32 Unloaded = GrantLooseAmmoAsSingleStacks(AmmoDA, Count, true);
	if (Unloaded <= 0) return false;
	WeaponInstance->WeaponMagazineAmmo = Count - Unloaded;
	NotifyInventoryChanged();
	return true;
}

bool UInventoryComponent::AddExistingItemInstance(UInventoryItemInstance* Item)
{
	if (!Item || !Item->ItemData) return false;
	for (UInventoryItemInstance* Existing : Items)
		if (Existing == Item) return false;
	int32 SlotX = -1, SlotY = -1;
	if (!FindSpaceForItem(Item->Width, Item->Height, Item->ShapeMask, SlotX, SlotY)) return false;
	Item->SlotX = SlotX;
	Item->SlotY = SlotY;
	Items.Add(Item);
	OccupyGrid(Item, SlotX, SlotY);
	NotifyInventoryChanged();
	return true;
}
