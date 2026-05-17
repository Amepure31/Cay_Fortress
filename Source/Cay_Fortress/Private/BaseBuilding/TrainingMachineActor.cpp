#include "BaseBuilding/TrainingMachineActor.h"
#include "BaseBuilding/BaseBuildingConfigAssets.h"
#include "BaseBuilding/BaseBuildingHelpers.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Alex_PlayerCharacter.h"
#include "Alex_PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogTraining, Log, All);

ATrainingMachineActor::ATrainingMachineActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	MachineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MachineMesh"));
	MachineMesh->SetupAttachment(Root);

	InteractionVolume = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(Root);
	InteractionVolume->SetSphereRadius(100.0f);
	InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	InteractText = FText::FromString(TEXT("按E打开训练机"));
}

int32 ATrainingMachineActor::GetUpgradeLevel(ETrainingStatType StatType) const
{
	const uint8 Key = static_cast<uint8>(StatType);
	if (const int32* Found = UpgradeLevels.Find(Key)) return *Found;
	return 0;
}

bool ATrainingMachineActor::GetUpgradeConfig(ETrainingStatType StatType, FTrainingUpgradeConfig& OutConfig) const
{
	auto* CfgAsset = TryLoadDataAsset<UTrainingMachineConfigAsset>(ConfigAssetPath);
	const TArray<FTrainingUpgradeConfig>& Source = CfgAsset ? CfgAsset->UpgradeConfigs : UpgradeConfigs;
	for (const FTrainingUpgradeConfig& Cfg : Source)
	{
		if (Cfg.StatType == StatType)
		{
			OutConfig = Cfg;
			return true;
		}
	}
	return false;
}

bool ATrainingMachineActor::CanUpgradeStat(AActor* Interactor, ETrainingStatType StatType) const
{
	FTrainingUpgradeConfig Cfg;
	if (!GetUpgradeConfig(StatType, Cfg))
	{
		//UE_LOG(LogTraining, Warning, TEXT("[训练机] 未找到 StatType=%d 的配置"), static_cast<uint8>(StatType));
		return false;
	}
	UInventoryItemDataAsset* CostDA = TryLoadDataAsset<UInventoryItemDataAsset>(Cfg.CostItemPath);
	if (!CostDA)
	{
		//UE_LOG(LogTraining, Warning, TEXT("[训练机] CostItemPath 解析失败: %s"), *Cfg.CostItemPath.ToString());
		return false;
	}
	if (Cfg.CostAmountPerLevel <= 0) return false;
	const int32 CurrentLevel = GetUpgradeLevel(StatType);
	if (CurrentLevel >= Cfg.MaxLevels)
	{
		//UE_LOG(LogTraining, Log, TEXT("[训练机] StatType=%d 已达最大等级 %d/%d"), static_cast<uint8>(StatType), CurrentLevel, Cfg.MaxLevels);
		return false;
	}
	return true;
}

bool ATrainingMachineActor::UpgradeStat(AActor* Interactor, ETrainingStatType StatType)
{
	FTrainingUpgradeConfig Cfg;
	if (!GetUpgradeConfig(StatType, Cfg)) return false;
	if (!CanUpgradeStat(Interactor, StatType)) return false;

	AAlex_PlayerCharacter* Player = Cast<AAlex_PlayerCharacter>(Interactor);
	if (!Player) return false;

	TArray<UInventoryComponent*> Sources;
	CollectAllSources(Interactor, Sources);
	//UE_LOG(LogTraining, Log, TEXT("[训练机] 搜索到 %d 个物品来源, 准备训练 StatType=%d"), Sources.Num(), static_cast<uint8>(StatType));

	UInventoryItemDataAsset* CostDA = TryLoadDataAsset<UInventoryItemDataAsset>(Cfg.CostItemPath);
	if (!CostDA) return false;

	const int32 Taken = ConsumeFromSources(CostDA, Cfg.CostAmountPerLevel, Sources);
	if (Taken < Cfg.CostAmountPerLevel)
	{
		//UE_LOG(LogTraining, Warning, TEXT("[训练机] 消耗不足! 需要 %s x%d, 只扣了 %d"), *CostDA->GetName(), Cfg.CostAmountPerLevel, Taken);
		return false;
	}

	const float OldVal = [&]() -> float {
		switch (StatType)
		{
		case ETrainingStatType::MaxHealth:       return Player->MaxHealth;
		case ETrainingStatType::MaxStamina:      return Player->MaxStamina;
		case ETrainingStatType::StaminaRecovery: return Player->StaminaRecoveryRate;
		case ETrainingStatType::MaxHunger:       return Player->MaxHunger;
		case ETrainingStatType::MaxHydration:    return Player->MaxHydration;
		case ETrainingStatType::MaxCarryWeight:  return Player->MaxCarryWeight;
		default: return 0.0f;
		}
	}();

	switch (StatType)
	{
	case ETrainingStatType::MaxHealth:       Player->SetMaxHealth(Player->MaxHealth + Cfg.IncrementPerLevel); break;
	case ETrainingStatType::MaxStamina:      Player->SetMaxStamina(Player->MaxStamina + Cfg.IncrementPerLevel); break;
	case ETrainingStatType::StaminaRecovery: Player->SetStaminaRecoveryRate(Player->StaminaRecoveryRate + Cfg.IncrementPerLevel); break;
	case ETrainingStatType::MaxHunger:       Player->SetMaxHunger(Player->MaxHunger + Cfg.IncrementPerLevel); break;
	case ETrainingStatType::MaxHydration:    Player->SetMaxHydration(Player->MaxHydration + Cfg.IncrementPerLevel); break;
	case ETrainingStatType::MaxCarryWeight:  Player->SetMaxCarryWeight(Player->MaxCarryWeight + Cfg.IncrementPerLevel); break;
	}

	const uint8 Key = static_cast<uint8>(StatType);
	const int32 NewLevel = GetUpgradeLevel(StatType) + 1;
	UpgradeLevels.Add(Key, NewLevel);

	//UE_LOG(LogTraining, Log, TEXT("[训练机] 训练完成 StatType=%d, 等级 %d, 属性 %.1f -> %.1f (+%.1f)"),
		//static_cast<uint8>(StatType), NewLevel, OldVal, OldVal + Cfg.IncrementPerLevel, Cfg.IncrementPerLevel);

	BP_OnTrainingCompleted(Interactor, StatType, NewLevel);
	return true;
}

bool ATrainingMachineActor::CanInteract_Implementation(AActor* Interactor) const
{
	return IsValid(Interactor);
}

void ATrainingMachineActor::Interact_Implementation(AActor* Interactor)
{
	if (APawn* Pawn = Cast<APawn>(Interactor))
	{
		if (AAlex_PlayerController* PC = Cast<AAlex_PlayerController>(Pawn->GetController()))
		{
			PC->OpenTrainingMachineUI(this);
		}
	}
}

FText ATrainingMachineActor::GetInteractText_Implementation() const
{
	return InteractText;
}
