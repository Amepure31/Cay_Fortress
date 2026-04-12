// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/UI_ItemSlot.h"
#include "UI_ItemWidget.generated.h"

class UInventoryItemInstance;
class UImage;
class UTextBlock;

UCLASS()
class CAY_FORTRESS_API UUI_ItemWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual UInventoryItemInstance* GetItemInstance() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual void SetItemInstance(UInventoryItemInstance* InItemInstance);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual void OnDraggedOver(UUI_ItemSlot* TargetSlot, bool bCanPlace);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual void OnDropped(UUI_ItemSlot* TargetSlot);

	virtual void OnDragCancelled();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	UImage* ItemIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	UTextBlock* StackSizeText;

private:
	UPROPERTY()
	UInventoryItemInstance* BoundItem;

	void UpdateStackSizeDisplay();
};
