// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/UI_Base_Class.h"
#include "UI_CharacterProperty.generated.h"

class UTextBlock;
class UProgressBar;

/**
 * 角色属性 HUD：生命、体力、饱食、水分、护甲、负重。
 * Widget 蓝图中可将控件按名称绑定（均可选，BindWidgetOptional），不需要的留空即可。
 */
UCLASS()
class CAY_FORTRESS_API UUI_CharacterProperty : public UUI_Base_Class
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Character|UI")
	void RefreshStatusDisplay();

protected:
	// --- Health ---
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> Bar_Health;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Health;

	// --- Stamina ---
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> Bar_Stamina;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Stamina;

	// --- Hunger ---
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> Bar_Hunger;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Hunger;

	// --- Hydration ---
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> Bar_Hydration;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Hydration;

	// --- Armor ---
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ArmorValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DamageReduction;

	// --- Carry Weight ---
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> Bar_CarryWeight;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CarryWeight;
};
