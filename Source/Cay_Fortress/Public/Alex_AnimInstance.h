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
protected:
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

private:
	// 检查角色是否正在跑步
	bool CheckIsRunning() const;

	UFUNCTION(BlueprintCallable, Category = "Character", meta = (BlueprintThreadSafe))
	float GetCurrentMoveSpeed_Bp() const;
};
