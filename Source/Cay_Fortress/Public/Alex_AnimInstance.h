// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Alex_AnimInstance.generated.h"

class AActor;
class ACharacter;
class APlayerController;
class UCharacterMovementComponent;

/**
 * 动画实例类
 * 处理角色动画状态和参数
 */
UCLASS()
class CAY_FORTRESS_API UAlex_AnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	// 获取角色
	UFUNCTION(BlueprintCallable, Category = "Character")
	ACharacter* GetOwningCharacter() const;

	// 获取角色移动组件
	UFUNCTION(BlueprintCallable, Category = "Character")
	UCharacterMovementComponent* GetCharacterMovement() const;

	// 获取玩家控制器
	UFUNCTION(BlueprintCallable, Category = "Character")
	APlayerController* GetPlayerController() const;

	// Getter 函数
	UFUNCTION(BlueprintCallable, Category = "Character", meta = (BlueprintThreadSafe))
	float GetGroundSpeed_Bp() const;

	UFUNCTION(BlueprintCallable, Category = "Character", meta = (BlueprintThreadSafe))
	FVector GetVelocity_Bp() const;

	UFUNCTION(BlueprintCallable, Category = "Character", meta = (BlueprintThreadSafe))
	float GetDirection_Bp() const;

	UFUNCTION(BlueprintCallable, Category = "Character", meta = (BlueprintThreadSafe))
	bool GetShouldMove_Bp() const;

	UFUNCTION(BlueprintCallable, Category = "Character", meta = (BlueprintThreadSafe))
	bool GetIsFalling_Bp() const;

	UFUNCTION(BlueprintCallable, Category = "Character", meta = (BlueprintThreadSafe))
	bool GetIsRunning_Bp() const;

	UFUNCTION(BlueprintCallable, Category = "Character", meta = (BlueprintThreadSafe))
	float GetMoveSpeed_Bp() const;

	UFUNCTION(BlueprintCallable, Category = "Character", meta = (BlueprintThreadSafe))
	float GetRunSpeed_Bp() const;

	UFUNCTION(BlueprintCallable, Category = "Character", meta = (BlueprintThreadSafe))
	float GetCurrentMoveSpeed_Bp() const;

	UFUNCTION(BlueprintCallable, Category = "Character", meta = (BlueprintThreadSafe))
	float GetSmoothedGroundSpeed_Bp() const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Aim", meta = (BlueprintThreadSafe))
	bool GetIsAiming_Bp() const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Melee", meta = (BlueprintThreadSafe))
	float GetAimMeleeUpperBodyReadyBlend_Bp() const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Aim", meta = (BlueprintThreadSafe))
	float GetAimOffsetPitch_Bp() const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Aim", meta = (BlueprintThreadSafe))
	float GetAimOffsetYaw_Bp() const;

	/** AnimGraph「Layered blend」瞄准层通用权重：随 bIsAiming 0→1 插值，非手枪时也适用（如通用举枪 pose） */
	UFUNCTION(BlueprintCallable, Category = "Combat|Aim", meta = (BlueprintThreadSafe))
	float GetAimLayerBlendWeight_Bp() const;

	/** 手枪 ADS + AimOffset 专用层权重：仅 bIsAiming 且当前武器为手枪时趋近 1，否则 0（避免徒手/步枪误叠 pistol idle） */
	UFUNCTION(BlueprintCallable, Category = "Combat|Aim", meta = (BlueprintThreadSafe))
	float GetPistolADSLayerWeight_Bp() const;

	/** 步枪风格武器 ADS Idle 层权重：瞄准 + 步枪/SMG/机枪且未近战压制时趋近 1；暂不接 Aim Offset */
	UFUNCTION(BlueprintCallable, Category = "Combat|Aim", meta = (BlueprintThreadSafe))
	float GetRifleADSLayerWeight_Bp() const;

	UFUNCTION(BlueprintCallable, Category = "Character", meta = (BlueprintThreadSafe))
	float GetLocomotionSpeedRatio_Bp() const;

	// 动画更新事件（每帧调用）
	virtual void NativeUpdateAnimation(float DeltaTime) override;

	// 公开给蓝图的变量
	UPROPERTY(BlueprintReadOnly, Category = "Character", meta = (Alias = "GroundSpeed_Bp", BlueprintGetter = "GetGroundSpeed_Bp"))
	float GroundSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Character", meta = (Alias = "Velocity_Bp", BlueprintGetter = "GetVelocity_Bp"))
	FVector Velocity;

	UPROPERTY(BlueprintReadOnly, Category = "Character", meta = (Alias = "Direction_Bp", BlueprintGetter = "GetDirection_Bp"))
	float Direction;

	UPROPERTY(BlueprintReadOnly, Category = "Character", meta = (Alias = "ShouldMove_Bp", BlueprintGetter = "GetShouldMove_Bp"))
	bool ShouldMove;

	UPROPERTY(BlueprintReadOnly, Category = "Character", meta = (Alias = "IsFalling_Bp", BlueprintGetter = "GetIsFalling_Bp"))
	bool IsFalling;

	UPROPERTY(BlueprintReadOnly, Category = "Character", meta = (Alias = "IsRunning_Bp", BlueprintGetter = "GetIsRunning_Bp"))
	bool IsRunning;

	UPROPERTY(BlueprintReadOnly, Category = "Character", meta = (Alias = "MoveSpeed_Bp", BlueprintGetter = "GetMoveSpeed_Bp"))
	float MoveSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Character", meta = (Alias = "RunSpeed_Bp", BlueprintGetter = "GetRunSpeed_Bp"))
	float RunSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Character", meta = (Alias = "CurrentMoveSpeed_Bp", BlueprintGetter = "GetCurrentMoveSpeed_Bp"))
	float CurrentMoveSpeed;

	/** 对 GroundSpeed 做插值，Blend Space 纵轴可改用此项减少节拍抖动 */
	UPROPERTY(BlueprintReadOnly, Category = "Character", meta = (Alias = "SmoothedGroundSpeed_Bp", BlueprintGetter = "GetSmoothedGroundSpeed_Bp"))
	float SmoothedGroundSpeed;

	/** 地速 / 当前移动速度上限，0~1，随 C++ 里 MaxWalkSpeed 插值变化，便于 Walk/Jog 混合 */
	UPROPERTY(BlueprintReadOnly, Category = "Character", meta = (Alias = "LocomotionSpeedRatio_Bp", BlueprintGetter = "GetLocomotionSpeedRatio_Bp"))
	float LocomotionSpeedRatio;

	/** 与角色同步：是否在瞄准（上半身 Layer 可用） */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Aim", meta = (Alias = "IsAiming_Bp", BlueprintGetter = "GetIsAiming_Bp"))
	bool bIsAiming;

	/**
	 * 瞄准偏移输入（度）：供 AnimGraph「Aim Offset」。仅当 **瞄准且当前槽为手枪** 时跟随视角，否则平滑回 0。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Aim", meta = (Alias = "AimOffsetPitch_Bp", BlueprintGetter = "GetAimOffsetPitch_Bp"))
	float AimOffsetPitch;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Aim", meta = (Alias = "AimOffsetYaw_Bp", BlueprintGetter = "GetAimOffsetYaw_Bp"))
	float AimOffsetYaw;

	/** 与 PistolADSLayerWeight 同步：仅 **瞄准 + 手枪** 时趋向 1，避免徒手枪 ADS 层权重误混 */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Aim", meta = (Alias = "AimLayerBlendWeight_Bp", BlueprintGetter = "GetAimLayerBlendWeight_Bp"))
	float AimLayerBlendWeight = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Aim", meta = (Alias = "PistolADSLayerWeight_Bp", BlueprintGetter = "GetPistolADSLayerWeight_Bp"))
	float PistolADSLayerWeight = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Aim", meta = (Alias = "RifleADSLayerWeight_Bp", BlueprintGetter = "GetRifleADSLayerWeight_Bp"))
	float RifleADSLayerWeight = 0.f;

	/**
	 * 瞄准时上半身举枪准备度 0~1（C++ 侧插值）；可在 AnimBP 里驱动举枪过渡曲线。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Melee", meta = (Alias = "AimMeleeUpperBodyReadyBlend_Bp", BlueprintGetter = "GetAimMeleeUpperBodyReadyBlend_Bp"))
	float AimMeleeUpperBodyReadyBlend;

private:
	// 检查角色是否正在跑步
	bool CheckIsRunning() const;

	/** alex.CombatAnimLogPeriod：节流输出瞄准层诊断 */
	float DebugCombatAnimLogTimer = 0.f;
};
