#include "BaseBuilding/WeaponWorkbenchActor.h"
#include "BaseBuilding/BaseBuildingConfigAssets.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/InventoryItemType.h"
#include "Alex_PlayerCharacter.h"
#include "Alex_PlayerController.h"
#include "Equipment/EquipmentComponent.h"
#include "Equipment/EquipmentTypes.h"
#include "BaseBuilding/StorageCabinetActor.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogWorkbench, Log, All);

template<typename T>
static T* TryLoadDataAsset(const FSoftObjectPath& Path)
{
	if (!Path.IsValid()) return nullptr;
	UObject* Loaded = Path.TryLoad();
	if (!Loaded) return nullptr;
	if (T* Direct = Cast<T>(Loaded)) return Direct;
	if (UBlueprint* BP = Cast<UBlueprint>(Loaded))
		if (BP->GeneratedClass && BP->GeneratedClass->IsChildOf(T::StaticClass()))
			return Cast<T>(BP->GeneratedClass->GetDefaultObject());
	return nullptr;
}

AWeaponWorkbenchActor::AWeaponWorkbenchActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	WorkbenchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WorkbenchMesh"));
	WorkbenchMesh->SetupAttachment(Root);

	InteractionVolume = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(Root);
	InteractionVolume->SetSphereRadius(100.0f);
	InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

	InteractText = FText::FromString(TEXT("按E打开武器工作台"));
}

UWeaponWorkbenchConfigAsset* AWeaponWorkbenchActor::LoadWorkbenchConfig() const
{
	auto* Cfg = TryLoadDataAsset<UWeaponWorkbenchConfigAsset>(WorkbenchConfigAssetPath);
	if (!Cfg && WorkbenchConfigAssetPath.IsValid())
		UE_LOG(LogWorkbench, Warning, TEXT("[工作台] 改装配方 DA 加载失败: %s"), *WorkbenchConfigAssetPath.ToString());
	return Cfg;
}

UStorageCabinetConfigAsset* AWeaponWorkbenchActor::LoadStorageConfig() const
{
	auto* Cfg = TryLoadDataAsset<UStorageCabinetConfigAsset>(StorageConfigAssetPath);
	if (!Cfg && StorageConfigAssetPath.IsValid())
		UE_LOG(LogWorkbench, Warning, TEXT("[工作台] 储物配置 DA 加载失败: %s"), *StorageConfigAssetPath.ToString());
	return Cfg;
}

void AWeaponWorkbenchActor::BeginPlay()
{
	Super::BeginPlay();
	const auto* Cfg = LoadStorageConfig();
	const int32 W = Cfg ? Cfg->InitialGridWidth : InitialGridWidth;
	const int32 H = Cfg ? Cfg->InitialGridHeight : InitialGridHeight;
	UE_LOG(LogWorkbench, Log, TEXT("[工作台] 初始化储物网格 %dx%d"), W, H);
	if (InventoryComponent)
		InventoryComponent->InitializeGrid(FMath::Max(1, W), FMath::Max(1, H));
}

// ========== Weapon mod ==========

int32 AWeaponWorkbenchActor::GetWeaponModLevel(const UInventoryItemInstance* WeaponInstance, EWeaponModType ModType)
{
	if (!WeaponInstance) return 0;
	const uint8 Key = static_cast<uint8>(ModType);
	if (const int32* Found = WeaponInstance->WeaponModLevels.Find(Key)) return *Found;
	return 0;
}

bool AWeaponWorkbenchActor::GetModConfig(EWeaponModType ModType, FWeaponModConfig& OutConfig) const
{
	const auto* Cfg = LoadWorkbenchConfig();
	const TArray<FWeaponModConfig>& Source = Cfg ? Cfg->ModConfigs : ModConfigs;
	for (const FWeaponModConfig& CfgEntry : Source)
		if (CfgEntry.ModType == ModType) { OutConfig = CfgEntry; return true; }
	return false;
}

bool AWeaponWorkbenchActor::CanApplyMod(AActor* Interactor, UInventoryItemInstance* WeaponInstance, EWeaponModType ModType) const
{
	if (!WeaponInstance || !WeaponInstance->ItemData) return false;
	if (WeaponInstance->ItemData->ItemData.ItemType != EInventoryItemType::Weapon) return false;

	FWeaponModConfig Cfg;
	if (!GetModConfig(ModType, Cfg)) return false;
	UInventoryItemDataAsset* CostDA = TryLoadDataAsset<UInventoryItemDataAsset>(Cfg.CostItemPath);
	if (!CostDA || Cfg.CostAmountPerLevel <= 0) return false;
	if (GetWeaponModLevel(WeaponInstance, ModType) >= Cfg.MaxUpgradeLevels) return false;
	return true;
}

namespace
{
	static void CollectAllSources(AActor* Interactor, UInventoryComponent* SelfInv, TArray<UInventoryComponent*>& OutSources)
	{
		if (AAlex_PlayerCharacter* Player = Cast<AAlex_PlayerCharacter>(Interactor))
			if (UInventoryComponent* PlayerInv = Player->FindComponentByClass<UInventoryComponent>())
				OutSources.Add(PlayerInv);
		if (SelfInv)
			OutSources.Add(SelfInv);
		if (UWorld* World = Interactor ? Interactor->GetWorld() : nullptr)
			for (TActorIterator<AStorageCabinetActor> It(World); It; ++It)
				if (UInventoryComponent* CabInv = (*It)->GetInventoryComponent())
					if (CabInv != SelfInv)
						OutSources.Add(CabInv);
	}

	static int32 ConsumeFromSources(UInventoryItemDataAsset* CostItem, int32 Amount, const TArray<UInventoryComponent*>& Sources)
	{
		int32 Remaining = Amount;
		for (UInventoryComponent* Source : Sources)
		{
			if (!Source || Remaining <= 0) continue;
			const int32 Before = Remaining;
			Remaining -= Source->ConsumeItemByDataAsset(CostItem, Remaining);
			if (Remaining < Before)
				UE_LOG(LogWorkbench, Log, TEXT("[工作台] 扣除 %s x%d"), *CostItem->GetName(), Before - Remaining);
		}
		return Amount - Remaining;
	}
}

bool AWeaponWorkbenchActor::ApplyWeaponMod(AActor* Interactor, UInventoryItemInstance* WeaponInstance, EWeaponModType ModType)
{
	if (!CanApplyMod(Interactor, WeaponInstance, ModType)) return false;

	FWeaponModConfig Cfg;
	if (!GetModConfig(ModType, Cfg)) return false;

	TArray<UInventoryComponent*> Sources;
	CollectAllSources(Interactor, InventoryComponent, Sources);
	UE_LOG(LogWorkbench, Log, TEXT("[工作台] 搜索到 %d 个来源, 改装 ModType=%d"), Sources.Num(), static_cast<uint8>(ModType));

	UInventoryItemDataAsset* CostDA = TryLoadDataAsset<UInventoryItemDataAsset>(Cfg.CostItemPath);
	if (!CostDA) return false;

	const int32 Taken = ConsumeFromSources(CostDA, Cfg.CostAmountPerLevel, Sources);
	if (Taken < Cfg.CostAmountPerLevel) return false;

	const int32 OldLevel = GetWeaponModLevel(WeaponInstance, ModType);
	const int32 NewLevel = OldLevel + 1;
	WeaponInstance->WeaponModLevels.Add(static_cast<uint8>(ModType), NewLevel);
	ApplyModifierToOverrides(WeaponInstance, ModType, NewLevel);

	UE_LOG(LogWorkbench, Log, TEXT("[工作台] 改装完成 %s ModType=%d Lv%d->%d"),
		*WeaponInstance->GetName(), static_cast<uint8>(ModType), OldLevel, NewLevel);
	BP_OnWeaponModApplied(Interactor, WeaponInstance, ModType, NewLevel);
	return true;
}

void AWeaponWorkbenchActor::ApplyModifierToOverrides(UInventoryItemInstance* Weapon, EWeaponModType ModType, int32 Level) const
{
	if (!Weapon || !Weapon->ItemData) return;
	FWeaponModConfig Cfg;
	if (!GetModConfig(ModType, Cfg)) return;

	const FWeaponItemStats& Base = Weapon->ItemData->ItemData.WeaponStats;
	const float Eff = Cfg.ModifierPerLevel * Level;

	switch (ModType)
	{
	case EWeaponModType::Damage:
		Weapon->WeaponStatOverrides.bOverrideDamage = true;
		Weapon->WeaponStatOverrides.Damage = Cfg.bIsMultiplicative ? Base.Damage * FMath::Pow(Cfg.ModifierPerLevel, Level) : Base.Damage + Eff; break;
	case EWeaponModType::FireRate:
		Weapon->WeaponStatOverrides.bOverrideFireRate = true;
		Weapon->WeaponStatOverrides.FireRate = Cfg.bIsMultiplicative ? Base.FireRate * FMath::Pow(Cfg.ModifierPerLevel, Level) : Base.FireRate + Eff; break;
	case EWeaponModType::MagazineCapacity:
		Weapon->WeaponStatOverrides.bOverrideMagazineCapacity = true;
		Weapon->WeaponStatOverrides.MagazineCapacity = FMath::Max(1, Cfg.bIsMultiplicative ? FMath::RoundToInt(Base.MagazineCapacity * FMath::Pow(Cfg.ModifierPerLevel, Level)) : Base.MagazineCapacity + FMath::RoundToInt(Eff)); break;
	case EWeaponModType::Recoil:
		Weapon->WeaponStatOverrides.bOverrideHorizontalRecoil = true;
		Weapon->WeaponStatOverrides.bOverrideVerticalRecoil = true;
		Weapon->WeaponStatOverrides.HorizontalRecoil = Cfg.bIsMultiplicative ? Base.HorizontalRecoil * FMath::Pow(Cfg.ModifierPerLevel, Level) : Base.HorizontalRecoil + Eff;
		Weapon->WeaponStatOverrides.VerticalRecoil = Cfg.bIsMultiplicative ? Base.VerticalRecoil * FMath::Pow(Cfg.ModifierPerLevel, Level) : Base.VerticalRecoil + Eff; break;
	case EWeaponModType::Range:
		Weapon->WeaponStatOverrides.bOverrideRange = true;
		Weapon->WeaponStatOverrides.Range = Cfg.bIsMultiplicative ? Base.Range * FMath::Pow(Cfg.ModifierPerLevel, Level) : Base.Range + Eff; break;
	case EWeaponModType::ReloadTime:
		Weapon->WeaponStatOverrides.bOverrideReloadTime = true;
		Weapon->WeaponStatOverrides.ReloadTime = Cfg.bIsMultiplicative ? Base.ReloadTime * FMath::Pow(Cfg.ModifierPerLevel, Level) : Base.ReloadTime + Eff; break;
	case EWeaponModType::HipFireAccuracy:
		Weapon->WeaponStatOverrides.bOverrideHipFireAccuracy = true;
		Weapon->WeaponStatOverrides.HipFireAccuracy = Cfg.bIsMultiplicative ? Base.HipFireAccuracy * FMath::Pow(Cfg.ModifierPerLevel, Level) : Base.HipFireAccuracy + Eff; break;
	case EWeaponModType::ADSAccuracy:
		Weapon->WeaponStatOverrides.bOverrideADSAccuracy = true;
		Weapon->WeaponStatOverrides.ADSAccuracy = Cfg.bIsMultiplicative ? Base.ADSAccuracy * FMath::Pow(Cfg.ModifierPerLevel, Level) : Base.ADSAccuracy + Eff; break;
	case EWeaponModType::Handling:
		Weapon->WeaponStatOverrides.bOverrideHandling = true;
		Weapon->WeaponStatOverrides.Handling = Cfg.bIsMultiplicative ? Base.Handling * FMath::Pow(Cfg.ModifierPerLevel, Level) : Base.Handling + Eff; break;
	}
}

// ========== Storage / upgrade ==========

int32 AWeaponWorkbenchActor::GetMaxUpgradeLevel() const
{
	const auto* Cfg = LoadStorageConfig();
	const TArray<FStorageCabinetUpgradeLevel>& Levels = Cfg ? Cfg->UpgradeLevels : UpgradeLevels;
	return Levels.Num();
}

bool AWeaponWorkbenchActor::CanUpgrade(const TArray<UInventoryComponent*>& Sources) const
{
	if (UpgradeLevel >= GetMaxUpgradeLevel()) return false;
	TArray<FStorageCabinetCostEntry> Costs;
	if (!GetCurrentUpgradeCosts(Costs) || Costs.Num() == 0) return false;
	for (const FStorageCabinetCostEntry& Entry : Costs)
	{
		UInventoryItemDataAsset* CostDA = TryLoadDataAsset<UInventoryItemDataAsset>(Entry.ItemPath);
		if (!CostDA) return false;
		int32 Remaining = Entry.Amount;
		for (const UInventoryComponent* Source : Sources)
		{
			if (!Source) continue;
			int32 Available = 0;
			for (const TObjectPtr<UInventoryItemInstance>& Item : Source->Items)
				if (Item && Item->ItemData == CostDA) Available += FMath::Max(0, Item->StackSize);
			Remaining -= FMath::Min(Remaining, Available);
			if (Remaining <= 0) break;
		}
		if (Remaining > 0) return false;
	}
	return true;
}

bool AWeaponWorkbenchActor::GetCurrentUpgradeCosts(TArray<FStorageCabinetCostEntry>& OutCosts) const
{
	const auto* Cfg = LoadStorageConfig();
	const TArray<FStorageCabinetUpgradeLevel>& Levels = Cfg ? Cfg->UpgradeLevels : UpgradeLevels;
	if (UpgradeLevel < 0 || UpgradeLevel >= Levels.Num()) return false;
	OutCosts = Levels[UpgradeLevel].CostEntries;
	return true;
}

namespace
{
	static int32 ConsumeCostEntry(const FStorageCabinetCostEntry& Entry, const TArray<UInventoryComponent*>& Sources)
	{
		UInventoryItemDataAsset* CostDA = TryLoadDataAsset<UInventoryItemDataAsset>(Entry.ItemPath);
		if (!CostDA) return 0;
		int32 Remaining = Entry.Amount;
		for (UInventoryComponent* Source : Sources)
		{
			if (!Source || Remaining <= 0) continue;
			const int32 Before = Remaining;
			Remaining -= Source->ConsumeItemByDataAsset(CostDA, Remaining);
			if (Remaining < Before)
				UE_LOG(LogWorkbench, Log, TEXT("[工作台] 升级扣除 %s x%d"), *CostDA->GetName(), Before - Remaining);
		}
		return Entry.Amount - Remaining;
	}
}

bool AWeaponWorkbenchActor::UpgradeGrid(AActor* Interactor)
{
	if (!InventoryComponent) return false;
	TArray<UInventoryComponent*> Sources;
	CollectAllSources(Interactor, InventoryComponent, Sources);

	if (!CanUpgrade(Sources)) return false;

	TArray<FStorageCabinetCostEntry> Costs;
	if (!GetCurrentUpgradeCosts(Costs)) return false;
	for (const FStorageCabinetCostEntry& Entry : Costs)
		if (ConsumeCostEntry(Entry, Sources) < Entry.Amount) return false;

	const auto* Cfg = LoadStorageConfig();
	const TArray<FStorageCabinetUpgradeLevel>& Levels = Cfg ? Cfg->UpgradeLevels : UpgradeLevels;
	const FStorageCabinetUpgradeLevel& Lvl = Levels[UpgradeLevel];

	const int32 MW = Cfg ? Cfg->MaxGridWidth : MaxGridWidth;
	const int32 MH = Cfg ? Cfg->MaxGridHeight : MaxGridHeight;
	const int32 NewW = FMath::Min(InventoryComponent->GridWidth + Lvl.AddedColumns, MW);
	const int32 NewH = FMath::Min(InventoryComponent->GridHeight + Lvl.AddedRows, MH);
	UE_LOG(LogWorkbench, Log, TEXT("[工作台] 升级网格 %dx%d -> %dx%d"), InventoryComponent->GridWidth, InventoryComponent->GridHeight, NewW, NewH);
	if (!InventoryComponent->ExpandGrid(NewW, NewH)) return false;

	++UpgradeLevel;
	BP_OnGridUpgraded(UpgradeLevel);
	return true;
}

// ========== Weapons from inventory + equipment ==========

TArray<UInventoryItemInstance*> AWeaponWorkbenchActor::GetWeaponsFromInventory(AActor* Interactor)
{
	TArray<UInventoryItemInstance*> Weapons;
	if (AAlex_PlayerCharacter* Player = Cast<AAlex_PlayerCharacter>(Interactor))
	{
		if (UInventoryComponent* Inv = Player->FindComponentByClass<UInventoryComponent>())
			for (const TObjectPtr<UInventoryItemInstance>& Item : Inv->Items)
				if (Item && Item->ItemData && Item->ItemData->ItemData.ItemType == EInventoryItemType::Weapon)
					Weapons.Add(Item);
		if (UEquipmentComponent* Equip = Player->FindComponentByClass<UEquipmentComponent>())
			for (const FEquipmentSlotEntry& Slot : Equip->GetAllSlots())
				if ((Slot.SlotType == EEquipmentSlotType::Weapon1 || Slot.SlotType == EEquipmentSlotType::Weapon2) && Slot.EquippedItem && Slot.EquippedItem->ItemData)
					Weapons.Add(Slot.EquippedItem);
	}
	return Weapons;
}

// ========== Bind weapon ==========

DEFINE_LOG_CATEGORY_STATIC(LogWeaponWorkbench, Log, All);

void AWeaponWorkbenchActor::BindWeapon(UInventoryItemInstance* Weapon)
{
	UE_LOG(LogWeaponWorkbench, Log, TEXT("[WorkbenchActor] BindWeapon called — Weapon=%s, previous BoundWeapon=%s"),
		Weapon ? *Weapon->GetName() : TEXT("null (unbind)"),
		BoundWeapon ? *BoundWeapon->GetName() : TEXT("null"));

	if (!Weapon)
	{
		BoundWeapon = nullptr;
		UE_LOG(LogWeaponWorkbench, Log, TEXT("[WorkbenchActor] BoundWeapon cleared"));
		return;
	}
	if (Weapon->ItemData && Weapon->ItemData->ItemData.ItemType == EInventoryItemType::Weapon)
	{
		BoundWeapon = Weapon;
		UE_LOG(LogWeaponWorkbench, Log, TEXT("[WorkbenchActor] BoundWeapon set to '%s' (Type=Weapon)"), *Weapon->GetName());
	}
	else
	{
		UE_LOG(LogWeaponWorkbench, Warning, TEXT("[WorkbenchActor] BindWeapon REJECTED — ItemData=%s, ItemType=%d"),
			Weapon->ItemData ? TEXT("valid") : TEXT("null"),
			Weapon->ItemData ? static_cast<int32>(Weapon->ItemData->ItemData.ItemType) : -1);
	}
}

// ========== Upgrade external storage cabinet ==========

bool AWeaponWorkbenchActor::UpgradeStorageCabinet(AStorageCabinetActor* Cabinet, AActor* Interactor)
{
	if (!Cabinet || !Cabinet->GetInventoryComponent()) return false;

	TArray<UInventoryComponent*> Sources;
	CollectAllSources(Interactor, InventoryComponent, Sources);
	// Also include the target cabinet's own inventory
	if (UInventoryComponent* CabInv = Cabinet->GetInventoryComponent())
		Sources.AddUnique(CabInv);

	if (!Cabinet->CanUpgrade(Sources)) return false;
	return Cabinet->UpgradeGrid(Interactor);
}

// ========== Open / Close ==========

void AWeaponWorkbenchActor::OpenWorkbench() { bIsOpen = true; }
void AWeaponWorkbenchActor::CloseWorkbench() { bIsOpen = false; }

// ========== Interaction ==========

bool AWeaponWorkbenchActor::CanInteract_Implementation(AActor* Interactor) const { return IsValid(Interactor); }

void AWeaponWorkbenchActor::Interact_Implementation(AActor* Interactor)
{
	if (APawn* Pawn = Cast<APawn>(Interactor))
		if (AAlex_PlayerController* PC = Cast<AAlex_PlayerController>(Pawn->GetController()))
			PC->OpenWeaponWorkbenchUI(this);
}

FText AWeaponWorkbenchActor::GetInteractText_Implementation() const { return InteractText; }
