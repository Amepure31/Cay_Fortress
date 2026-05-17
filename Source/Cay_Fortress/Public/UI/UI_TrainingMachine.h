#pragma once

#include "CoreMinimal.h"
#include "UI/UI_Base_Class.h"
#include "BaseBuilding/TrainingMachineTypes.h"
#include "UI_TrainingMachine.generated.h"

class ATrainingMachineActor;
class UButton;
class UImage;
class UTextBlock;

UCLASS()
class CAY_FORTRESS_API UUI_TrainingMachine : public UUI_Base_Class
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

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
	void SetActiveStatType(ETrainingStatType StatType);

	UFUNCTION(BlueprintCallable, Category = "TrainingMachine")
	void RefreshDisplay();

	UFUNCTION(BlueprintCallable, Category = "TrainingMachine")
	void RequestClose();

	UFUNCTION(BlueprintImplementableEvent, Category = "TrainingMachine")
	void BP_OnTrainingCompleted(ETrainingStatType StatType, int32 NewLevel);

	UFUNCTION(BlueprintImplementableEvent, Category = "TrainingMachine")
	void BP_OnMachineBound();

	UFUNCTION(BlueprintImplementableEvent, Category = "TrainingMachine")
	void BP_OnActiveStatTypeChanged(ETrainingStatType StatType);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UButton> UpgradeButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UButton> MaxHealthButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UButton> MaxStaminaButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UButton> StaminaRecoveryButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UButton> MaxHungerButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UButton> MaxHydrationButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UButton> MaxCarryWeightButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UImage> CostImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UTextBlock> CostText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(BlueprintReadOnly, Category = "TrainingMachine")
	TObjectPtr<ATrainingMachineActor> BoundMachine;

	UPROPERTY(BlueprintReadOnly, Category = "TrainingMachine")
	ETrainingStatType ActiveStatType = ETrainingStatType::MaxHealth;

	UFUNCTION()
	void OnUpgradeButtonClicked();
	UFUNCTION()
	void OnMaxHealthClicked();
	UFUNCTION()
	void OnMaxStaminaClicked();
	UFUNCTION()
	void OnStaminaRecoveryClicked();
	UFUNCTION()
	void OnMaxHungerClicked();
	UFUNCTION()
	void OnMaxHydrationClicked();
	UFUNCTION()
	void OnMaxCarryWeightClicked();

private:
	void UpdateStatusText();
	void UpdateCostDisplay();
	void UpdateButtonColors();
};
