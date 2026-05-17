#pragma once

#include "CoreMinimal.h"
#include "UI/UI_EquipmentSlot.h"
#include "UI_WorkbenchWeaponSlot.generated.h"

class UUI_WeaponWorkbench;
class UInventoryItemInstance;

UCLASS()
class CAY_FORTRESS_API UUI_WorkbenchWeaponSlot : public UUI_EquipmentSlot
{
	GENERATED_BODY()

public:
	/** Bind to parent workbench widget */
	void SetParentWorkbench(UUI_WeaponWorkbench* InWorkbench);

	/** Display a weapon directly, bypassing equipment component */
	UFUNCTION(BlueprintCallable, Category = "WorkbenchSlot")
	void RefreshWithWeapon(UInventoryItemInstance* Weapon);

	/** Clear the bound weapon and restore empty state */
	UFUNCTION(BlueprintCallable, Category = "WorkbenchSlot")
	void ClearWeapon();

	UInventoryItemInstance* GetWeapon() const { return CurrentWeapon; }

protected:
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void RefreshSlotVisual() override;

private:
	UPROPERTY()
	TObjectPtr<UInventoryItemInstance> CurrentWeapon;
	TWeakObjectPtr<UUI_WeaponWorkbench> ParentWorkbench;
};
