#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Equipment/EquipmentTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Alex_PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInventoryComponent;
class UEquipmentComponent;
class UAnimMontage;
class UInventoryItemInstance;
class UAIPerceptionStimuliSourceComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAlexAttackMontageFinished, bool, bInterrupted);

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
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

	/** 供 AI Sight 感知注册为刺激源 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception", meta = (AllowPrivateAccess = "true"))
	UAIPerceptionStimuliSourceComponent* AIPerceptionStimuliSource;

	/** 瞄准时挂在角色网格手部插槽上的手枪外观（静态网格）；松开瞄准隐藏 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Aim", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* AimPistolMeshComp;

	/** 瞄准时步枪外观（与手枪互斥显示，逻辑同手枪） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Aim", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* AimRifleMeshComp;

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

	// --- 瞄准（长按进入 / 松开退出，逻辑在 C++，AnimBP 读 bIsAiming） ---

	/** 瞄准时弹簧臂长度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim", meta = (ClampMin = "50.0", UIMin = "50.0"))
	float AimCameraArmLength;

	/** 瞄准时 FOV */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim", meta = (ClampMin = "30.0", ClampMax = "120.0", UIMin = "30.0", UIMax = "120.0"))
	float AimFieldOfView;

	/** 瞄准下移速上限（与奔跑互斥） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim", meta = (ClampMin = "0.0", UIMax = "600.0", UIMin = "0.0"))
	float AimMaxMoveSpeed;

	/** 镜头与 FOV 插值速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float AimCameraInterpSpeed;

	/** 瞄准时视角输入缩放（相对非瞄准） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim", meta = (ClampMin = "0.05", ClampMax = "1.0", UIMin = "0.05", UIMax = "1.0"))
	float AimLookSpeedScale;

	/**
	 * Aim Offset 的 Pitch 基准：true = 用 Mesh 世界 Pitch 与控制器比；false = 用胶囊 Actor Pitch。
	 * Aim Offset 的 **Yaw 默认用胶囊 Actor**（避免 Mannequin Mesh 相对胶囊 -90° 时 Yaw 恒顶 ±90）。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	bool bAimOffsetPitchUseMeshBase;

	/**
	 * 若 true，Aim Offset 的 Yaw 改用语 Mesh 世界 Yaw（旧行为）；一般用不到。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	bool bAimOffsetYawUsesMeshBase;

	/**
	 * 瞄准时让胶囊跟随控制器朝向（关闭「朝运动方向转身」），减少 Actor/Mesh 与镜头不同步时 AO/ADS 极端扭曲。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	bool bApplyControllerDesiredRotationWhileAiming;
	/**
	 * 瞄准叠层最大权重 0~1。若 ADS 序列与骨架不匹配导致上半身被「整块替没」，可先在角色上降到 0.7~0.9 试混合出 Locomotion。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim", meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"))
	float PistolADSLayerBlendCap;

	/**
	 * 步枪风格武器（步枪 / SMG / 机枪）在 AnimBP 里叠 ADS Idle 时的最大权重 0~1；与手枪分开调。
	 * 当前不驱动 Aim Offset，仅提供层权重供 Layered blend。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim", meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"))
	float RifleADSLayerBlendCap;

	/** 瞄准时显示的手枪 StaticMesh（在蓝图或 C++ 子类上指定资源） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	TObjectPtr<UStaticMesh> AimPistolStaticMesh;

	/** 手部挂点名（角色 SkeletalMesh 上需存在该 Socket） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	FName AimPistolAttachSocketName;

	/** 相对 Socket 的网格变换（微调位置/旋转） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	FTransform AimPistolRelativeTransform;

	/** 若为 true，仅当 ActiveWeaponSlot 装备为「手枪」类型时才显示 meshes；徒手瞄准则不显示 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	bool bShowAimPistolOnlyWhenPistolEquipped;

	/** 瞄准时显示的步枪 StaticMesh（与手枪分开指定） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	TObjectPtr<UStaticMesh> AimRifleStaticMesh;

	/** 步枪模型挂点（默认同右手插槽，可改为背枪等 Socket） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	FName AimRifleAttachSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	FTransform AimRifleRelativeTransform;

	/** 若为 true，仅当 ActiveWeaponSlot 为步枪/SMG/机枪（与 Anim 步枪 ADS 层一致）时才显示步枪模型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	bool bShowAimRifleOnlyWhenRifleEquipped;

	/** 当前是否处于瞄准（供 AnimInstance / Layer 混合） */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Aim")
	bool bIsAiming;

	/** 用于远程攻击判定的当前武器槽（步枪/手枪等通过物品数据区分） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
	EEquipmentSlotType ActiveWeaponSlot;

	/** 边跑边近战：近战蒙太奇播放时不停止其它蒙太奇（AnimBP 里近战需用上半身 Slot，下半身保留 Locomotion） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee")
	bool bMeleeMontageDoNotStopAllMontages;

	/**
	 * 瞄准开火蒙太奇默认不 StopAll，避免清空上半身/ADS Layer（与近战选项一致）。
	 * 若你的射击 Montage 必须独占全身，可在蓝图中关闭此项。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged")
	bool bRangedMontageDoNotStopAllMontages;

	/**
	 * 近战蒙太奇若仍含多段（如预备+出手），在此填要**直接切入**的段名（如 Attack）；留空则从首段播放。
	 * 与 Melee Montage Lead In Trim Seconds **二选一**：设了有效段名则忽略裁剪时间。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee")
	FName MeleeMontageStartSectionName;

	/** 近战蒙太奇播放速率（>1 整体加快）；仅影响空手近战 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee", meta = (ClampMin = "0.25", ClampMax = "4.0", UIMin = "0.25", UIMax = "4.0"))
	float MeleeMontagePlayRate;

	/**
	 * 无有效「段名跳转」时，从时间轴跳过开头若干秒（压缩前摇）。单段蒙太奇可在编辑器里对时间试听后填这里。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee", meta = (ClampMin = "0.0", ClampMax = "3.0", UIMin = "0.0", UIMax = "3.0"))
	float MeleeMontageLeadInTrimSeconds;

	/**
	 * 挥拳（近战蒙太奇）播放期间短暂压低手枪/步枪 ADS 叠层权重（AnimInstance），避免 ADS 盖过 UpperBody_Attack。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee", meta = (ClampMin = "0", ClampMax = "2", UIMin = "0", UIMax = "2"))
	float MeleePistolADSSuppressSeconds;

	/** 两次空手近战之间的最短间隔（秒）；防止 BlendOut 提前解锁导致的连点鬼畜 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee", meta = (ClampMin = "0", ClampMax = "3", UIMin = "0", UIMax = "3"))
	float MeleeAttackCooldownSeconds;

	/** 空手 / 近战一拳（不按瞄准键时的默认攻击） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> UnarmedAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> RifleStyleFireMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> PistolFireMontage;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnAlexAttackMontageFinished OnAttackMontageFinished;

protected:
	/** 目标移动速度（用于平滑过渡） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Movement")
	float TargetMoveSpeed;

	/** 当前实际移动速度（用于平滑过渡） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Movement")
	float CurrentMoveSpeed;

	/** 当前跑步速度 */
	float CurrentRunSpeed;

	/** 奔跑键是否仍由玩家按住（用于退出瞄准时恢复走/跑目标速度） */
	bool bRunInputHeld;

	bool bCachedMoveRotSettingsFromAim = false;
	bool bCachedOrientRotationToMovement = false;
	bool bCachedUseControllerDesiredRotation = false;

	float DefaultCameraArmLength;
	float DefaultFieldOfView;
	float CurrentCameraArmLengthDisplay;
	float CurrentFieldOfViewDisplay;

	bool bAttackMontagePlaying;

	float PistolADSMeleeSuppressRemain = 0.f;

	/** 上一次成功播放近战蒙太奇时的世界时间（秒） */
	float LastMeleeAttackTime = -1000.f;

	TObjectPtr<UAnimMontage> ActiveAttackMontageGuard;

	void LogAnimMontageSkeletonMismatches() const;
	void UpdateAimPistolVisual();
	void UpdateAimRifleVisual();
	void UpdateAimWeaponVisuals();
	void UpdateAimPresentation(float DeltaTime);
	void HealStaleAttackMontageGuard(bool bLogIfHealed);
	void HandleAttackMontageBlendOut(UAnimMontage* Montage, bool bInterrupted);
	void ApplyAimMovementConstraints();
	UAnimMontage* SelectRangedAttackMontage() const;
	UAnimMontage* SelectMeleeAttackMontage() const;
	bool PlayAttackMontage(UAnimMontage* Montage, bool bStopAllMontagesWhenPlaying, bool bIsMeleeAttackMontage);
	void HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void HandleEquipmentChanged(EEquipmentSlotType SlotType, UInventoryItemInstance* NewItem);

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

	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	void SetRunInputHeld(bool bHeld);

	UFUNCTION(BlueprintPure, Category = "Character|Movement")
	bool IsRunInputHeld() const { return bRunInputHeld; }

	UFUNCTION(BlueprintCallable, Category = "Combat|Aim")
	void SetAiming(bool bWantAim);

	UFUNCTION(BlueprintPure, Category = "Combat|Aim")
	bool IsAiming() const { return bIsAiming; }

	/** ActiveWeaponSlot 上是否为手枪（用于 AnimBP 手枪 ADS / Aim Offset 层权重） */
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
	bool IsActiveWeaponPistol() const;

	/**
	 * ActiveWeaponSlot 上是否为「步枪风格」武器（步枪 / SMG / 机枪），用于 AnimBP 步枪 ADS Idle 层（无 AO）。
	 * 与手枪互斥：同槽只会命中一种。
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
	bool IsActiveWeaponRifleStyleForAdsLayer() const;

	/** AnimInstance：近战时是否在压制手枪 ADS 层 */
	UFUNCTION(BlueprintPure, Category = "Combat|Melee")
	float GetPistolADSMeleeSuppressRemain() const { return PistolADSMeleeSuppressRemain; }

	UFUNCTION(BlueprintPure, Category = "Combat|Aim")
	float GetPistolADSLayerBlendCap() const { return PistolADSLayerBlendCap; }

	UFUNCTION(BlueprintPure, Category = "Combat|Aim")
	float GetRifleADSLayerBlendCap() const { return RifleADSLayerBlendCap; }

	/** 尝试播放攻击蒙太奇；瞄准时仅允许远程（开火蒙太奇），不播放近战。 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool TryAttack();

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsAttackMontagePlaying() const { return bAttackMontagePlaying; }
};
