// Fill out your copyright notice in the Description page of Project Settings.

#include "Alex_PlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Equipment/EquipmentComponent.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Inventory/InventoryItemType.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Cay_FortressLog.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarAlexCombatLogAttacks(
	TEXT("alex.CombatLogAttacks"),
	1,
	TEXT("If non-zero, log TryAttack / PlayAttackMontage / Aim toggles to LogAlexCombat."),
	ECVF_Default);

AAlex_PlayerCharacter::AAlex_PlayerCharacter()
{
	// 创建背包组件
	Inventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));

	// 创建装备组件
	Equipment = CreateDefaultSubobject<UEquipmentComponent>(TEXT("Equipment"));

	AimPistolAttachSocketName = TEXT("Weapon_R_Socket");
	bShowAimPistolOnlyWhenPistolEquipped = true;
	AimPistolRelativeTransform = FTransform::Identity;

	AimPistolMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AimPistolVisual"));
	if (AimPistolMeshComp)
	{
		AimPistolMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		AimPistolMeshComp->SetGenerateOverlapEvents(false);
		AimPistolMeshComp->SetCanEverAffectNavigation(false);
		AimPistolMeshComp->SetVisibility(false);
		if (USkeletalMeshComponent* SkelMesh = GetMesh())
		{
			AimPistolMeshComp->SetupAttachment(SkelMesh, AimPistolAttachSocketName);
		}
	}

	// 创建弹簧臂组件
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	
	// 设置弹簧臂属性
	CameraBoom->TargetArmLength = 300.0f; // 摄像机距离角色的距离
	CameraBoom->bUsePawnControlRotation = true; // 允许弹簧臂跟随控制器旋转
	CameraBoom->bInheritPitch = false; // 不继承角色的俯仰旋转
	CameraBoom->bInheritRoll = false; // 不继承角色的翻滚旋转
	
	// 创建摄像机组件
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	
	// 设置摄像机属性
	FollowCamera->bUsePawnControlRotation = false; // 摄像机不直接跟随控制器旋转，而是通过弹簧臂
	
	// 设置角色属性
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// 初始化角色属性
	Health = 100.0f;
	MaxHealth = 100.0f;
	Stamina = 100.0f;
	MaxStamina = 100.0f;
	MoveSpeed = 350.0f;
	RunSpeed = 600.0f;
	SpeedTransitionTime = 0.3f;
	TargetMoveSpeed = MoveSpeed;
	CurrentMoveSpeed = MoveSpeed;
	CurrentRunSpeed = RunSpeed;
	bRunInputHeld = false;

	bIsAiming = false;
	ActiveWeaponSlot = EEquipmentSlotType::Weapon1;
	AimCameraArmLength = 180.f;
	AimFieldOfView = 70.f;
	AimMaxMoveSpeed = 200.f;
	AimCameraInterpSpeed = 8.f;
	AimLookSpeedScale = 0.38f;
	bAimOffsetPitchUseMeshBase = false;
	bAimOffsetYawUsesMeshBase = false;
	bApplyControllerDesiredRotationWhileAiming = true;
	bAttackMontagePlaying = false;
	bMeleeMontageDoNotStopAllMontages = true;
	bRangedMontageDoNotStopAllMontages = true;
	PistolADSLayerBlendCap = 1.f;
	MeleePistolADSSuppressSeconds = 0.45f;
	MeleeAttackCooldownSeconds = 0.5f;
	MeleeMontageStartSectionName = NAME_None;
	MeleeMontagePlayRate = 1.45f;
	MeleeMontageLeadInTrimSeconds = 0.f;
	DefaultCameraArmLength = 300.f;
	DefaultFieldOfView = 90.f;
	CurrentCameraArmLengthDisplay = 300.f;
	CurrentFieldOfViewDisplay = 90.f;

	// 设置角色移动组件
	if (GetCharacterMovement())
	{
		// 设置最大加速度
		GetCharacterMovement()->MaxAcceleration = 1000.0f;
		// 设置行走时的减速度
		GetCharacterMovement()->BrakingDecelerationWalking = 400.0f;
		// 设置最大行走速度
		GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
		// 设置最大跑步速度
		GetCharacterMovement()->MaxFlySpeed = RunSpeed;
	}
}

void AAlex_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (CameraBoom)
	{
		DefaultCameraArmLength = CameraBoom->TargetArmLength;
		CurrentCameraArmLengthDisplay = DefaultCameraArmLength;
	}
	if (FollowCamera)
	{
		DefaultFieldOfView = FollowCamera->FieldOfView;
		CurrentFieldOfViewDisplay = DefaultFieldOfView;
	}

	if (Equipment)
	{
		Equipment->OnEquipmentChanged.AddDynamic(this, &AAlex_PlayerCharacter::HandleEquipmentChanged);
	}

	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		SkelMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}

	LogAnimMontageSkeletonMismatches();
	UpdateAimPistolVisual();
}

void AAlex_PlayerCharacter::UpdateAimPistolVisual()
{
	if (!AimPistolMeshComp || !GetMesh())
	{
		return;
	}

	const bool bHasMeshAsset = AimPistolStaticMesh != nullptr;
	const bool bPistolOk = !bShowAimPistolOnlyWhenPistolEquipped || IsActiveWeaponPistol();
	const bool bWantShow = bIsAiming && bHasMeshAsset && bPistolOk;

	if (!bWantShow)
	{
		AimPistolMeshComp->SetVisibility(false);
		AimPistolMeshComp->SetStaticMesh(nullptr);
		return;
	}

	AimPistolMeshComp->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		AimPistolAttachSocketName);
	AimPistolMeshComp->SetRelativeTransform(AimPistolRelativeTransform);
	AimPistolMeshComp->SetStaticMesh(AimPistolStaticMesh);
	AimPistolMeshComp->MarkRenderStateDirty();
	AimPistolMeshComp->SetVisibility(true);
}

void AAlex_PlayerCharacter::LogAnimMontageSkeletonMismatches() const
{
	const USkeletalMeshComponent* SkelComp = GetMesh();
	const USkeletalMesh* SkMesh = SkelComp ? SkelComp->GetSkeletalMeshAsset() : nullptr;
	const USkeleton* MeshSkel = SkMesh ? SkMesh->GetSkeleton() : nullptr;
	if (!MeshSkel)
	{
		return;
	}

	auto CheckMontage = [&](const UAnimMontage* MontageAsset, const TCHAR* Label)
	{
		if (!MontageAsset)
		{
			return;
		}
		const USkeleton* MS = MontageAsset->GetSkeleton();
		if (MS != MeshSkel)
		{
			UE_LOG(LogAlexCombat, Error,
				TEXT("%sSKELETON mismatch | %s montage skel=%s mesh skel=%s"),
				CayFortressCombatLog::Prefix(),
				Label,
				MS ? *MS->GetName() : TEXT("null"),
				*MeshSkel->GetName());
		}
	};

	CheckMontage(UnarmedAttackMontage.Get(), TEXT("UnarmedAttack"));
	CheckMontage(PistolFireMontage.Get(), TEXT("PistolFire"));
	CheckMontage(RifleStyleFireMontage.Get(), TEXT("RifleFire"));
}

float AAlex_PlayerCharacter::GetHealth() const
{
	return Health;
}

float AAlex_PlayerCharacter::GetMaxHealth() const
{
	return MaxHealth;
}

float AAlex_PlayerCharacter::GetHealthPercent() const
{
	return MaxHealth > 0.0f ? (Health / MaxHealth) : 0.0f;
}

void AAlex_PlayerCharacter::SetHealth(float InHealth)
{
	Health = FMath::Clamp(InHealth, 0.0f, MaxHealth);
}

void AAlex_PlayerCharacter::Heal(float Amount)
{
	if (Amount > 0.0f)
	{
		Health = FMath::Clamp(Health + Amount, 0.0f, MaxHealth);
	}
}

float AAlex_PlayerCharacter::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Damage > 0.0f)
	{
		float FinalDamage = Damage;
		if (Equipment)
		{
			const float Reduction = Equipment->GetTotalDamageReduction();
			FinalDamage = Damage * (1.0f - Reduction);
		}
		Health = FMath::Clamp(Health - FinalDamage, 0.0f, MaxHealth);
	}
	return Damage;
}

float AAlex_PlayerCharacter::GetStamina() const
{
	return Stamina;
}

float AAlex_PlayerCharacter::GetMaxStamina() const
{
	return MaxStamina;
}

float AAlex_PlayerCharacter::GetStaminaPercent() const
{
	return MaxStamina > 0.0f ? (Stamina / MaxStamina) : 0.0f;
}

void AAlex_PlayerCharacter::SetStamina(float InStamina)
{
	Stamina = FMath::Clamp(InStamina, 0.0f, MaxStamina);
}

void AAlex_PlayerCharacter::RecoverStamina(float Amount)
{
	if (Amount > 0.0f)
	{
		Stamina = FMath::Clamp(Stamina + Amount, 0.0f, MaxStamina);
	}
}

bool AAlex_PlayerCharacter::ConsumeStamina(float Amount)
{
	if (Amount > 0.0f && Stamina >= Amount)
	{
		Stamina -= Amount;
		return true;
	}
	return false;
}

float AAlex_PlayerCharacter::GetMoveSpeed() const
{
	return MoveSpeed;
}

void AAlex_PlayerCharacter::SetMoveSpeed(float InMoveSpeed)
{
	MoveSpeed = FMath::Clamp(InMoveSpeed, 0.0f, 600.0f);
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	}
}

void AAlex_PlayerCharacter::SetTargetMoveSpeed(float InTargetMoveSpeed)
{
	TargetMoveSpeed = FMath::Clamp(InTargetMoveSpeed, 0.0f, 600.0f);
}

float AAlex_PlayerCharacter::GetTargetMoveSpeed() const
{
	return TargetMoveSpeed;
}

float AAlex_PlayerCharacter::GetCurrentMoveSpeed() const
{
	return CurrentMoveSpeed;
}

void AAlex_PlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (SpeedTransitionTime > 0.0f)
	{
		CurrentMoveSpeed = FMath::FInterpTo(CurrentMoveSpeed, TargetMoveSpeed, DeltaTime, 1.0f / SpeedTransitionTime);
	}
	else
	{
		CurrentMoveSpeed = TargetMoveSpeed;
	}

	ApplyAimMovementConstraints();

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = CurrentMoveSpeed;
	}

	UpdateAimPresentation(DeltaTime);

	if (PistolADSMeleeSuppressRemain > 0.f)
	{
		PistolADSMeleeSuppressRemain = FMath::Max(0.f, PistolADSMeleeSuppressRemain - DeltaTime);
	}
}

float AAlex_PlayerCharacter::GetRunSpeed() const
{
	return RunSpeed;
}

void AAlex_PlayerCharacter::SetRunSpeed(float InRunSpeed)
{
	RunSpeed = FMath::Clamp(InRunSpeed, 0.0f, 600.0f);
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxFlySpeed = RunSpeed;
	}
}

void AAlex_PlayerCharacter::SetRunInputHeld(bool bHeld)
{
	bRunInputHeld = bHeld;
}

void AAlex_PlayerCharacter::SetAiming(bool bWantAim)
{
	if (bIsAiming == bWantAim)
	{
		return;
	}

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();

	if (!bWantAim && bIsAiming)
	{
		if (bCachedMoveRotSettingsFromAim && MoveComp)
		{
			MoveComp->bOrientRotationToMovement = bCachedOrientRotationToMovement;
			MoveComp->bUseControllerDesiredRotation = bCachedUseControllerDesiredRotation;
			bCachedMoveRotSettingsFromAim = false;
		}
	}

	bIsAiming = bWantAim;
	if (bIsAiming)
	{
		TargetMoveSpeed = FMath::Min(TargetMoveSpeed, AimMaxMoveSpeed);
		CurrentMoveSpeed = FMath::Min(CurrentMoveSpeed, AimMaxMoveSpeed);

		if (bApplyControllerDesiredRotationWhileAiming && MoveComp)
		{
			if (!bCachedMoveRotSettingsFromAim)
			{
				bCachedOrientRotationToMovement = MoveComp->bOrientRotationToMovement;
				bCachedUseControllerDesiredRotation = MoveComp->bUseControllerDesiredRotation;
				bCachedMoveRotSettingsFromAim = true;
			}
			MoveComp->bOrientRotationToMovement = false;
			MoveComp->bUseControllerDesiredRotation = true;
		}
	}
	else
	{
		TargetMoveSpeed = bRunInputHeld ? RunSpeed : MoveSpeed;
	}

	if (CVarAlexCombatLogAttacks.GetValueOnGameThread() != 0)
	{
		const FString SlotLabel = StaticEnum<EEquipmentSlotType>()
			? StaticEnum<EEquipmentSlotType>()->GetNameStringByValue(static_cast<int64>(ActiveWeaponSlot))
			: TEXT("?");
		FString WeaponInfo = TEXT("Item=(none)");
		if (Equipment)
		{
			if (const UInventoryItemInstance* WItem = Equipment->GetEquippedItem(ActiveWeaponSlot))
			{
				if (WItem->ItemData)
				{
					const FInventoryItemData& Id = WItem->ItemData->ItemData;
					if (Id.ItemType == EInventoryItemType::Weapon)
					{
						const UEnum* WCEnum = StaticEnum<EWeaponClass>();
						const FString WCName = WCEnum
							? WCEnum->GetNameStringByValue(static_cast<int64>(Id.WeaponStats.WeaponClass))
							: TEXT("?");
						WeaponInfo = FString::Printf(TEXT("WeaponClass=%s (set Pistol on DA for ADS layer)"), *WCName);
					}
					else
					{
						WeaponInfo = TEXT("Slot item not Weapon type");
					}
				}
				else
				{
					WeaponInfo = TEXT("Item missing ItemData");
				}
			}
			else
			{
				WeaponInfo = TEXT("Slot empty");
			}
		}
		else
		{
			WeaponInfo = TEXT("No Equipment component");
		}

		const FString Msg = FString::Printf(
			TEXT("AIM %s | WeaponSlot=%s(%d) PistolEquipped=%d | %s | AnimClass=%s"),
			bIsAiming ? TEXT("ON---") : TEXT("OFF--"),
			*SlotLabel,
			static_cast<int32>(ActiveWeaponSlot),
			IsActiveWeaponPistol() ? 1 : 0,
			*WeaponInfo,
			GetMesh() && GetMesh()->GetAnimClass() ? *GetMesh()->GetAnimClass()->GetName() : TEXT("(none)"));
		UE_LOG(LogAlexCombat, Warning, TEXT("%s%s"), CayFortressCombatLog::Prefix(), *Msg);
		CayFortressCombatLog::NotifyScreen(*Msg, 0.f);
	}

	UpdateAimPistolVisual();
}

void AAlex_PlayerCharacter::UpdateAimPresentation(float DeltaTime)
{
	if (!CameraBoom || !FollowCamera)
	{
		return;
	}

	const float GoalArm = bIsAiming ? AimCameraArmLength : DefaultCameraArmLength;
	const float GoalFov = bIsAiming ? AimFieldOfView : DefaultFieldOfView;
	CurrentCameraArmLengthDisplay = FMath::FInterpTo(
		CurrentCameraArmLengthDisplay,
		GoalArm,
		DeltaTime,
		AimCameraInterpSpeed);
	CurrentFieldOfViewDisplay = FMath::FInterpTo(
		CurrentFieldOfViewDisplay,
		GoalFov,
		DeltaTime,
		AimCameraInterpSpeed);
	CameraBoom->TargetArmLength = CurrentCameraArmLengthDisplay;
	FollowCamera->SetFieldOfView(CurrentFieldOfViewDisplay);
}

void AAlex_PlayerCharacter::ApplyAimMovementConstraints()
{
	if (!bIsAiming)
	{
		return;
	}
	TargetMoveSpeed = FMath::Min(TargetMoveSpeed, AimMaxMoveSpeed);
	CurrentMoveSpeed = FMath::Min(CurrentMoveSpeed, AimMaxMoveSpeed);
}

bool AAlex_PlayerCharacter::IsActiveWeaponPistol() const
{
	if (!Equipment)
	{
		return false;
	}

	const UInventoryItemInstance* Item = Equipment->GetEquippedItem(ActiveWeaponSlot);
	if (!Item || !Item->ItemData)
	{
		return false;
	}

	const FInventoryItemData& Data = Item->ItemData->ItemData;
	if (Data.ItemType != EInventoryItemType::Weapon)
	{
		return false;
	}

	return Data.WeaponStats.WeaponClass == EWeaponClass::Pistol;
}

UAnimMontage* AAlex_PlayerCharacter::SelectRangedAttackMontage() const
{
	if (!Equipment)
	{
		return nullptr;
	}

	UInventoryItemInstance* Item = Equipment->GetEquippedItem(ActiveWeaponSlot);
	if (!Item || !Item->ItemData)
	{
		return nullptr;
	}

	const FInventoryItemData& Data = Item->ItemData->ItemData;
	if (Data.ItemType != EInventoryItemType::Weapon)
	{
		return nullptr;
	}

	switch (Data.WeaponStats.WeaponClass)
	{
	case EWeaponClass::Pistol:
		if (PistolFireMontage)
		{
			return PistolFireMontage.Get();
		}
		return nullptr;
	default:
		break;
	}

	return RifleStyleFireMontage.Get();
}

UAnimMontage* AAlex_PlayerCharacter::SelectMeleeAttackMontage() const
{
	return UnarmedAttackMontage.Get();
}

void AAlex_PlayerCharacter::HealStaleAttackMontageGuard(const bool bLogIfHealed)
{
	if (!bAttackMontagePlaying)
	{
		return;
	}

	USkeletalMeshComponent* SkelMesh = GetMesh();
	UAnimInstance* AnimInst = SkelMesh ? SkelMesh->GetAnimInstance() : nullptr;
	if (!AnimInst || !ActiveAttackMontageGuard || !AnimInst->Montage_IsActive(ActiveAttackMontageGuard))
	{
		if (bLogIfHealed && CVarAlexCombatLogAttacks.GetValueOnGameThread() != 0)
		{
			UE_LOG(LogAlexCombat, Warning,
				TEXT("%sATTACK HEAL stale guard (montage ended without callback / interrupted)"),
				CayFortressCombatLog::Prefix());
		}
		ActiveAttackMontageGuard = nullptr;
		bAttackMontagePlaying = false;
	}
}

void AAlex_PlayerCharacter::HandleAttackMontageBlendOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage && ActiveAttackMontageGuard == Montage)
	{
		ActiveAttackMontageGuard = nullptr;
	}
	if (bInterrupted)
	{
		bAttackMontagePlaying = false;
	}
	if (CVarAlexCombatLogAttacks.GetValueOnGameThread() != 0)
	{
		UE_LOG(LogAlexCombat, Display,
			TEXT("%sMONTAGE BlendOut | %s | interrupted=%d"),
			CayFortressCombatLog::Prefix(),
			*GetNameSafe(Montage),
			bInterrupted ? 1 : 0);
	}
}

bool AAlex_PlayerCharacter::PlayAttackMontage(UAnimMontage* Montage, bool bStopAllMontagesWhenPlaying, bool bIsMeleeAttackMontage)
{
	const bool bDiag = CVarAlexCombatLogAttacks.GetValueOnGameThread() != 0;
	if (bIsMeleeAttackMontage && bIsAiming)
	{
		if (bDiag)
		{
			UE_LOG(LogAlexCombat, Warning,
				TEXT("%sPLAY ABORT | melee blocked while aiming"),
				CayFortressCombatLog::Prefix());
		}
		return false;
	}
	if (!Montage || !GetMesh())
	{
		if (bDiag)
		{
			UE_LOG(LogAlexCombat, Error,
				TEXT("%sPLAY ABORT | MontageValid=%d MeshValid=%d"),
				CayFortressCombatLog::Prefix(),
				Montage != nullptr ? 1 : 0,
				GetMesh() != nullptr ? 1 : 0);
		}
		return false;
	}

	UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
	if (!AnimInst)
	{
		if (bDiag)
		{
			UE_LOG(LogAlexCombat, Error,
				TEXT("%sPLAY ABORT | AnimInstance=null Mesh=%s"),
				CayFortressCombatLog::Prefix(),
				*GetNameSafe(GetMesh()));
		}
		return false;
	}

	const float PlayRate = bIsMeleeAttackMontage ? MeleeMontagePlayRate : 1.f;
	float StartTime = 0.f;
	bool bJumpMeleeSection = false;
	if (bIsMeleeAttackMontage && !MeleeMontageStartSectionName.IsNone()
		&& Montage->GetSectionIndex(MeleeMontageStartSectionName) != INDEX_NONE)
	{
		bJumpMeleeSection = true;
	}
	else if (bIsMeleeAttackMontage && MeleeMontageLeadInTrimSeconds > 0.f)
	{
		const float Len = Montage->GetPlayLength();
		if (Len > KINDA_SMALL_NUMBER)
		{
			StartTime = FMath::Clamp(MeleeMontageLeadInTrimSeconds, 0.f, Len - 0.01f);
		}
	}

	const float PlayLength = AnimInst->Montage_Play(
		Montage,
		PlayRate,
		EMontagePlayReturnType::MontageLength,
		StartTime,
		bStopAllMontagesWhenPlaying);
	if (PlayLength <= 0.f)
	{
		if (bDiag)
		{
			const FString Msg = FString::Printf(
				TEXT("PLAY FAIL Montage_Play<=0 | %s | Skel=%s | Anim=%s | StopAll=%d Melee=%d Rate=%.2f"),
				*GetNameSafe(Montage),
				Montage->GetSkeleton() ? *Montage->GetSkeleton()->GetName() : TEXT("null"),
				*AnimInst->GetClass()->GetName(),
				bStopAllMontagesWhenPlaying ? 1 : 0,
				bIsMeleeAttackMontage ? 1 : 0,
				PlayRate);
			UE_LOG(LogAlexCombat, Error, TEXT("%s%s"), CayFortressCombatLog::Prefix(), *Msg);
			CayFortressCombatLog::NotifyScreen(*Msg, 0.f);
		}
		return false;
	}

	if (bJumpMeleeSection)
	{
		AnimInst->Montage_JumpToSection(MeleeMontageStartSectionName, Montage);
	}

	ActiveAttackMontageGuard = Montage;
	bAttackMontagePlaying = true;

	if (bIsMeleeAttackMontage && MeleePistolADSSuppressSeconds > 0.f)
	{
		PistolADSMeleeSuppressRemain = MeleePistolADSSuppressSeconds;
	}

	if (bIsMeleeAttackMontage && GetWorld())
	{
		LastMeleeAttackTime = GetWorld()->GetTimeSeconds();
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AAlex_PlayerCharacter::HandleAttackMontageEnded);
	AnimInst->Montage_SetEndDelegate(EndDelegate, Montage);

	FOnMontageBlendingOutStarted BlendOutDelegate;
	BlendOutDelegate.BindUObject(this, &AAlex_PlayerCharacter::HandleAttackMontageBlendOut);
	AnimInst->Montage_SetBlendingOutDelegate(BlendOutDelegate, Montage);
	if (bDiag)
	{
		const FString Msg = FString::Printf(
			TEXT("PLAY OK len=%.2f | %s | StopAll=%d Melee=%d Rate=%.2f"),
			PlayLength,
			*GetNameSafe(Montage),
			bStopAllMontagesWhenPlaying ? 1 : 0,
			bIsMeleeAttackMontage ? 1 : 0,
			PlayRate);
		UE_LOG(LogAlexCombat, Display, TEXT("%s%s"), CayFortressCombatLog::Prefix(), *Msg);
	}
	return true;
}

bool AAlex_PlayerCharacter::TryAttack()
{
	const bool bDiag = CVarAlexCombatLogAttacks.GetValueOnGameThread() != 0;
	HealStaleAttackMontageGuard(bDiag);
	if (bAttackMontagePlaying)
	{
		if (bDiag)
		{
			UE_LOG(LogAlexCombat, Warning,
				TEXT("%sATTACK IGNORED (previous montage still marked playing)"),
				CayFortressCombatLog::Prefix());
		}
		return false;
	}

	UAnimMontage* Montage = nullptr;
	bool bStopAllMontages = true;
	bool bPlayAsMeleeMontage = false;

	if (bIsAiming)
	{
		Montage = SelectRangedAttackMontage();
		if (Montage == UnarmedAttackMontage.Get())
		{
			if (bDiag)
			{
				const FString Msg = FString::Printf(
					TEXT("ATTACK ABORT | Aiming: Pistol/Rifle fire montage must not be the same asset as UnarmedAttackMontage"));
				UE_LOG(LogAlexCombat, Warning, TEXT("%s%s"), CayFortressCombatLog::Prefix(), *Msg);
				CayFortressCombatLog::NotifyScreen(*Msg, 0.f);
			}
			return false;
		}
		if (Montage)
		{
			bStopAllMontages = !bRangedMontageDoNotStopAllMontages;
			bPlayAsMeleeMontage = false;
		}
		else
		{
			if (bDiag)
			{
				const FString Msg = FString::Printf(
					TEXT("ATTACK ABORT | Aiming: melee disabled; assign Pistol/Rifle fire montage for attacks while ADS"));
				UE_LOG(LogAlexCombat, Warning, TEXT("%s%s"), CayFortressCombatLog::Prefix(), *Msg);
				CayFortressCombatLog::NotifyScreen(*Msg, 0.f);
			}
			return false;
		}
	}
	else
	{
		if (MeleeAttackCooldownSeconds > KINDA_SMALL_NUMBER && GetWorld())
		{
			const float Now = GetWorld()->GetTimeSeconds();
			if (Now - LastMeleeAttackTime < MeleeAttackCooldownSeconds)
			{
				if (bDiag)
				{
					UE_LOG(LogAlexCombat, Warning,
						TEXT("%sATTACK IGNORED (melee cooldown %.2fs)"),
						CayFortressCombatLog::Prefix(),
						MeleeAttackCooldownSeconds);
				}
				return false;
			}
		}
		Montage = SelectMeleeAttackMontage();
		if (!Montage)
		{
			if (bDiag)
			{
				const FString Msg = FString::Printf(TEXT("ATTACK ABORT | UnarmedAttackMontage not assigned"));
				UE_LOG(LogAlexCombat, Error, TEXT("%s%s"), CayFortressCombatLog::Prefix(), *Msg);
				CayFortressCombatLog::NotifyScreen(*Msg, 0.f);
			}
			return false;
		}
		bStopAllMontages = !bMeleeMontageDoNotStopAllMontages;
		bPlayAsMeleeMontage = true;
	}

	if (bDiag)
	{
		const FString Msg = FString::Printf(
			TEXT("ATTACK pick | %s | Melee=%d StopAll=%d | MeleeNoStopAll=%d RangedNoStopAll=%d | Aiming=%d PistolEq=%d"),
			*GetNameSafe(Montage),
			bPlayAsMeleeMontage ? 1 : 0,
			bStopAllMontages ? 1 : 0,
			bMeleeMontageDoNotStopAllMontages ? 1 : 0,
			bRangedMontageDoNotStopAllMontages ? 1 : 0,
			bIsAiming ? 1 : 0,
			IsActiveWeaponPistol() ? 1 : 0);
		UE_LOG(LogAlexCombat, Warning, TEXT("%s%s"), CayFortressCombatLog::Prefix(), *Msg);
	}

	return PlayAttackMontage(Montage, bStopAllMontages, bPlayAsMeleeMontage);
}

void AAlex_PlayerCharacter::HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage && ActiveAttackMontageGuard == Montage)
	{
		ActiveAttackMontageGuard = nullptr;
	}
	bAttackMontagePlaying = false;
	if (CVarAlexCombatLogAttacks.GetValueOnGameThread() != 0)
	{
		UE_LOG(LogAlexCombat, Display,
			TEXT("%sMONTAGE End | %s | interrupted=%d"),
			CayFortressCombatLog::Prefix(),
			*GetNameSafe(Montage),
			bInterrupted ? 1 : 0);
	}
	OnAttackMontageFinished.Broadcast(bInterrupted);
}

void AAlex_PlayerCharacter::HandleEquipmentChanged(EEquipmentSlotType SlotType, UInventoryItemInstance* NewItem)
{
	if (SlotType == EEquipmentSlotType::Weapon1 || SlotType == EEquipmentSlotType::Weapon2)
	{
		if (CVarAlexCombatLogAttacks.GetValueOnGameThread() != 0)
		{
			const FString Msg = FString::Printf(
				TEXT("EQUIP weapon slot %s(%d) changed -> %s | forcing Aim OFF"),
				StaticEnum<EEquipmentSlotType>()
					? *StaticEnum<EEquipmentSlotType>()->GetNameStringByValue(static_cast<int64>(SlotType))
					: TEXT("?"),
				static_cast<int32>(SlotType),
				NewItem ? *NewItem->GetName() : TEXT("(empty)"));
			UE_LOG(LogAlexCombat, Warning, TEXT("%s%s"), CayFortressCombatLog::Prefix(), *Msg);
			CayFortressCombatLog::NotifyScreen(*Msg, 0.f);
		}
		SetAiming(false);
		PistolADSMeleeSuppressRemain = 0.f;
	}
}
