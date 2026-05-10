// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Enemy/EnemyBehavior.h"
#include "EnemyCharacter.generated.h"

class UAnimMontage;

/** 敌人身体：生命、受击、死亡与 AI 阶段驱动的蒙太奇（AnimBP 读 UEnemyAnimInstance 状态做 Walk/Idle/Run）。 */
UCLASS()
class CAY_FORTRESS_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintPure, Category = "A | 状态")
	EEnemyBehavior GetVisualAIBehavior() const;

	UFUNCTION(BlueprintPure, Category = "A | 状态")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category = "A | 状态")
	bool IsScreamIntroPlaying() const { return bScreamIntroPlaying; }

	UFUNCTION(BlueprintPure, Category = "A | 状态")
	bool IsHitReactMoveLocked() const { return bHitReactMoveLocked; }

	/** 由 AI 在受击硬直期间设置，供 AnimBP 与移速配合。 */
	void SetHitReactMoveLocked(bool bLocked);

	/** 由 EnemyAIController::SetBehaviorState 调用，驱动蒙太奇与移速。 */
	void NotifyAIBehaviorChanged(EEnemyBehavior Previous, EEnemyBehavior NewState);

	/** 尖叫结束由蒙太奇回调通知 AI 恢复 Chase 寻路。 */
	void NotifyScreamIntroMontageEnded();

	/** 由 AnimNotify_EnemyAttackDamage 调用，对玩家进行近战扫描并造成伤害。 */
	void PerformAttackDamageToPlayer();

	/**
	 * 武器专用 Trace 命中本角色骨骼网格（Physics Asset）时，按 Hit.BoneName 映射伤害倍率。
	 * 需在骨骼网格上挂好 Physics Asset，且网格对 CayFortressWeapon 为 Block。
	 */
	UFUNCTION(BlueprintPure, Category = "A | 战斗")
	float GetWeaponHitDamageMultiplierFromBoneName(FName HitBoneName) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A | 属性", meta = (ClampMin = "0"))
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A | 属性", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "A | 动画")
	TObjectPtr<UAnimMontage> ScreamMontage;

	UPROPERTY(EditDefaultsOnly, Category = "A | 动画")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "A | 动画")
	TObjectPtr<UAnimMontage> PostAttackRoarMontage;

	UPROPERTY(EditDefaultsOnly, Category = "A | 动画")
	TObjectPtr<UAnimMontage> HitReactMontage;

	UPROPERTY(EditDefaultsOnly, Category = "A | 战斗", meta = (ClampMin = "0"))
	float AttackDamageAmount = 15.f;

	/** 攻击门/障碍物时的伤害（默认 50，门 150 血 = 3 次破）。 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 战斗", meta = (ClampMin = "0"))
	float DoorDamageAmount = 50.f;

	UPROPERTY(EditDefaultsOnly, Category = "A | 战斗", meta = (ClampMin = "10", ClampMax = "400"))
	float AttackMeleeRange = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "A | 战斗", meta = (ClampMin = "10", ClampMax = "200"))
	float AttackMeleeSweepRadius = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "A | 动画")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditDefaultsOnly, Category = "A | 移动", meta = (ClampMin = "50"))
	float RoamWalkSpeed = 280.f;

	UPROPERTY(EditDefaultsOnly, Category = "A | 移动", meta = (ClampMin = "50"))
	float AlertWalkSpeed = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "A | 移动", meta = (ClampMin = "50"))
	float ChaseRunSpeed = 520.f;

	UPROPERTY(EditDefaultsOnly, Category = "A | 移动", meta = (ClampMin = "0"))
	float AttackMoveSpeed = 80.f;

	/**
	 * MaxWalkSpeed 向目标值插值的速度系数（FInterpTo 的 InterpSpeed）。
	 * 越大越快贴目标；漫游/警戒/追逐切换时更丝滑。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 移动", meta = (ClampMin = "0.5", ClampMax = "40"))
	float MaxWalkSpeedInterpSpeed = 9.f;

	/** 命中骨骼名含 head/neck 时的倍率 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 战斗|部位倍率", meta = (ClampMin = "0"))
	float WeaponHitDamageMultHead = 2.f;

	/** 命中脊柱、骨盆、髋根等躯干骨骼时的倍率 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 战斗|部位倍率", meta = (ClampMin = "0"))
	float WeaponHitDamageMultTorso = 1.f;

	/** 命中肩、臂、手等时的倍率 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 战斗|部位倍率", meta = (ClampMin = "0"))
	float WeaponHitDamageMultArm = 0.85f;

	/** 命中腿、脚等时的倍率 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 战斗|部位倍率", meta = (ClampMin = "0"))
	float WeaponHitDamageMultLeg = 0.75f;

	/** BoneName 无法归类时的倍率 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 战斗|部位倍率", meta = (ClampMin = "0"))
	float WeaponHitDamageMultDefault = 1.f;

	/** 世界中最多保留的敌人尸体数量；超过则销毁最早登记的一具。 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 死亡", meta = (ClampMin = "1", ClampMax = "64", UIMin = "1", UIMax = "64"))
	int32 MaxEnemyCorpsesInWorld = 15;

	/** 死亡后根胶囊收缩半径（厘米），避免尸体旁仍立着大胶囊碰撞/调试线。 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 死亡", meta = (ClampMin = "0.5", UIMin = "0.5"))
	float CorpseCapsuleRadiusCm = 1.f;

	/** 死亡后根胶囊收缩半高（厘米）。 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 死亡", meta = (ClampMin = "0.5", UIMin = "0.5"))
	float CorpseCapsuleHalfHeightCm = 1.f;

private:
	void HandleDeathSequenceStarted();
	void ApplyMovementSpeedForBehavior(EEnemyBehavior Behavior, bool bDuringScreamHoldInPlace);
	void UpdateSmoothedMaxWalkSpeed(float DeltaSeconds);
	void PlayAttackMontageIfPossible();
	void PlayPostAttackRoar();
	void PlayDeathMontageAndCleanup();
	void ConfigureWeaponTraceCollision();

	UFUNCTION()
	void OnScreamMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnPostAttackRoarMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnDeathMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY(Transient)
	bool bIsDead = false;

	UPROPERTY(Transient)
	bool bScreamIntroPlaying = false;

	UPROPERTY(Transient)
	bool bHitReactMoveLocked = false;

	UPROPERTY(Transient)
	bool bAttackHitSuccessful = false;

	UPROPERTY(Transient)
	bool bAttackHitDoor = false;

	UPROPERTY(Transient)
	bool bPostAttackRoarPlaying = false;

	float LocomotionTargetMaxWalkSpeed = 280.f;

	bool bRegisteredAsCorpse = false;

	void FreezeDeathPoseMesh();
	void DisableRootCapsuleForCorpse();
	void RegisterAsCorpseAndCullOldest();
	static void RemoveCorpseFromGlobalList(const AEnemyCharacter* Corpse);
};
