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

	UFUNCTION(BlueprintPure, Category = "AI|Visual")
	EEnemyBehavior GetVisualAIBehavior() const;

	UFUNCTION(BlueprintPure, Category = "AI|Visual")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category = "AI|Visual")
	bool IsScreamIntroPlaying() const { return bScreamIntroPlaying; }

	UFUNCTION(BlueprintPure, Category = "AI|Visual")
	bool IsHitReactMoveLocked() const { return bHitReactMoveLocked; }

	/** 由 AI 在受击硬直期间设置，供 AnimBP 与移速配合。 */
	void SetHitReactMoveLocked(bool bLocked);

	/** 由 EnemyAIController::SetBehaviorState 调用，驱动蒙太奇与移速。 */
	void NotifyAIBehaviorChanged(EEnemyBehavior Previous, EEnemyBehavior NewState);

	/** 尖叫结束由蒙太奇回调通知 AI 恢复 Chase 寻路。 */
	void NotifyScreamIntroMontageEnded();

	/**
	 * 武器专用 Trace 命中本角色骨骼网格（Physics Asset）时，按 Hit.BoneName 映射伤害倍率。
	 * 需在骨骼网格上挂好 Physics Asset，且网格对 CayFortressWeapon 为 Block。
	 */
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetWeaponHitDamageMultiplierFromBoneName(FName HitBoneName) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (ClampMin = "0"))
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages")
	TObjectPtr<UAnimMontage> ScreamMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages")
	TObjectPtr<UAnimMontage> HitReactMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Locomotion", meta = (ClampMin = "50"))
	float RoamWalkSpeed = 280.f;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Locomotion", meta = (ClampMin = "50"))
	float AlertWalkSpeed = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Locomotion", meta = (ClampMin = "50"))
	float ChaseRunSpeed = 520.f;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Locomotion", meta = (ClampMin = "0"))
	float AttackMoveSpeed = 80.f;

	/**
	 * MaxWalkSpeed 向目标值插值的速度系数（FInterpTo 的 InterpSpeed）。
	 * 越大越快贴目标；漫游/警戒/追逐切换时更丝滑。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Animation|Locomotion", meta = (ClampMin = "0.5", ClampMax = "40"))
	float MaxWalkSpeedInterpSpeed = 9.f;

	/** 命中骨骼名含 head/neck 时的倍率 */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|PhysicsAssetHit", meta = (ClampMin = "0"))
	float WeaponHitDamageMultHead = 2.f;

	/** 命中脊柱、骨盆、髋根等躯干骨骼时的倍率 */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|PhysicsAssetHit", meta = (ClampMin = "0"))
	float WeaponHitDamageMultTorso = 1.f;

	/** 命中肩、臂、手等时的倍率 */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|PhysicsAssetHit", meta = (ClampMin = "0"))
	float WeaponHitDamageMultArm = 0.85f;

	/** 命中腿、脚等时的倍率 */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|PhysicsAssetHit", meta = (ClampMin = "0"))
	float WeaponHitDamageMultLeg = 0.75f;

	/** BoneName 无法归类时的倍率 */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|PhysicsAssetHit", meta = (ClampMin = "0"))
	float WeaponHitDamageMultDefault = 1.f;

	/** 世界中最多保留的敌人尸体数量；超过则销毁最早登记的一具。 */
	UPROPERTY(EditDefaultsOnly, Category = "Character|Death", meta = (ClampMin = "1", ClampMax = "64", UIMin = "1", UIMax = "64"))
	int32 MaxEnemyCorpsesInWorld = 15;

	/** 死亡后根胶囊收缩半径（厘米），避免尸体旁仍立着大胶囊碰撞/调试线。 */
	UPROPERTY(EditDefaultsOnly, Category = "Character|Death", meta = (ClampMin = "0.5", UIMin = "0.5"))
	float CorpseCapsuleRadiusCm = 1.f;

	/** 死亡后根胶囊收缩半高（厘米）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Character|Death", meta = (ClampMin = "0.5", UIMin = "0.5"))
	float CorpseCapsuleHalfHeightCm = 1.f;

private:
	void HandleDeathSequenceStarted();
	void ApplyMovementSpeedForBehavior(EEnemyBehavior Behavior, bool bDuringScreamHoldInPlace);
	void UpdateSmoothedMaxWalkSpeed(float DeltaSeconds);
	void PlayAttackMontageIfPossible();
	void PlayDeathMontageAndCleanup();
	void ConfigureWeaponTraceCollision();

	UFUNCTION()
	void OnScreamMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

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

	float LocomotionTargetMaxWalkSpeed = 280.f;

	bool bRegisteredAsCorpse = false;

	void FreezeDeathPoseMesh();
	void DisableRootCapsuleForCorpse();
	void RegisterAsCorpseAndCullOldest();
	static void RemoveCorpseFromGlobalList(const AEnemyCharacter* Corpse);
};
