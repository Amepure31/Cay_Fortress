// Fill out your copyright notice in the Description page of Project Settings.


#include "Alex_AnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Alex_PlayerCharacter.h"
#include "Cay_FortressLog.h"
#include "HAL/IConsoleManager.h"
#include "Logging/LogMacros.h"

static TAutoConsoleVariable<float> CVarAlexCombatAnimLogPeriod(
	TEXT("alex.CombatAnimLogPeriod"),
	0.5f,
	TEXT("Interval (seconds) for [Anim] aim/pistol layer diagnostics (LogAlexCombat). 0 = off."),
	ECVF_Default);

/** 0 = 强制关掉手枪 ADS 叠层权重（排查「开镜网格体炸裂」是否来自 ADS+Layer）；1 = 正常 */
static TAutoConsoleVariable<float> CVarAlexDebugAdsLayerMul(
	TEXT("alex.DebugAdsLayerMul"),
	1.f,
	TEXT("Multiplies PistolADSLayerWeight, RifleADSLayerWeight, and AimLayerBlendWeight after interp. Use 0 in PIE to isolate ADS layers."),
	ECVF_Default);

/** 非 0 时 AimOffset 输出恒为 0（区分是 AO 资产还是 Alex_Pistol_Idle_ADS 序列） */
static TAutoConsoleVariable<int32> CVarAlexDebugDisableAimOffset(
	TEXT("alex.DebugDisableAimOffset"),
	0,
	TEXT("If non-zero, AimOffsetPitch/Yaw forced to 0 after smoothing (isolate Aim Offset asset)."),
	ECVF_Default);

/**
 * Aim Offset 输入限幅（度）。默认 -1 表示用代码内建 ±90。
 * 若 AO 资产在网格四角采样很差，可改为 40~60，使游戏内永远走网格中心区域。
 */
static TAutoConsoleVariable<float> CVarAlexAimOffsetPitchClampMax(
	TEXT("alex.AimOffsetPitchClampMax"),
	-1.f,
	TEXT("If >= 0, clamps aim pitch delta to this magnitude (degrees). <0 = default 90."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarAlexAimOffsetYawClampMax(
	TEXT("alex.AimOffsetYawClampMax"),
	-1.f,
	TEXT("If >= 0, clamps aim yaw delta to this magnitude (degrees). <0 = default 90."),
	ECVF_Default);

namespace AlexAnimInstanceCVars
{
	/** Ground speed above this counts as moving for locomotion (blend / ShouldMove). */
	static constexpr float MoveAnimSpeedThreshold = 3.f;

	/** 介于 Walk/Run 速度差之间的比例，高于此阈值认为处于「冲刺档位」，与 CurrentMoveSpeed 插值对齐，避免松手瞬间逻辑跳变 */
	static constexpr float RunThresholdBlendFraction = 0.22f;

	static constexpr float SmoothedGroundSpeedInterpSpeed = 18.f;

	/** Direction 平滑最大角速度（度/秒），减轻急转时 Blend Space 抖动 */
	static constexpr float DirectionSmoothMaxDegPerSec = 720.f;

	/** 瞄准备战混合逼近速度（越大越快到手可出拳） */
	static constexpr float AimUpperBodyBlendInterpSpeed = 10.f;

	/** 瞄准层 Blend Weight 与手枪 ADS 层插值速度 */
	static constexpr float AimLayerBlendInterpSpeed = 14.f;

	/** Aim Offset 数值平滑，减轻镜头抖动 */
	static constexpr float AimOffsetInterpSpeed = 22.f;

	/** Aim Offset 输入限幅（与常见 AO 资产横纵范围一致） */
	static constexpr float AimOffsetPitchClamp = 90.f;
	static constexpr float AimOffsetYawClamp = 90.f;

	/** Equivalent to UKismetAnimationLibrary::CalculateDirection for horizontal velocity (degrees, ~-180..180). */
	static float CalculateLocomotionDirectionDegrees(const FVector& HorizontalVelocity, const FRotator& BaseRotation)
	{
		const FRotationMatrix RotationMatrix(BaseRotation);
		const FVector Forward = RotationMatrix.GetUnitAxis(EAxis::X);
		const FVector Right = RotationMatrix.GetUnitAxis(EAxis::Y);
		const FVector Dir = HorizontalVelocity.GetSafeNormal();
		const float ForwardDot = FVector::DotProduct(Forward, Dir);
		const float RightDot = FVector::DotProduct(Right, Dir);
		return FMath::RadiansToDegrees(FMath::Atan2(RightDot, ForwardDot));
	}
}

// 动画更新事件（每帧调用）
void UAlex_AnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);


	// 获取角色
	ACharacter* Character = GetOwningCharacter();
	if (Character)
	{
		// 获取角色移动组件
		UCharacterMovementComponent* Movement = GetCharacterMovement();
		if (Movement)
		{
			AAlex_PlayerCharacter* PlayerCharacter = Cast<AAlex_PlayerCharacter>(Character);
			if (PlayerCharacter)
			{
				MoveSpeed = PlayerCharacter->GetMoveSpeed();
				RunSpeed = PlayerCharacter->GetRunSpeed();
				CurrentMoveSpeed = PlayerCharacter->GetCurrentMoveSpeed();
			}
			else
			{
				MoveSpeed = 0.f;
				RunSpeed = 0.f;
				CurrentMoveSpeed = Movement->MaxWalkSpeed;
			}

			Velocity = Movement->Velocity;
			GroundSpeed = FMath::Sqrt(Velocity.X * Velocity.X + Velocity.Y * Velocity.Y);

			const float SpeedDenom = FMath::Max(CurrentMoveSpeed, 1.f);
			LocomotionSpeedRatio = FMath::Clamp(GroundSpeed / SpeedDenom, 0.f, 1.f);

			SmoothedGroundSpeed = FMath::FInterpTo(
				SmoothedGroundSpeed,
				GroundSpeed,
				DeltaTime,
				AlexAnimInstanceCVars::SmoothedGroundSpeedInterpSpeed);

			const FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.f);
			float TargetDirection = 0.f;
			if (HorizontalVelocity.SizeSquared() > KINDA_SMALL_NUMBER)
			{
				TargetDirection = AlexAnimInstanceCVars::CalculateLocomotionDirectionDegrees(
					HorizontalVelocity,
					Character->GetActorRotation());
			}
			const float DirDelta = FMath::FindDeltaAngleDegrees(Direction, TargetDirection);
			const float MaxDirStep = AlexAnimInstanceCVars::DirectionSmoothMaxDegPerSec * DeltaTime;
			Direction = FMath::UnwindDegrees(Direction + FMath::Clamp(DirDelta, -MaxDirStep, MaxDirStep));

			const bool bHasMoveIntent =
				Movement->GetCurrentAcceleration().SizeSquared() > KINDA_SMALL_NUMBER;
			ShouldMove = GroundSpeed > AlexAnimInstanceCVars::MoveAnimSpeedThreshold || bHasMoveIntent;

			IsFalling = Movement->IsFalling();
			IsRunning = CheckIsRunning();

			if (PlayerCharacter)
			{
				bIsAiming = PlayerCharacter->IsAiming();
				const float GoalBlend = bIsAiming ? 1.f : 0.f;
				AimMeleeUpperBodyReadyBlend = FMath::FInterpTo(
					AimMeleeUpperBodyReadyBlend,
					GoalBlend,
					DeltaTime,
					AlexAnimInstanceCVars::AimUpperBodyBlendInterpSpeed);
			}
			else
			{
				bIsAiming = false;
				AimMeleeUpperBodyReadyBlend = FMath::FInterpTo(
					AimMeleeUpperBodyReadyBlend,
					0.f,
					DeltaTime,
					AlexAnimInstanceCVars::AimUpperBodyBlendInterpSpeed);
			}

			const bool bPistolEquipped = PlayerCharacter && PlayerCharacter->IsActiveWeaponPistol();
			const bool bRifleStyleEquipped =
				PlayerCharacter && PlayerCharacter->IsActiveWeaponRifleStyleForAdsLayer();
			const bool bMeleeSuppADS = PlayerCharacter && PlayerCharacter->GetPistolADSMeleeSuppressRemain() > KINDA_SMALL_NUMBER;
			const float PistolAdsCap =
				PlayerCharacter ? FMath::Clamp(PlayerCharacter->GetPistolADSLayerBlendCap(), 0.f, 1.f) : 1.f;
			const float RifleAdsCap =
				PlayerCharacter ? FMath::Clamp(PlayerCharacter->GetRifleADSLayerBlendCap(), 0.f, 1.f) : 1.f;

			// Aim Offset：手枪瞄准且未在近战「暂压 ADS」时才算（挥拳时让上半身回到 Slot/Locomotion）
			float TargetAimPitch = 0.f;
			float TargetAimYaw = 0.f;
			if (bIsAiming && bPistolEquipped && !bMeleeSuppADS)
			{
				if (const APlayerController* PC = Cast<APlayerController>(Character->GetController()))
				{
					const FRotator ControlRot = PC->GetControlRotation();
					const FRotator ActorRot = Character->GetActorRotation();
					USkeletalMeshComponent* Skel = Character->GetMesh();

					float BasePitch = ActorRot.Pitch;
					float BaseYaw = ActorRot.Yaw;
					if (PlayerCharacter && Skel)
					{
						if (PlayerCharacter->bAimOffsetPitchUseMeshBase)
						{
							BasePitch = Skel->GetComponentRotation().Pitch;
						}
						if (PlayerCharacter->bAimOffsetYawUsesMeshBase)
						{
							BaseYaw = Skel->GetComponentRotation().Yaw;
						}
					}

					const float DeltaPitch = FMath::FindDeltaAngleDegrees(BasePitch, ControlRot.Pitch);
					const float DeltaYaw = FMath::FindDeltaAngleDegrees(BaseYaw, ControlRot.Yaw);
					const float PitchClamp = CVarAlexAimOffsetPitchClampMax.GetValueOnGameThread() >= 0.f
						? FMath::Clamp(CVarAlexAimOffsetPitchClampMax.GetValueOnGameThread(), 1.f, 89.f)
						: AlexAnimInstanceCVars::AimOffsetPitchClamp;
					const float YawClamp = CVarAlexAimOffsetYawClampMax.GetValueOnGameThread() >= 0.f
						? FMath::Clamp(CVarAlexAimOffsetYawClampMax.GetValueOnGameThread(), 1.f, 89.f)
						: AlexAnimInstanceCVars::AimOffsetYawClamp;
					TargetAimPitch = FMath::Clamp(DeltaPitch, -PitchClamp, PitchClamp);
					TargetAimYaw = FMath::Clamp(DeltaYaw, -YawClamp, YawClamp);
				}
			}
			AimOffsetPitch = FMath::FInterpTo(
				AimOffsetPitch,
				TargetAimPitch,
				DeltaTime,
				AlexAnimInstanceCVars::AimOffsetInterpSpeed);
			AimOffsetYaw = FMath::FInterpTo(
				AimOffsetYaw,
				TargetAimYaw,
				DeltaTime,
				AlexAnimInstanceCVars::AimOffsetInterpSpeed);

			const float PistolLayerGoal =
				(bIsAiming && bPistolEquipped && !bMeleeSuppADS) ? PistolAdsCap : 0.f;
			const float RifleLayerGoal =
				(bIsAiming && bRifleStyleEquipped && !bMeleeSuppADS) ? RifleAdsCap : 0.f;
			AimLayerBlendWeight = FMath::FInterpTo(
				AimLayerBlendWeight,
				PistolLayerGoal,
				DeltaTime,
				AlexAnimInstanceCVars::AimLayerBlendInterpSpeed);
			PistolADSLayerWeight = FMath::FInterpTo(
				PistolADSLayerWeight,
				PistolLayerGoal,
				DeltaTime,
				AlexAnimInstanceCVars::AimLayerBlendInterpSpeed);
			RifleADSLayerWeight = FMath::FInterpTo(
				RifleADSLayerWeight,
				RifleLayerGoal,
				DeltaTime,
				AlexAnimInstanceCVars::AimLayerBlendInterpSpeed);

			const float AdsLayerMul = FMath::Clamp(CVarAlexDebugAdsLayerMul.GetValueOnGameThread(), 0.f, 1.f);
			PistolADSLayerWeight *= AdsLayerMul;
			RifleADSLayerWeight *= AdsLayerMul;
			AimLayerBlendWeight *= AdsLayerMul;

			if (CVarAlexDebugDisableAimOffset.GetValueOnGameThread() != 0)
			{
				AimOffsetPitch = 0.f;
				AimOffsetYaw = 0.f;
			}

			const float AnimDiagPeriod = CVarAlexCombatAnimLogPeriod.GetValueOnAnyThread();
			if (AnimDiagPeriod > 0.f && PlayerCharacter)
			{
				DebugCombatAnimLogTimer += DeltaTime;
				if (DebugCombatAnimLogTimer >= AnimDiagPeriod)
				{
					DebugCombatAnimLogTimer = 0.f;
					if (bIsAiming || PistolADSLayerWeight > 0.02f || RifleADSLayerWeight > 0.02f || AimLayerBlendWeight > 0.02f)
					{
						const FString Msg = FString::Printf(
							TEXT("ANIM layer | %s | Aim=%d Pistol=%d RifleStyle=%d SuppADS=%.2f | PistolCap=%.2f RifleCap=%.2f | PistolW=%.2f RifleW=%.2f AimLayerW=%.2f | Pitch=%.1f Yaw=%.1f | %s"),
							*GetNameSafe(Character),
							bIsAiming ? 1 : 0,
							bPistolEquipped ? 1 : 0,
							bRifleStyleEquipped ? 1 : 0,
							PlayerCharacter ? PlayerCharacter->GetPistolADSMeleeSuppressRemain() : 0.f,
							PistolAdsCap,
							RifleAdsCap,
							PistolADSLayerWeight,
							RifleADSLayerWeight,
							AimLayerBlendWeight,
							AimOffsetPitch,
							AimOffsetYaw,
							*GetNameSafe(GetClass()));
						UE_LOG(LogAlexCombat, Warning, TEXT("%s%s"), CayFortressCombatLog::Prefix(), *Msg);
					}
				}
			}
		}
		else
		{
			// 如果没有移动组件，重置变量
			Velocity = FVector::ZeroVector;
			GroundSpeed = 0.0f;
			Direction = 0.0f;
			ShouldMove = false;
			IsFalling = false;
			IsRunning = false;
			MoveSpeed = 0.0f;
			RunSpeed = 0.0f;
			CurrentMoveSpeed = 0.0f;
			SmoothedGroundSpeed = 0.f;
			LocomotionSpeedRatio = 0.f;
			bIsAiming = false;
			AimMeleeUpperBodyReadyBlend = 0.f;
			AimOffsetPitch = 0.f;
			AimOffsetYaw = 0.f;
			AimLayerBlendWeight = 0.f;
			PistolADSLayerWeight = 0.f;
			RifleADSLayerWeight = 0.f;
		}
	}
	else
	{
		// 如果没有角色，重置变量
		Velocity = FVector::ZeroVector;
		GroundSpeed = 0.0f;
		Direction = 0.0f;
		ShouldMove = false;
		IsFalling = false;
		IsRunning = false;
		MoveSpeed = 0.0f;
		RunSpeed = 0.0f;
		CurrentMoveSpeed = 0.0f;
		SmoothedGroundSpeed = 0.f;
		LocomotionSpeedRatio = 0.f;
		bIsAiming = false;
		AimMeleeUpperBodyReadyBlend = 0.f;
		AimOffsetPitch = 0.f;
		AimOffsetYaw = 0.f;
		AimLayerBlendWeight = 0.f;
		PistolADSLayerWeight = 0.f;
		RifleADSLayerWeight = 0.f;
	}
}

// 获取角色
ACharacter* UAlex_AnimInstance::GetOwningCharacter() const
{
	// 获取拥有者（在动画实例中，应该使用 GetOwningActor() 获取绑定的 Actor）
	AActor* Owner = GetOwningActor();
	if (Owner)
	{
		return Cast<ACharacter>(Owner);
	}
	return nullptr;
}

// 获取角色移动组件
UCharacterMovementComponent* UAlex_AnimInstance::GetCharacterMovement() const
{
	// 获取角色
	ACharacter* Character = GetOwningCharacter();
	if (Character)
	{
		return Character->GetCharacterMovement();
	}
	return nullptr;
}

// 获取玩家控制器
APlayerController* UAlex_AnimInstance::GetPlayerController() const
{
	// 获取角色
	ACharacter* Character = GetOwningCharacter();
	if (Character)
	{
		return Character->GetController<APlayerController>();
	}
	return nullptr;
}

// 是否与 Alex 里 CurrentMoveSpeed 插值一致：按「当前速度上限档位」判冲刺，随 SpeedTransitionTime 落下后再切回步走
bool UAlex_AnimInstance::CheckIsRunning() const
{
	if (GroundSpeed <= 0.1f)
	{
		return false;
	}

	ACharacter* Character = GetOwningCharacter();
	if (!Character)
	{
		return false;
	}

	const AAlex_PlayerCharacter* PlayerCharacter = Cast<AAlex_PlayerCharacter>(Character);
	if (!PlayerCharacter)
	{
		return false;
	}

	if (PlayerCharacter->IsAiming())
	{
		return false;
	}

	const float WalkCap = PlayerCharacter->GetMoveSpeed();
	const float RunCap = PlayerCharacter->GetRunSpeed();
	const float CurrentCap = PlayerCharacter->GetCurrentMoveSpeed();

	if (RunCap <= WalkCap + KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const float SprintBand = RunCap - WalkCap;
	const float RunThreshold =
		WalkCap + SprintBand * AlexAnimInstanceCVars::RunThresholdBlendFraction;
	return CurrentCap > RunThreshold;
}

// Getter 函数
float UAlex_AnimInstance::GetGroundSpeed_Bp() const
{
	return GroundSpeed;
}

FVector UAlex_AnimInstance::GetVelocity_Bp() const
{
	return Velocity;
}

float UAlex_AnimInstance::GetDirection_Bp() const
{
	return Direction;
}

bool UAlex_AnimInstance::GetShouldMove_Bp() const
{
	return ShouldMove;
}

bool UAlex_AnimInstance::GetIsFalling_Bp() const
{
	return IsFalling;
}

bool UAlex_AnimInstance::GetIsRunning_Bp() const
{
	return IsRunning;
}

float UAlex_AnimInstance::GetMoveSpeed_Bp() const
{
	return MoveSpeed;
}

float UAlex_AnimInstance::GetRunSpeed_Bp() const
{
	return RunSpeed;
}

float UAlex_AnimInstance::GetCurrentMoveSpeed_Bp() const
{
	return CurrentMoveSpeed;
}

float UAlex_AnimInstance::GetSmoothedGroundSpeed_Bp() const
{
	return SmoothedGroundSpeed;
}

float UAlex_AnimInstance::GetLocomotionSpeedRatio_Bp() const
{
	return LocomotionSpeedRatio;
}

bool UAlex_AnimInstance::GetIsAiming_Bp() const
{
	return bIsAiming;
}

float UAlex_AnimInstance::GetAimOffsetPitch_Bp() const
{
	return AimOffsetPitch;
}

float UAlex_AnimInstance::GetAimOffsetYaw_Bp() const
{
	return AimOffsetYaw;
}

float UAlex_AnimInstance::GetAimMeleeUpperBodyReadyBlend_Bp() const
{
	return AimMeleeUpperBodyReadyBlend;
}

float UAlex_AnimInstance::GetAimLayerBlendWeight_Bp() const
{
	return AimLayerBlendWeight;
}

float UAlex_AnimInstance::GetPistolADSLayerWeight_Bp() const
{
	return PistolADSLayerWeight;
}

float UAlex_AnimInstance::GetRifleADSLayerWeight_Bp() const
{
	return RifleADSLayerWeight;
}
