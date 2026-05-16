#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "BaseBuilding/WeaponWorkbenchTypes.h"
#include "BaseBuilding/StorageCabinetTypes.h"
#include "WeaponWorkbenchActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USphereComponent;
class UInventoryComponent;
class UInventoryItemDataAsset;
class UInventoryItemInstance;
class AStorageCabinetActor;

UCLASS()
class CAY_FORTRESS_API AWeaponWorkbenchActor : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	AWeaponWorkbenchActor();

protected:
	virtual void BeginPlay() override;

	// -- Components --
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Workbench|Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Workbench|Visuals")
	TObjectPtr<UStaticMeshComponent> WorkbenchMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Workbench|Components")
	TObjectPtr<USphereComponent> InteractionVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Workbench|Inventory")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	// -- Config --
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workbench")
	FText InteractText;

public:
	/** 改装配方 DA 路径（优先） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workbench", meta = (DisplayName = "改装配方DA"))
	FSoftObjectPath WorkbenchConfigAssetPath;

	/** 储物配置 DA 路径（优先，复用储物柜 DA） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workbench", meta = (DisplayName = "储物配置DA"))
	FSoftObjectPath StorageConfigAssetPath;
protected:

	// -- Inline fallbacks --
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workbench|Mods")
	TArray<FWeaponModConfig> ModConfigs;
protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Workbench|Grid", meta = (DisplayName = "初始列数", ClampMin = "1"))
	int32 InitialGridWidth = 4;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Workbench|Grid", meta = (DisplayName = "初始行数", ClampMin = "1"))
	int32 InitialGridHeight = 4;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Workbench|Grid", meta = (DisplayName = "最大列数", ClampMin = "1"))
	int32 MaxGridWidth = 10;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Workbench|Grid", meta = (DisplayName = "最大行数", ClampMin = "1"))
	int32 MaxGridHeight = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Workbench|Upgrade", meta = (DisplayName = "升级等级"))
	TArray<FStorageCabinetUpgradeLevel> UpgradeLevels;

public:
	// -- Weapon mod --
	UFUNCTION(BlueprintCallable, Category = "Workbench")
	bool ApplyWeaponMod(AActor* Interactor, UInventoryItemInstance* WeaponInstance, EWeaponModType ModType);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Workbench")
	static int32 GetWeaponModLevel(const UInventoryItemInstance* WeaponInstance, EWeaponModType ModType);

	UFUNCTION(BlueprintCallable, Category = "Workbench")
	bool CanApplyMod(AActor* Interactor, UInventoryItemInstance* WeaponInstance, EWeaponModType ModType) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Workbench")
	bool GetModConfig(EWeaponModType ModType, FWeaponModConfig& OutConfig) const;

	// -- Storage / grid --
	UFUNCTION(BlueprintCallable, Category = "Workbench")
	bool UpgradeGrid(AActor* Interactor);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Workbench")
	int32 GetUpgradeLevel() const { return UpgradeLevel; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Workbench")
	int32 GetMaxUpgradeLevel() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Workbench")
	bool CanUpgrade(const TArray<UInventoryComponent*>& Sources) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Workbench")
	bool GetCurrentUpgradeCosts(TArray<FStorageCabinetCostEntry>& OutCosts) const;

	UFUNCTION(BlueprintCallable, Category = "Workbench")
	UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintCallable, Category = "Workbench")
	static TArray<UInventoryItemInstance*> GetWeaponsFromInventory(AActor* Interactor);

	/** Bind a weapon into the workbench slot */
	UPROPERTY(BlueprintReadOnly, Category = "Workbench")
	TObjectPtr<UInventoryItemInstance> BoundWeapon;

	UFUNCTION(BlueprintCallable, Category = "Workbench")
	void BindWeapon(UInventoryItemInstance* Weapon);

	/** Upgrade an external storage cabinet using sources (player + this workbench's inventory + other cabinets) */
	UFUNCTION(BlueprintCallable, Category = "Workbench")
	bool UpgradeStorageCabinet(AStorageCabinetActor* Cabinet, AActor* Interactor);

	UFUNCTION(BlueprintCallable, Category = "Workbench")
	void OpenWorkbench();

	UFUNCTION(BlueprintCallable, Category = "Workbench")
	void CloseWorkbench();

	UFUNCTION(BlueprintCallable, Category = "Workbench")
	bool IsWorkbenchOpen() const { return bIsOpen; }

	// -- Events --
	UFUNCTION(BlueprintImplementableEvent, Category = "Workbench")
	void BP_OnWeaponModApplied(AActor* User, UInventoryItemInstance* Weapon, EWeaponModType ModType, int32 NewLevel);

	UFUNCTION(BlueprintImplementableEvent, Category = "Workbench")
	void BP_OnGridUpgraded(int32 NewLevel);

	// -- IInteractableInterface --
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractText_Implementation() const override;

private:
	int32 UpgradeLevel = 0;
	bool bIsOpen = false;

	void ApplyModifierToOverrides(UInventoryItemInstance* Weapon, EWeaponModType ModType, int32 Level) const;
	class UWeaponWorkbenchConfigAsset* LoadWorkbenchConfig() const;
	class UStorageCabinetConfigAsset* LoadStorageConfig() const;
};
