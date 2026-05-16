#include "BaseBuilding/StorageCabinetActor.h"
#include "BaseBuilding/BaseBuildingConfigAssets.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Alex_PlayerCharacter.h"
#include "Alex_PlayerController.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogCabinet, Log, All);

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

AStorageCabinetActor::AStorageCabinetActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	CabinetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CabinetMesh"));
	CabinetMesh->SetupAttachment(Root);

	InteractionVolume = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(Root);
	InteractionVolume->SetSphereRadius(100.0f);
	InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

	InteractText = FText::FromString(TEXT("按E打开储物柜"));
}

UStorageCabinetConfigAsset* AStorageCabinetActor::LoadConfigAsset() const
{
	UStorageCabinetConfigAsset* Cfg = TryLoadDataAsset<UStorageCabinetConfigAsset>(ConfigAssetPath);
	if (!ConfigAssetPath.IsValid())
	{
		UE_LOG(LogCabinet, Log, TEXT("[%s] ConfigAssetPath 未设置，使用内联配置"), *GetName());
	}
	else if (!Cfg)
	{
		UE_LOG(LogCabinet, Warning, TEXT("[%s] ConfigAssetPath 加载失败: %s"), *GetName(), *ConfigAssetPath.ToString());
	}
	else
	{
		UE_LOG(LogCabinet, Log, TEXT("[%s] ConfigAsset 加载成功: %s"), *GetName(), *Cfg->GetName());
	}
	return Cfg;
}

void AStorageCabinetActor::BeginPlay()
{
	Super::BeginPlay();
	const auto* Cfg = LoadConfigAsset();
	const int32 W = Cfg ? Cfg->InitialGridWidth : InitialGridWidth;
	const int32 H = Cfg ? Cfg->InitialGridHeight : InitialGridHeight;
	UE_LOG(LogCabinet, Log, TEXT("[%s] 初始化网格 %dx%d"), *GetName(), W, H);
	if (InventoryComponent)
		InventoryComponent->InitializeGrid(FMath::Max(1, W), FMath::Max(1, H));
}

int32 AStorageCabinetActor::GetMaxUpgradeLevel() const
{
	const auto* Cfg = LoadConfigAsset();
	const TArray<FStorageCabinetUpgradeLevel>& Levels = Cfg ? Cfg->UpgradeLevels : UpgradeLevels;
	return Levels.Num();
}

bool AStorageCabinetActor::CanUpgrade(const TArray<UInventoryComponent*>& Sources) const
{
	if (UpgradeLevel >= GetMaxUpgradeLevel())
	{
		UE_LOG(LogCabinet, Log, TEXT("[%s] 已达最大升级等级 %d/%d"), *GetName(), UpgradeLevel, GetMaxUpgradeLevel());
		return false;
	}

	TArray<FStorageCabinetCostEntry> Costs;
	if (!GetCurrentUpgradeCosts(Costs) || Costs.Num() == 0)
	{
		UE_LOG(LogCabinet, Warning, TEXT("[%s] 当前等级 %d 无消耗配置"), *GetName(), UpgradeLevel);
		return false;
	}

	for (const FStorageCabinetCostEntry& Entry : Costs)
	{
		UInventoryItemDataAsset* CostDA = TryLoadDataAsset<UInventoryItemDataAsset>(Entry.ItemPath);
		if (!CostDA)
		{
			UE_LOG(LogCabinet, Warning, TEXT("[%s] 消耗物品路径加载失败: %s"), *GetName(), *Entry.ItemPath.ToString());
			return false;
		}

		int32 Remaining = Entry.Amount;
		for (const UInventoryComponent* Source : Sources)
		{
			if (!Source) continue;
			int32 Available = 0;
			for (const TObjectPtr<UInventoryItemInstance>& Item : Source->Items)
				if (Item && Item->ItemData == CostDA)
					Available += FMath::Max(0, Item->StackSize);
			Remaining -= FMath::Min(Remaining, Available);
			if (Remaining <= 0) break;
		}
		if (Remaining > 0)
		{
			UE_LOG(LogCabinet, Log, TEXT("[%s] 升级不满足: 需要 %s x%d, 还缺 %d"), *GetName(), *CostDA->GetName(), Entry.Amount, Remaining);
			return false;
		}
		UE_LOG(LogCabinet, Log, TEXT("[%s] 消耗满足: %s x%d"), *GetName(), *CostDA->GetName(), Entry.Amount);
	}
	return true;
}

bool AStorageCabinetActor::GetCurrentUpgradeCosts(TArray<FStorageCabinetCostEntry>& OutCosts) const
{
	const auto* Cfg = LoadConfigAsset();
	const TArray<FStorageCabinetUpgradeLevel>& Levels = Cfg ? Cfg->UpgradeLevels : UpgradeLevels;
	if (UpgradeLevel < 0 || UpgradeLevel >= Levels.Num()) return false;
	OutCosts = Levels[UpgradeLevel].CostEntries;
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

	static int32 ConsumeSources(const FStorageCabinetCostEntry& Entry, const TArray<UInventoryComponent*>& Sources)
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
				UE_LOG(LogCabinet, Log, TEXT("[储物柜] 从来源扣除 %s x%d"), *CostDA->GetName(), Before - Remaining);
		}
		return Entry.Amount - Remaining;
	}
}

bool AStorageCabinetActor::UpgradeGrid(AActor* Interactor)
{
	if (!InventoryComponent) return false;

	TArray<UInventoryComponent*> Sources;
	CollectAllSources(Interactor, InventoryComponent, Sources);
	UE_LOG(LogCabinet, Log, TEXT("[%s] 搜索到 %d 个物品来源"), *GetName(), Sources.Num());

	if (!CanUpgrade(Sources)) return false;

	TArray<FStorageCabinetCostEntry> Costs;
	if (!GetCurrentUpgradeCosts(Costs)) return false;

	// Deduct all costs
	for (const FStorageCabinetCostEntry& Entry : Costs)
	{
		const int32 Taken = ConsumeSources(Entry, Sources);
		if (Taken < Entry.Amount)
		{
			UE_LOG(LogCabinet, Error, TEXT("[%s] 升级扣除物品失败！需要 %d, 只扣了 %d"), *GetName(), Entry.Amount, Taken);
			return false;
		}
	}

	const auto* Cfg = LoadConfigAsset();
	const TArray<FStorageCabinetUpgradeLevel>& Levels = Cfg ? Cfg->UpgradeLevels : UpgradeLevels;
	const FStorageCabinetUpgradeLevel& Lvl = Levels[UpgradeLevel];

	const int32 MW = Cfg ? Cfg->MaxGridWidth : MaxGridWidth;
	const int32 MH = Cfg ? Cfg->MaxGridHeight : MaxGridHeight;
	const int32 OldW = InventoryComponent->GridWidth;
	const int32 OldH = InventoryComponent->GridHeight;
	const int32 NewWidth = FMath::Min(OldW + Lvl.AddedColumns, MW);
	const int32 NewHeight = FMath::Min(OldH + Lvl.AddedRows, MH);

	UE_LOG(LogCabinet, Log, TEXT("[%s] 升级网格 %dx%d -> %dx%d (等级 %d -> %d)"),
		*GetName(), OldW, OldH, NewWidth, NewHeight, UpgradeLevel, UpgradeLevel + 1);

	if (!InventoryComponent->ExpandGrid(NewWidth, NewHeight))
	{
		UE_LOG(LogCabinet, Error, TEXT("[%s] ExpandGrid 失败!"), *GetName());
		return false;
	}

	++UpgradeLevel;
	BP_OnCabinetUpgraded(UpgradeLevel);
	return true;
}

void AStorageCabinetActor::OpenCabinet()
{
	bIsCabinetOpen = true;
}

void AStorageCabinetActor::CloseCabinet()
{
	bIsCabinetOpen = false;
}

bool AStorageCabinetActor::CanInteract_Implementation(AActor* Interactor) const
{
	return IsValid(Interactor) && InventoryComponent != nullptr;
}

void AStorageCabinetActor::Interact_Implementation(AActor* Interactor)
{
	if (!CanInteract_Implementation(Interactor)) return;
	if (APawn* Pawn = Cast<APawn>(Interactor))
		if (AAlex_PlayerController* PC = Cast<AAlex_PlayerController>(Pawn->GetController()))
			PC->OpenStorageCabinetUI(this);
}

FText AStorageCabinetActor::GetInteractText_Implementation() const
{
	return InteractText;
}
