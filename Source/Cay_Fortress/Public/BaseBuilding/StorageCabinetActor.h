#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "BaseBuilding/StorageCabinetTypes.h"
#include "StorageCabinetActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USphereComponent;
class UInventoryComponent;

UCLASS()
class CAY_FORTRESS_API AStorageCabinetActor : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	AStorageCabinetActor();

protected:
	virtual void BeginPlay() override;

	// -- Components --
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cabinet|Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cabinet|Visuals")
	TObjectPtr<UStaticMeshComponent> CabinetMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cabinet|Components")
	TObjectPtr<USphereComponent> InteractionVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cabinet|Inventory")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	// -- Config --
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cabinet")
	FText InteractText;

	/** 集中配置 DataAsset 路径（优先）。不填则使用下方内联属性 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cabinet", meta = (DisplayName = "配置DataAsset"))
	FSoftObjectPath ConfigAssetPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cabinet|Grid", meta = (DisplayName = "初始列数", ClampMin = "1"))
	int32 InitialGridWidth = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cabinet|Grid", meta = (DisplayName = "初始行数", ClampMin = "1"))
	int32 InitialGridHeight = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cabinet|Grid", meta = (DisplayName = "最大列数", ClampMin = "1"))
	int32 MaxGridWidth = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cabinet|Grid", meta = (DisplayName = "最大行数", ClampMin = "1"))
	int32 MaxGridHeight = 10;

	/** 内联升级配置（ConfigAssetPath 优先） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cabinet|Upgrade", meta = (DisplayName = "升级等级"))
	TArray<FStorageCabinetUpgradeLevel> UpgradeLevels;

public:
	UFUNCTION(BlueprintCallable, Category = "Cabinet")
	bool UpgradeGrid(AActor* Interactor);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cabinet")
	int32 GetUpgradeLevel() const { return UpgradeLevel; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cabinet")
	int32 GetMaxUpgradeLevel() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cabinet")
	bool CanUpgrade(const TArray<UInventoryComponent*>& Sources) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cabinet")
	bool GetCurrentUpgradeCosts(TArray<FStorageCabinetCostEntry>& OutCosts) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cabinet")
	UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintCallable, Category = "Cabinet")
	void OpenCabinet();

	UFUNCTION(BlueprintCallable, Category = "Cabinet")
	void CloseCabinet();

	UFUNCTION(BlueprintCallable, Category = "Cabinet")
	bool IsCabinetOpen() const { return bIsCabinetOpen; }

	// -- Events --
	UFUNCTION(BlueprintImplementableEvent, Category = "Cabinet")
	void BP_OnCabinetOpened(AActor* User);

	UFUNCTION(BlueprintImplementableEvent, Category = "Cabinet")
	void BP_OnCabinetUpgraded(int32 NewLevel);

	// -- IInteractableInterface --
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractText_Implementation() const override;

private:
	int32 UpgradeLevel = 0;
	bool bIsCabinetOpen = false;

	class UStorageCabinetConfigAsset* LoadConfigAsset() const;
};
