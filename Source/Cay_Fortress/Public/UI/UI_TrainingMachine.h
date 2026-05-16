#pragma once

#include "CoreMinimal.h"
#include "UI/UI_Base_Class.h"
#include "BaseBuilding/TrainingMachineTypes.h"
#include "UI_TrainingMachine.generated.h"

class ATrainingMachineActor;

UCLASS()
class CAY_FORTRESS_API UUI_TrainingMachine : public UUI_Base_Class
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TrainingMachine")
	void BindTrainingMachine(ATrainingMachineActor* InMachine);

	UFUNCTION(BlueprintCallable, Category = "TrainingMachine")
	bool TryUpgradeStat(ETrainingStatType StatType);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TrainingMachine")
	int32 GetStatUpgradeLevel(ETrainingStatType StatType) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TrainingMachine")
	bool GetStatUpgradeConfig(ETrainingStatType StatType, FTrainingUpgradeConfig& OutConfig) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TrainingMachine")
	bool CanUpgradeStat(ETrainingStatType StatType) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TrainingMachine")
	TArray<FTrainingUpgradeConfig> GetAllConfigs() const;

	UFUNCTION(BlueprintCallable, Category = "TrainingMachine")
	void RequestClose();

	UFUNCTION(BlueprintImplementableEvent, Category = "TrainingMachine")
	void BP_OnTrainingCompleted(ETrainingStatType StatType, int32 NewLevel);

	UFUNCTION(BlueprintImplementableEvent, Category = "TrainingMachine")
	void BP_OnMachineBound();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "TrainingMachine")
	TObjectPtr<ATrainingMachineActor> BoundMachine;
};
