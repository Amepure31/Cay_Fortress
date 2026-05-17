// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MinimapCaptureActor.generated.h"

class USceneComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

/** 小地图俯视捕获 Actor：挂载 SceneCapture2D，自动跟随玩家，渲染到 RenderTarget。 */
UCLASS(BlueprintType, Blueprintable)
class CAY_FORTRESS_API AMinimapCaptureActor : public AActor
{
	GENERATED_BODY()

public:
	AMinimapCaptureActor();

	/** 获取小地图渲染目标 */
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	UTextureRenderTarget2D* GetMinimapRenderTarget() const { return MinimapRenderTarget; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneCaptureComponent2D> SceneCapture;

	/** 编辑器配置的渲染目标（须在蓝图或实例上设置） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	TObjectPtr<UTextureRenderTarget2D> MinimapRenderTarget;

	/** 捕获高度（Z 轴距离地面） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float CaptureHeight = 2000.f;

	/** 正交投影宽度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float OrthoWidth = 5000.f;

	/** 跟随平滑速度（越大越快），0 = 无延迟瞬间跟随 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float FollowSpeed = 10.f;

private:
	TWeakObjectPtr<AActor> FollowTarget;
	FTimerHandle CaptureTimerHandle;
};
