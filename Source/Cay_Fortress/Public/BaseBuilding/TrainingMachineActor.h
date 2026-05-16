#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "BaseBuilding/TrainingMachineTypes.h"
#include "TrainingMachineActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USphereComponent;
class UInventoryComponent;

UCLASS()
class CAY_FORTRESS_API ATrainingMachineActor : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	ATrainingMachineActor();

	// -- Components --
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Training|Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Training|Visuals")
	TObjectPtr<UStaticMeshComponent> MachineMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Training|Components")
	TObjectPtr<USphereComponent> InteractionVolume;

	// -- Config --
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training")
	FText InteractText;

	/** 集中配方 DataAsset（优先）。不填则使用下方的 UpgradeConfigs 数组 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training")
	TObjectPtr<class UTrainingMachineConfigAsset> ConfigAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training")
	TArray<FTrainingUpgradeConfig> UpgradeConfigs;

public:
	UFUNCTION(BlueprintCallable, Category = "Training")
	bool UpgradeStat(AActor* Interactor, ETrainingStatType StatType);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Training")
	int32 GetUpgradeLevel(ETrainingStatType StatType) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Training")
	bool GetUpgradeConfig(ETrainingStatType StatType, FTrainingUpgradeConfig& OutConfig) const;

	UFUNCTION(BlueprintCallable, Category = "Training")
	bool CanUpgradeStat(AActor* Interactor, ETrainingStatType StatType) const;

	// -- Events --
	UFUNCTION(BlueprintImplementableEvent, Category = "Training")
	void BP_OnTrainingCompleted(AActor* User, ETrainingStatType StatType, int32 NewLevel);

	// -- IInteractableInterface --
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractText_Implementation() const override;

private:
	UPROPERTY(SaveGame)
	TMap<uint8, int32> UpgradeLevels;
};
