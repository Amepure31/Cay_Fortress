// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Enemy/EnemyBehavior.h"
#include "EnemyAnimInstance.generated.h"

/**
 * 敌人动画实例：地面速度 + AI 可视状态（供 AnimBP 选 Roam Walk / Alert Idle / Chase Run 等）。
 * 蒙太奇（尖叫、攻击、受击、死亡）由 AEnemyCharacter 在槽位播放，可在 AnimGraph 用 Slot 叠层。
 */
UCLASS()
class CAY_FORTRESS_API UEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed = 0.f;

	/** 对 GroundSpeed 插值，Blend Space 横轴用这个可减少急转弯时步频抖动。 */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float SmoothedGroundSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsFalling = false;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Visual")
	EEnemyBehavior AIBehavior = EEnemyBehavior::Roam;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Visual")
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Visual")
	bool bScreamIntroPlaying = false;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Visual")
	bool bHitReactMoveLocked = false;

	/** SmoothedGroundSpeed 逼近 GroundSpeed 的速度（越大越快跟上真实地速）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "1", ClampMax = "60"))
	float GroundSpeedSmoothing = 16.f;
};
