// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "Enemy/EnemyBehavior.h"
#include "EnemyAIController.generated.h"

class UAIPerceptionComponent;
class UBehaviorTree;
class UAISenseConfig_Sight;
class AEnemyCharacter;

/**
 * 敌人 AI：行为树 + 黑板同步 + AI 感知（Sight）。
 * 默认加载 /Game/Enemy/Logic/BT_Enemy；可在子类或蓝图里覆盖 BehaviorTree。
 */
UCLASS()
class CAY_FORTRESS_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController(const FObjectInitializer& ObjectInitializer);

	/** 切换行为阶段；同步黑板键 BehaviorState 等。 */
	UFUNCTION(BlueprintCallable, Category = "A | 状态")
	void SetBehaviorState(EEnemyBehavior NewState);

	UFUNCTION(BlueprintPure, Category = "A | 状态")
	EEnemyBehavior GetBehaviorState() const { return CurrentState; }

	UFUNCTION(BlueprintCallable, Category = "A | 状态")
	void SetBehaviorStateByIndex(int32 Index);

	UFUNCTION(BlueprintPure, Category = "A | 状态")
	int32 GetBehaviorStateIndex() const;

	/** 将 CurrentState / Suspicion / 目标等写入黑板（键名与 BB_Enemy 一致）。 */
	UFUNCTION(BlueprintCallable, Category = "A | AI")
	void SyncStateToBlackboard();

	/** 察觉 / 追踪目标：仅接受 AAlex_PlayerCharacter（见感知与 Tick 过滤）。 */
	UPROPERTY(BlueprintReadWrite, Category = "A | 感知")
	TObjectPtr<AActor> FocusTarget = nullptr;

	/** Sight 感知当前是否认为「看见」Alex；失去后 Chase 会切 Alert 并依赖 LastKnown。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "A | 感知")
	bool bPlayerCurrentlyPerceived = false;

	/** 与 Alex 距离 ≤ 此值（cm）且处于 Chase/Alert 时切入 Attack（并同步黑板）。 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 战斗", meta = (ClampMin = "50", ClampMax = "800"))
	float AttackTriggerRange = 220.f;

	/**
	 * 为 true（默认）：离开 Attack 由攻击蒙太奇播完触发，保证整段播完；为 false 时用 AttackPlaceholderDuration 计时结束攻击。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 战斗")
	bool bEndAttackWhenAttackMontageCompletes = true;

	/** bEndAttackWhenAttackMontageCompletes 为 false 时，Attack 状态最长保持时间（秒），避免无蒙太奇时卡死。 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 战斗", meta = (ClampMin = "0.1", ClampMax = "5"))
	float AttackPlaceholderDuration = 0.5f;

	/** 一次攻击结束后多少秒内不再进入 Attack（攻击间隔）。 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 战斗", meta = (ClampMin = "0", ClampMax = "15"))
	float AttackIntervalSeconds = 1.2f;

	/** 攻击蒙太奇播完（或占位超时）后由 AEnemyCharacter / Tick 调用。 */
	void NotifyAttackMontageFinished(bool bInterruptedMontage = false);

	UPROPERTY(BlueprintReadWrite, Category = "A | 警戒", meta = (ClampMin = "0"))
	float Suspicion = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "A | 警戒", meta = (ClampMin = "1"))
	float SuspicionMax = 100.f;

	/** Chase 中失去 Sight 后切入 Alert 时的警戒比例（相对 SuspicionMax），接近满值。 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 警戒", meta = (ClampMin = "0.1", ClampMax = "1"))
	float AlertSuspicionFractionAfterChaseLoss = 0.92f;

	UPROPERTY(BlueprintReadWrite, Category = "A | 追逐")
	FVector LastKnownPlayerLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "A | 追逐")
	bool bHasLastKnownPlayerLocation = false;

	/** 无行为树时用于占位随机漫游；有行为树时请在蓝图关掉或保持 false。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "A | AI|漫游")
	bool bPrototypeRandomRoam = false;

	/** Sight 半径（仅用于默认 UAISenseConfig_Sight）。 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 感知", meta = (ClampMin = "100"))
	float SightRadius = 2400.f;

	/** 水平半角（度）。 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 感知", meta = (ClampMin = "5", ClampMax = "89"))
	float SightHalfAngleDegrees = 80.f;

	UPROPERTY(EditDefaultsOnly, Category = "A | AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;

	/**
	 * Chase 且仍被 Sight 感知时，更新「追玩家」目标点的间隔（秒）。
	 * 同步黑板 LastKnown，并在 bDriveChaseFromController 时触发 MoveTo 重寻路。
	 * 0 表示每帧更新（负载更高）；默认约 0.3s 已较平滑。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 追逐", meta = (ClampMin = "0", ClampMax = "2.0"))
	float ChaseBlackboardRefreshInterval = 0.3f;

	/**
	 * 为 true 时 Chase 由控制器反复 MoveTo 更新路径，途中即可改目标，不会等 BT 里整段 Move To 走完。
	 * 行为树 Chase 分支请用 Wait 等占位，勿再挂 Move To（否则与 C++ 抢路径、仍可能不连贯）。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 追逐")
	bool bDriveChaseFromController = true;

	/** DriveChaseMove 到达判定（cm），略大可减少贴脸抖动。 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 追逐", meta = (ClampMin = "15", ClampMax = "400"))
	float ChaseMoveAcceptanceRadius = 75.f;

	/** 受击硬直期间不发起 Chase MoveTo；结束后由定时器恢复奔跑。 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 战斗", meta = (ClampMin = "0", ClampMax = "3"))
	float HitReactStunSeconds = 0.35f;

	/** 检测挡路门的球形扫描半径（cm）。 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 破门", meta = (ClampMin = "100", ClampMax = "800"))
	float DoorDetectionRadius = 400.f;

	/** 攻击门的距离补偿（在 AttackTriggerRange 基础上叠加，方便门体较远也能命中，cm）。 */
	UPROPERTY(EditDefaultsOnly, Category = "A | 破门", meta = (ClampMin = "0", ClampMax = "400"))
	float DoorAttackRangeAdj = 80.f;

	/** Chase 下当前正在破坏的门目标；非空时会 MoveTo 门并进入 Attack。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "A | 破门")
	TObjectPtr<AActor> CurrentDoorTarget = nullptr;

	/** 攻门回调：在攻击中检测到门已破坏后调用，恢复 Chase。 */
	void NotifyDoorDestroyedDuringAttack();

	/** Alert→Chase 尖叫蒙太奇期间由角色侧设为 true，抑制 DriveChaseMove。 */
	void SetChaseMoveSuppressedForScream(bool bSuppress);

	/** 受击（非致命）：硬直后追向玩家。DamageCauser 用于尝试锁定 Alex。 */
	void NotifyDamagedBy(AActor* DamageCauser);

	/** 强制立即感知玩家（附近敌人被攻击/听到跑动时调用）。 */
	void ForceDetectPlayer(AActor* PlayerActor);

	/** 死亡时停行为树与移动（蒙太奇由 AEnemyCharacter 播放）。 */
	void NotifyDeathBegin();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "A | 感知")
	TObjectPtr<UAIPerceptionComponent> AIPerception;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "A | 状态")
	EEnemyBehavior CurrentState = EEnemyBehavior::Roam;

private:
	void RequestRandomMove();
	void SeedRoamBlackboardTarget();
	void ConfigurePerceptionSenses();
	/** 用 NavMesh 吸附后的点更新 LastKnownPlayerLocation（供黑板 MoveTo）。 */
	void ApplyLastKnownFromActor(AActor* SourceActor);
	void DriveChaseMove();
	void HandlePathBlocked();
	void ClearDoorTarget();

	/** 在玩家最后已知位置附近搜索最近的门，找到则设为 CurrentDoorTarget 返回 true。 */
	bool TryTargetDoorNearPlayer();

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void OnHitReactStunTimer();

	/** 无行为树时 C++ 随机漫游球半径。 */
	UPROPERTY(EditDefaultsOnly, Category = "A | AI|漫游", meta = (ClampMin = "100"))
	float RandomMoveRadius = 2500.f;

	/** 行为树漫游：相对出生锚点的随机点最大水平距离（cm），需在 NavMesh 上。 */
	UPROPERTY(EditDefaultsOnly, Category = "A | AI|漫游", meta = (ClampMin = "200"))
	float RoamWanderRadius = 1800.f;

	UPROPERTY(EditDefaultsOnly, Category = "A | AI|漫游", meta = (ClampMin = "0.05"))
	float RetryDelaySeconds = 0.75f;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> DefaultSightConfig;

	FTimerHandle RetryMoveTimerHandle;

	float ChaseBlackboardRefreshAccumulator = 0.f;

	/** 首次 Possess 时记录，供 Roam 随机点中心使用。 */
	UPROPERTY(Transient)
	FVector RoamSpawnAnchor = FVector::ZeroVector;

	UPROPERTY(Transient)
	bool bRoamSpawnAnchorValid = false;

	/** RunBehaviorTree 后首帧黑板可能尚未就绪，用于限次延迟 Seed。 */
	int32 RoamSeedAttemptCounter = 0;

	float AttackPlaceholderRemain = 0.f;
	float AttackCooldownRemain = 0.f;

	FTimerHandle HitReactStunTimerHandle;
	bool bSuppressChaseMoveForScream = false;
	bool bSuppressChaseMoveForHitReact = false;
};
