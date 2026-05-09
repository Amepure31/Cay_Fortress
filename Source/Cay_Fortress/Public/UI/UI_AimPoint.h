#pragma once

#include "CoreMinimal.h"
#include "UI/UI_Base_Class.h"
#include "UI_AimPoint.generated.h"

class UImage;
class AAlex_PlayerCharacter;

/**
 * 瞄准准星：仅瞄准时显示；可选第二点沿枪口射线弹着点偏移。
 * 在 Widget 蓝图中将控件命名为 ReticleCenter、ReticleFollow（均可选，BindWidgetOptional）。
 * 建议根为全屏 Overlay，子 Image 锚点居中 (0.5, 0.5)，跟随点用 C++ SetRenderTranslation 相对屏幕中心偏移。
 */
UCLASS()
class CAY_FORTRESS_API UUI_AimPoint : public UUI_Base_Class
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** 由 PlayerController Tick 调用，刷新显隐与跟随偏移 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Aim")
	void RefreshAimDisplay();

	UFUNCTION(BlueprintCallable, Category = "Combat|Aim")
	void SetFollowImpactEnabled(bool bEnabled) { bFollowImpactPoint = bEnabled; }

	UFUNCTION(BlueprintPure, Category = "Combat|Aim")
	bool IsFollowImpactEnabled() const { return bFollowImpactPoint; }

	/** 是否显示沿弹着点偏移的第二准星（可在设置菜单中切换） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	bool bFollowImpactPoint;

	/** 弹着点无法投影到屏幕或落在视口外时隐藏跟随准星 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	bool bHideFollowWhenNotOnScreen;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ReticleCenter;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ReticleFollow;
};
