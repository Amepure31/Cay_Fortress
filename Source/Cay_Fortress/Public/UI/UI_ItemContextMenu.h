#pragma once

#include "CoreMinimal.h"
#include "UI/UI_Base_Class.h"
#include "UI_ItemContextMenu.generated.h"

class UButton;
class UTextBlock;
class UUI_Inventory;
class UInventoryItemInstance;

UCLASS()
class CAY_FORTRESS_API UUI_ItemContextMenu : public UUI_Base_Class
{
	GENERATED_BODY()

public:
	void Show(UUI_Inventory* InInventory, UInventoryItemInstance* InItem, FVector2D ScreenPosition);

	UFUNCTION(BlueprintCallable, Category = "ContextMenu")
	static void CloseActiveMenu();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UButton> UseButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UTextBlock> UseButtonText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UButton> DiscardButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UTextBlock> DiscardButtonText;

private:
	UFUNCTION()
	void OnUseClicked();

	UFUNCTION()
	void OnDiscardClicked();

	void CloseMenu();

	TWeakObjectPtr<UUI_Inventory> OwningInventory;
	TWeakObjectPtr<UInventoryItemInstance> TargetItem;
	float Elapsed = 0.0f;

	static TWeakObjectPtr<UUI_ItemContextMenu> ActiveMenu;
};
