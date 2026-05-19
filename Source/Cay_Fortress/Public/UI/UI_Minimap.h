// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/UI_Base_Class.h"
#include "UI_Minimap.generated.h"

class UImage;
class UTextBlock;
class UOverlay;

/** 小地图 Widget：显示俯视渲染目标、玩家箭头、敌人标记。 */
UCLASS()
class CAY_FORTRESS_API UUI_Minimap : public UUI_Base_Class
{
	GENERATED_BODY()

public:
	/** 设置小地图渲染目标纹理 */
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetRenderTarget(UTextureRenderTarget2D* RT);

	/** 切换小地图大小模式 */
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetLargeMode(bool bLarge);

	/** 设置区域名称文本 */
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetAreaName(FText Name);

	/** 更新玩家箭头朝向（Yaw 角度）和偏移（相对地图中心的像素偏移） */
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetPlayerYaw(float Yaw, FVector2D PixelOffset = FVector2D::ZeroVector);

	/** 更新敌人标记：Offsets = 从小地图中心出发的像素偏移，Yaws = 对应朝向 */
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void UpdateEnemyMarkers(const TArray<FVector2D>& Offsets, const TArray<float>& Yaws);

	/** 小地图在 Widget 空间中的半径（像素），由蓝图布局决定 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float MapRadius = 200.f;

	/** 世界单位到像素的缩放：MapRadius / (OrthoWidth/2)，400×400 地图 = 200 / 2500 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float WorldToPixelScale = 0.08f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UImage> MinimapImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UTextBlock> AreaNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UImage> PlayerIndicator;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Widget")
	TObjectPtr<UOverlay> EnemyOverlay;

	/** 敌人标记素材 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	TObjectPtr<UTexture2D> EnemyMarkerTexture;

	bool bLargeMap = false;

private:
	UPROPERTY()
	TArray<TObjectPtr<UImage>> EnemyMarkerPool;

	void EnsureEnemyMarkerPool(int32 Count);
};
