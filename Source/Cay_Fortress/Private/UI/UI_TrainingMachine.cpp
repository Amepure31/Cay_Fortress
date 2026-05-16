#include "UI/UI_TrainingMachine.h"
#include "BaseBuilding/TrainingMachineActor.h"
#include "Alex_PlayerController.h"

void UUI_TrainingMachine::BindTrainingMachine(ATrainingMachineActor* InMachine)
{
	BoundMachine = InMachine;
	BP_OnMachineBound();
}

bool UUI_TrainingMachine::TryUpgradeStat(ETrainingStatType StatType)
{
	if (!BoundMachine) return false;
	AAlex_PlayerController* PC = GetOwningPlayer<AAlex_PlayerController>();
	if (!PC) return false;
	return BoundMachine->UpgradeStat(PC->GetPawn(), StatType);
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
	return BoundMachine->UpgradeConfigs;
}

void UUI_TrainingMachine::RequestClose()
{
	AAlex_PlayerController* PC = GetOwningPlayer<AAlex_PlayerController>();
	if (PC) PC->CloseTrainingMachineUI(false);
}
