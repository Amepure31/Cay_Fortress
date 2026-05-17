#include "UI/UI_TrainingMachine.h"
#include "BaseBuilding/TrainingMachineActor.h"
#include "BaseBuilding/BaseBuildingConfigAssets.h"
#include "BaseBuilding/BaseBuildingHelpers.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Alex_PlayerController.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

DEFINE_LOG_CATEGORY_STATIC(LogTrainingUI, Log, All);

void UUI_TrainingMachine::NativeConstruct()
{
	Super::NativeConstruct();
	if (UpgradeButton)
		UpgradeButton->OnClicked.AddDynamic(this, &UUI_TrainingMachine::OnUpgradeButtonClicked);
	if (MaxHealthButton)
		MaxHealthButton->OnClicked.AddDynamic(this, &UUI_TrainingMachine::OnMaxHealthClicked);
	if (MaxStaminaButton)
		MaxStaminaButton->OnClicked.AddDynamic(this, &UUI_TrainingMachine::OnMaxStaminaClicked);
	if (StaminaRecoveryButton)
		StaminaRecoveryButton->OnClicked.AddDynamic(this, &UUI_TrainingMachine::OnStaminaRecoveryClicked);
	if (MaxHungerButton)
		MaxHungerButton->OnClicked.AddDynamic(this, &UUI_TrainingMachine::OnMaxHungerClicked);
	if (MaxHydrationButton)
		MaxHydrationButton->OnClicked.AddDynamic(this, &UUI_TrainingMachine::OnMaxHydrationClicked);
	if (MaxCarryWeightButton)
		MaxCarryWeightButton->OnClicked.AddDynamic(this, &UUI_TrainingMachine::OnMaxCarryWeightClicked);
}

void UUI_TrainingMachine::BindTrainingMachine(ATrainingMachineActor* InMachine)
{
	BoundMachine = InMachine;
	RefreshDisplay();
	BP_OnMachineBound();
}

void UUI_TrainingMachine::SetActiveStatType(ETrainingStatType StatType)
{
	ActiveStatType = StatType;
	RefreshDisplay();
	BP_OnActiveStatTypeChanged(StatType);
}

void UUI_TrainingMachine::RefreshDisplay()
{
	UpdateStatusText();
	UpdateCostDisplay();
	UpdateButtonColors();
}

// ========== Status text ==========

void UUI_TrainingMachine::UpdateStatusText()
{
	if (!StatusText) return;

	FTrainingUpgradeConfig Cfg;
	const bool bConfigured = GetStatUpgradeConfig(ActiveStatType, Cfg);

	if (!bConfigured)
	{
		StatusText->SetText(FText::FromString(TEXT("满级")));
		StatusText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		return;
	}

	const int32 CurrentLevel = GetStatUpgradeLevel(ActiveStatType);
	const int32 MaxLevel = Cfg.MaxLevels;

	if (CurrentLevel >= MaxLevel)
	{
		StatusText->SetText(FText::FromString(TEXT("满级")));
		StatusText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		return;
	}

	const int32 NextLevel = CurrentLevel + 1;
	const FString StatName = StaticEnum<ETrainingStatType>()->GetDisplayNameTextByValue(static_cast<int64>(ActiveStatType)).ToString();
	const FString Status = FString::Printf(TEXT("%s Lv.%d → Lv.%d (+%.1f)"),
		*StatName, CurrentLevel, NextLevel, Cfg.IncrementPerLevel);
	StatusText->SetText(FText::FromString(Status));
	StatusText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

// ========== Cost display ==========

void UUI_TrainingMachine::UpdateCostDisplay()
{
	FTrainingUpgradeConfig Cfg;
	if (!GetStatUpgradeConfig(ActiveStatType, Cfg) || GetStatUpgradeLevel(ActiveStatType) >= Cfg.MaxLevels)
	{
		if (CostImage) CostImage->SetVisibility(ESlateVisibility::Collapsed);
		if (CostText) CostText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (!Cfg.CostItemPath.IsValid() || Cfg.CostAmountPerLevel <= 0)
	{
		if (CostImage) CostImage->SetVisibility(ESlateVisibility::Collapsed);
		if (CostText) CostText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	UInventoryItemDataAsset* DA = TryLoadDataAsset<UInventoryItemDataAsset>(Cfg.CostItemPath);
	if (CostImage)
	{
		if (DA && DA->ItemData.Icon)
		{
			FSlateBrush Brush;
			Brush.SetResourceObject(DA->ItemData.Icon);
			Brush.ImageSize = FVector2D(DA->ItemData.Width * 64.0f, DA->ItemData.Height * 64.0f);
			Brush.DrawAs = ESlateBrushDrawType::Image;
			CostImage->SetBrush(Brush);
			CostImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
			CostImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (CostText)
	{
		CostText->SetText(FText::Format(FText::FromString(TEXT("x{0}")), FText::AsNumber(Cfg.CostAmountPerLevel)));
		CostText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

// ========== Button ==========

void UUI_TrainingMachine::UpdateButtonColors()
{
	if (!UpgradeButton) return;
	const bool bCan = CanUpgradeStat(ActiveStatType);
	UpgradeButton->SetColorAndOpacity(bCan
		? FLinearColor(0.1f, 0.8f, 0.1f, 1.0f)
		: FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
}

void UUI_TrainingMachine::OnUpgradeButtonClicked()
{
	if (TryUpgradeStat(ActiveStatType))
	{
		RefreshDisplay();
		BP_OnTrainingCompleted(ActiveStatType, GetStatUpgradeLevel(ActiveStatType));
	}
}

void UUI_TrainingMachine::OnMaxHealthClicked()      { SetActiveStatType(ETrainingStatType::MaxHealth); }
void UUI_TrainingMachine::OnMaxStaminaClicked()     { SetActiveStatType(ETrainingStatType::MaxStamina); }
void UUI_TrainingMachine::OnStaminaRecoveryClicked(){ SetActiveStatType(ETrainingStatType::StaminaRecovery); }
void UUI_TrainingMachine::OnMaxHungerClicked()      { SetActiveStatType(ETrainingStatType::MaxHunger); }
void UUI_TrainingMachine::OnMaxHydrationClicked()   { SetActiveStatType(ETrainingStatType::MaxHydration); }
void UUI_TrainingMachine::OnMaxCarryWeightClicked() { SetActiveStatType(ETrainingStatType::MaxCarryWeight); }

// ========== Passthrough to BoundMachine ==========

bool UUI_TrainingMachine::TryUpgradeStat(ETrainingStatType StatType)
{
	if (!BoundMachine) return false;
	AAlex_PlayerController* PC = GetOwningPlayer<AAlex_PlayerController>();
	if (!PC) return false;
	if (BoundMachine->UpgradeStat(PC->GetPawn(), StatType))
	{
		PC->RefreshPlayerInventoryUI();
		return true;
	}
	return false;
}

int32 UUI_TrainingMachine::GetStatUpgradeLevel(ETrainingStatType StatType) const
{
	return BoundMachine ? BoundMachine->GetUpgradeLevel(StatType) : 0;
}

bool UUI_TrainingMachine::GetStatUpgradeConfig(ETrainingStatType StatType, FTrainingUpgradeConfig& OutConfig) const
{
	return BoundMachine ? BoundMachine->GetUpgradeConfig(StatType, OutConfig) : false;
}

bool UUI_TrainingMachine::CanUpgradeStat(ETrainingStatType StatType) const
{
	if (!BoundMachine) return false;
	AAlex_PlayerController* PC = GetOwningPlayer<AAlex_PlayerController>();
	if (!PC) return false;
	return BoundMachine->CanUpgradeStat(PC->GetPawn(), StatType);
}

TArray<FTrainingUpgradeConfig> UUI_TrainingMachine::GetAllConfigs() const
{
	if (!BoundMachine) return {};
	auto* CfgAsset = TryLoadDataAsset<UTrainingMachineConfigAsset>(BoundMachine->ConfigAssetPath);
	return CfgAsset ? CfgAsset->UpgradeConfigs : BoundMachine->UpgradeConfigs;
}

void UUI_TrainingMachine::RequestClose()
{
	AAlex_PlayerController* PC = GetOwningPlayer<AAlex_PlayerController>();
	if (PC) PC->CloseTrainingMachineUI(false);
}
