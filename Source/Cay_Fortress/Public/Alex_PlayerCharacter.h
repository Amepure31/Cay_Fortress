#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Alex_PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInventoryComponent;
class UEquipmentComponent;

/**
 * 玩家角色类
 * 包含角色的基本属性和功能
 */
UCLASS()
class CAY_FORTRESS_API AAlex_PlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AAlex_PlayerCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// 弹簧臂组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	// 摄像机组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	// 背包组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	UInventoryComponent* Inventory;

	// 装备组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
	UEquipmentComponent* Equipment;

public:
	/** 生命值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (ClampMin = "0", UIMin = "0"))
	float Health;

	/** 最大生命值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (ClampMin = "1", UIMin = "1"))
	float MaxHealth;

	/** 耐力值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (ClampMin = "0", UIMin = "0"))
	float Stamina;

	/** 最大耐力值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (ClampMin = "1", UIMin = "1"))
	float MaxStamina;

	/** 移动速度（0-600） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Movement", meta = (ClampMin = "0", ClampMax = "600", UIMin = "0", UIMax = "600"))
	float MoveSpeed;

	/** 跑步速度（0-600） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Movement", meta = (ClampMin = "0", ClampMax = "600", UIMin = "0", UIMax = "600"))
	float RunSpeed;

	/** 平滑过渡时间（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Movement", meta = (ClampMin = "0.01", ClampMax = "2.0", UIMin = "0.01", UIMax = "2.0"))
	float SpeedTransitionTime;

protected:
	/** 目标移动速度（用于平滑过渡） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Movement")
	float TargetMoveSpeed;

	/** 当前实际移动速度（用于平滑过渡） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Movement")
	float CurrentMoveSpeed;

	/** 当前跑步速度 */
	float CurrentRunSpeed;

public:
	UFUNCTION(BlueprintCallable, Category = "Character|Inventory")
	UInventoryComponent* GetInventory() const { return Inventory; }

	UFUNCTION(BlueprintCallable, Category = "Character|Equipment")
	UEquipmentComponent* GetEquipment() const { return Equipment; }

	/**
	 * 获取当前生命值
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetHealth() const;

	/**
	 * 获取最大生命值
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetMaxHealth() const;

	/**
	 * 获取生命值百分比
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetHealthPercent() const;

	/**
	 * 设置生命值
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	void SetHealth(float InHealth);

	/**
	 * 恢复生命值
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	void Heal(float Amount);

	/**
	 * 受到伤害
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/**
	 * 获取当前耐力值
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetStamina() const;

	/**
	 * 获取最大耐力值
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetMaxStamina() const;

	/**
	 * 获取耐力值百分比
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetStaminaPercent() const;

	/**
	 * 设置耐力值
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	void SetStamina(float InStamina);

	/**
	 * 恢复耐力
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	void RecoverStamina(float Amount);

	/**
	 * 消耗耐力
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	bool ConsumeStamina(float Amount);

	/**
	 * 获取移动速度
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	float GetMoveSpeed() const;

	/**
	 * 设置移动速度
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	void SetMoveSpeed(float InMoveSpeed);

	/**
	 * 设置目标移动速度
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	void SetTargetMoveSpeed(float InTargetMoveSpeed);

	/**
	 * 获取目标移动速度
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	float GetTargetMoveSpeed() const;

	/**
	 * 获取当前实际移动速度
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	float GetCurrentMoveSpeed() const;

	/**
	 * 获取跑步速度
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	float GetRunSpeed() const;

	/**
	 * 设置跑步速度
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	void SetRunSpeed(float InRunSpeed);
};
