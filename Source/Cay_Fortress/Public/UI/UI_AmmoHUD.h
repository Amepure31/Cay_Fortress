// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/UI_Base_Class.h"
#include "UI_AmmoHUD.generated.h"

class UTextBlock;
class UImage;
class UTexture2D;

/**
 * 弹药 HUD：当前弹匣、背包剩余（与武器弹药类型一致）、手枪/步枪图标。
 * Widget 蓝图中可将控件命名为 Text_AmmoCurrent、Text_AmmoReserve、Image_WeaponType（均可选，BindWidgetOptional）。
 * 手枪与步枪贴图可在 Widget 默认值或子类中指定（PistolTypeHudTexture / RifleTypeHudTexture）。
 */
UCLASS()
class CAY_FORTRESS_API UUI_AmmoHUD : public UUI_Base_Class
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Combat|UI")
	void RefreshAmmoDisplay();

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_AmmoCurrent;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_AmmoReserve;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_WeaponType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|UI")
	TObjectPtr<UTexture2D> PistolTypeHudTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|UI")
	TObjectPtr<UTexture2D> RifleTypeHudTexture;

	/** 武器类型图标显示高度（Slate 单位）；宽度按贴图原始比例自动算出 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|UI", meta = (ClampMin = "1", UIMin = "1"))
	float WeaponTypeIconFixedHeight = 48.f;

	void ApplyWeaponTypeIconWithFixedHeight(UTexture2D* Icon);
};
