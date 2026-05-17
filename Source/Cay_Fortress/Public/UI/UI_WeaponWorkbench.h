#pragma once

#include "CoreMinimal.h"
#include "UI/UI_Base_Class.h"
#include "BaseBuilding/WeaponWorkbenchTypes.h"
#include "UI_WeaponWorkbench.generated.h"

class AWeaponWorkbenchActor;
class AStorageCabinetActor;
class UInventoryItemInstance;
class UUI_WorkbenchWeaponSlot;
class UButton;
class UImage;
class UTextBlock;

UCLASS()
class CAY_FORTRESS_API UUI_WeaponWorkbench : public UUI_Base_Class
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "WeaponWorkbench")
	void BindWeaponWorkbench(AWeaponWorkbenchActor* InWorkbench);

	// -- Called by WeaponSlot on drop --
	void OnWeaponSlotChanged(UInventoryItemInstance* Weapon);

	// -- Weapon mod --
	UFUNCTION(BlueprintCallable, Category = "WeaponWorkbench")
	bool TryApplyWeaponMod();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "WeaponWorkbench")
	static int32 GetWeaponModLevel(const UInventoryItemInstance* Weapon, EWeaponModType ModType);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "WeaponWorkbench")
	bool GetModConfig(EWeaponModType ModType, FWeaponModConfig& OutConfig) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "WeaponWorkbench")
	bool CanApplyMod() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "WeaponWorkbench")
	TArray<FWeaponModConfig> GetAllConfigs() const;

	UFUNCTION(BlueprintCallable, Category = "WeaponWorkbench")
	void SetActiveModType(EWeaponModType ModType);

	// -- Storage cabinet upgrade --
	UFUNCTION(BlueprintCallable, Category = "WeaponWorkbench")
	bool TryUpgradeStorageCabinet(AStorageCabinetActor* Cabinet);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "WeaponWorkbench")
	bool CanUpgradeStorageCabinet(AStorageCabinetActor* Cabinet) const;

	UFUNCTION(BlueprintCallable, Category = "WeaponWorkbench")
	TArray<AStorageCabinetActor*> GetNearbyStorageCabinets() const;

	// -- Common --
	UFUNCTION(BlueprintCallable, Category = "WeaponWorkbench")
	void RefreshDisplay();

	UFUNCTION(BlueprintCallable, Category = "WeaponWorkbench")
	void RequestClose();

	// Events
	UFUNCTION(BlueprintImplementableEvent, Category = "WeaponWorkbench")
	void BP_OnWeaponModApplied(UInventoryItemInstance* Weapon, EWeaponModType ModType, int32 NewLevel);

	UFUNCTION(BlueprintImplementableEvent, Category = "WeaponWorkbench")
	void BP_OnWorkbenchBound();

	UFUNCTION(BlueprintImplementableEvent, Category = "WeaponWorkbench")
	void BP_OnWeaponBound(UInventoryItemInstance* Weapon);

	UFUNCTION(BlueprintImplementableEvent, Category = "WeaponWorkbench")
	void BP_OnActiveModTypeChanged(EWeaponModType ModType);

	UFUNCTION(BlueprintImplementableEvent, Category = "WeaponWorkbench")
	void BP_OnStorageCabinetUpgraded(AStorageCabinetActor* Cabinet, int32 NewLevel);

protected:
	// -- Widgets --
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UUI_WorkbenchWeaponSlot> WeaponSlot;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UButton> UpgradeButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UButton> CabinetUpgradeButton;

	// Mod cost display
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UImage> CostImage;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UTextBlock> CostText;

	// Status text — prompts, level info, max-level
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UTextBlock> StatusText;

	// Cabinet upgrade cost display (up to 3 items)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UImage> CabinetCostImage1;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UImage> CabinetCostImage2;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UImage> CabinetCostImage3;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UTextBlock> CabinetCostText1;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UTextBlock> CabinetCostText2;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UTextBlock> CabinetCostText3;

	UFUNCTION()
	void OnUpgradeButtonClicked();
	UFUNCTION()
	void OnCabinetUpgradeButtonClicked();

	UPROPERTY(BlueprintReadOnly, Category = "WeaponWorkbench")
	TObjectPtr<AWeaponWorkbenchActor> BoundWorkbench;

	UPROPERTY(BlueprintReadOnly, Category = "WeaponWorkbench")
	EWeaponModType ActiveModType = EWeaponModType::Damage;

private:
	void UpdateCostDisplay();
	void UpdateCabinetCostDisplay();
	void UpdateButtonColors();
	void UpdateStatusText();
	void SetCostSlotWidget(int32 Index, UImage* ImageWidget, UTextBlock* TextWidget, const FSoftObjectPath& ItemPath, int32 Amount);
};
