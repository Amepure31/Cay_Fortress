#include "Alex_PlayerCharacter.h"
#include "CayFortressCollisionChannels.h"
#include "Alex_PlayerController.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/DamageType.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Equipment/EquipmentComponent.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Inventory/InventoryItemType.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Cay_FortressLog.h"
#include "HAL/IConsoleManager.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "Engine/HitResult.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Enemy/EnemyCharacter.h"

namespace
{
static void AlexMergeCloserHit(const FVector& TraceStart, FHitResult& Best, const FHitResult& Candidate)
{
	if (!Candidate.bBlockingHit) return;
	if (!Best.bBlockingHit)
	{
		Best = Candidate;
		return;
	}
	const float Db = FVector::DistSquared(TraceStart, Best.ImpactPoint);
	const float Dc = FVector::DistSquared(TraceStart, Candidate.ImpactPoint);
	if (Dc < Db) Best = Candidate;
}
} // namespace

static TAutoConsoleVariable<int32> CVarAlexCombatLogAttacks(
	TEXT("alex.CombatLogAttacks"),
	1,
	TEXT("If non-zero, log TryAttack / PlayAttackMontage / Aim toggles to LogAlexCombat."),
	ECVF_Default);

AAlex_PlayerCharacter::AAlex_PlayerCharacter()
{
	Inventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
	Equipment = CreateDefaultSubobject<UEquipmentComponent>(TEXT("Equipment"));

	AIPerceptionStimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("AIPerceptionStimuliSource"));
	if (AIPerceptionStimuliSource)
	{
		AIPerceptionStimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
	}

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

	AimRifleAttachSocketName = TEXT("Weapon_R_Socket");
	bShowAimRifleOnlyWhenRifleEquipped = true;
	AimRifleRelativeTransform = FTransform::Identity;

	AimRifleMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AimRifleVisual"));
	if (AimRifleMeshComp)
	{
		AimRifleMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		AimRifleMeshComp->SetGenerateOverlapEvents(false);
		AimRifleMeshComp->SetCanEverAffectNavigation(false);
		AimRifleMeshComp->SetVisibility(false);
		if (USkeletalMeshComponent* SkelMesh = GetMesh())
		{
			AimRifleMeshComp->SetupAttachment(SkelMesh, AimRifleAttachSocketName);
		}
	}

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = 8.f;
	CameraBoom->ProbeChannel = ECC_Camera;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// 防止弹簧臂碰撞检测打到自己胶囊体导致持续缩回
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	Health = 100.0f;
	MaxHealth = 100.0f;
	Stamina = 100.0f;
	MaxStamina = 100.0f;
	Hunger = 100.0f;
	MaxHunger = 100.0f;
	HungerDrainRate = 0.1f;
	Hydration = 100.0f;
	MaxHydration = 100.0f;
	HydrationDrainRate = 0.15f;
	MaxCarryWeight = 50.0f;
	MoveSpeed = 350.0f;
	RunSpeed = 600.0f;
	SpeedTransitionTime = 0.3f;
	TargetMoveSpeed = MoveSpeed;
	CurrentMoveSpeed = MoveSpeed;
	CurrentRunSpeed = RunSpeed;
	bRunInputHeld = false;

	bIsAiming = false;
	AimWeaponMuzzleSocketName = TEXT("Muzzle");
	AimUiTraceDefaultRangeMeters = 100.f;
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
	bReloadMontageDoNotStopAllMontages = true;
	ReloadMontagePlayRate = 1.f;
	bReloadRequiresAiming = false;
	PistolADSLayerBlendCap = 1.f;
	RifleADSLayerBlendCap = 1.f;
	MeleePistolADSSuppressSeconds = 0.45f;
	MeleeAttackCooldownSeconds = 0.5f;
	MeleeMontageStartSectionName = NAME_None;
	MeleeMontagePlayRate = 1.45f;
	MeleeMontageLeadInTrimSeconds = 0.f;
	DefaultCameraArmLength = 300.f;
	DefaultFieldOfView = 90.f;
	CurrentCameraArmLengthDisplay = 300.f;
	CurrentFieldOfViewDisplay = 90.f;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxAcceleration = 1000.0f;
		GetCharacterMovement()->BrakingDecelerationWalking = 400.0f;
		GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
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
	UpdateAimWeaponVisuals();
}

void AAlex_PlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Equipment)
	{
		Equipment->OnEquipmentChanged.RemoveDynamic(this, &AAlex_PlayerCharacter::HandleEquipmentChanged);
	}

	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		if (UAnimInstance* AnimInst = SkelMesh->GetAnimInstance())
		{
			if (ActiveAttackMontageGuard)
			{
				FOnMontageEnded EmptyEndDelegate;
				FOnMontageBlendingOutStarted EmptyBlendDelegate;
				AnimInst->Montage_SetEndDelegate(EmptyEndDelegate, ActiveAttackMontageGuard.Get());
				AnimInst->Montage_SetBlendingOutDelegate(EmptyBlendDelegate, ActiveAttackMontageGuard.Get());
			}
			if (ActiveReloadMontageGuard)
			{
				FOnMontageEnded EmptyEndDelegate;
				FOnMontageBlendingOutStarted EmptyBlendDelegate;
				AnimInst->Montage_SetEndDelegate(EmptyEndDelegate, ActiveReloadMontageGuard.Get());
				AnimInst->Montage_SetBlendingOutDelegate(EmptyBlendDelegate, ActiveReloadMontageGuard.Get());
			}
		}
	}

	ActiveAttackMontageGuard = nullptr;
	bAttackMontagePlaying = false;
	ActiveReloadMontageGuard = nullptr;
	bReloadMontagePlaying = false;
	OnAttackMontageFinished.Clear();
	OnReloadMontageFinished.Clear();
	OnHealthChanged.Clear();
	OnStaminaChanged.Clear();
	OnHungerChanged.Clear();
	OnHydrationChanged.Clear();
	OnCarryWeightChanged.Clear();

	Super::EndPlay(EndPlayReason);
}

void AAlex_PlayerCharacter::UpdateAimPistolVisual()
{
	if (!AimPistolMeshComp || !GetMesh()) return;

	const bool bHasMeshAsset = AimPistolStaticMesh != nullptr;
	const bool bPistolOk = !bShowAimPistolOnlyWhenPistolEquipped || IsActiveWeaponPistol();
	const bool bWantShow = (bIsAiming || bReloadMontagePlaying) && bHasMeshAsset && bPistolOk;

	if (!bWantShow)
	{
		AimPistolMeshComp->SetVisibility(false);
		AimPistolMeshComp->SetStaticMesh(nullptr);
		return;
	}

	AimPistolMeshComp->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, AimPistolAttachSocketName);
	AimPistolMeshComp->SetRelativeTransform(AimPistolRelativeTransform);
	AimPistolMeshComp->SetStaticMesh(AimPistolStaticMesh);
	AimPistolMeshComp->MarkRenderStateDirty();
	AimPistolMeshComp->SetVisibility(true);
}

void AAlex_PlayerCharacter::UpdateAimRifleVisual()
{
	if (!AimRifleMeshComp || !GetMesh()) return;

	const bool bHasMeshAsset = AimRifleStaticMesh != nullptr;
	const bool bRifleOk = !bShowAimRifleOnlyWhenRifleEquipped || IsActiveWeaponRifleStyleForAdsLayer();
	const bool bWantShow = (bIsAiming || bReloadMontagePlaying) && bHasMeshAsset && bRifleOk;

	if (!bWantShow)
	{
		AimRifleMeshComp->SetVisibility(false);
		AimRifleMeshComp->SetStaticMesh(nullptr);
		return;
	}

	AimRifleMeshComp->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, AimRifleAttachSocketName);
	AimRifleMeshComp->SetRelativeTransform(AimRifleRelativeTransform);
	AimRifleMeshComp->SetStaticMesh(AimRifleStaticMesh);
	AimRifleMeshComp->MarkRenderStateDirty();
	AimRifleMeshComp->SetVisibility(true);
}

void AAlex_PlayerCharacter::UpdateAimWeaponVisuals()
{
	UpdateAimPistolVisual();
	UpdateAimRifleVisual();
}

void AAlex_PlayerCharacter::LogAnimMontageSkeletonMismatches() const
{
	const USkeletalMeshComponent* SkelComp = GetMesh();
	const USkeletalMesh* SkMesh = SkelComp ? SkelComp->GetSkeletalMeshAsset() : nullptr;
	const USkeleton* MeshSkel = SkMesh ? SkMesh->GetSkeleton() : nullptr;
	if (!MeshSkel) return;

	auto CheckMontage = [&](const UAnimMontage* MontageAsset, const TCHAR* Label)
	{
		if (!MontageAsset) return;
		const USkeleton* MS = MontageAsset->GetSkeleton();
		if (MS != MeshSkel)
		{
			UE_LOG(LogAlexCombat, Error,
				TEXT("%sSKELETON mismatch | %s montage skel=%s mesh skel=%s"),
				CayFortressCombatLog::Prefix(), Label,
				MS ? *MS->GetName() : TEXT("null"), *MeshSkel->GetName());
		}
	};

	CheckMontage(UnarmedAttackMontage.Get(), TEXT("UnarmedAttack"));
	CheckMontage(PistolFireMontage.Get(), TEXT("PistolFire"));
	CheckMontage(RifleStyleFireMontage.Get(), TEXT("RifleFire"));
	CheckMontage(PistolReloadMontage.Get(), TEXT("PistolReload"));
	CheckMontage(RifleStyleReloadMontage.Get(), TEXT("RifleReload"));
}

UInventoryComponent* AAlex_PlayerCharacter::GetInventory() const
{
	if (Inventory)
	{
		return Inventory;
	}

	AAlex_PlayerCharacter* MutableThis = const_cast<AAlex_PlayerCharacter*>(this);

	UInventoryComponent* Found = FindComponentByClass<UInventoryComponent>();
	if (Found)
	{
		MutableThis->Inventory = Found;
		return Found;
	}

	// Last resort: the Blueprint removed the Inventory component. Re-create it at runtime.
	UInventoryComponent* NewInv = NewObject<UInventoryComponent>(MutableThis, TEXT("Inventory"));
	if (NewInv)
	{
		NewInv->RegisterComponent();
		NewInv->InitializeGrid(6, 10);
		MutableThis->Inventory = NewInv;
	}
	return MutableThis->Inventory;
}

float AAlex_PlayerCharacter::GetHealth() const { return Health; }
float AAlex_PlayerCharacter::GetMaxHealth() const { return MaxHealth; }
float AAlex_PlayerCharacter::GetHealthPercent() const { return MaxHealth > 0.0f ? (Health / MaxHealth) : 0.0f; }

void AAlex_PlayerCharacter::SetHealth(float InHealth)
{
	const float Old = Health;
	Health = FMath::Clamp(InHealth, 0.0f, MaxHealth);
	if (!FMath::IsNearlyEqual(Old, Health))
		OnHealthChanged.Broadcast(Health);
}

void AAlex_PlayerCharacter::Heal(float Amount)
{
	if (Amount > 0.0f) SetHealth(Health + Amount);
}

float AAlex_PlayerCharacter::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Damage > 0.0f)
	{
		const float ArmorVal = Equipment ? Equipment->GetTotalArmorValue() : 0.0f;
		const float DamageReduction = Equipment ? Equipment->GetTotalDamageReduction() : 0.0f;
		const float ArmorPct = (ArmorVal > KINDA_SMALL_NUMBER) ? (ArmorVal / (ArmorVal + 100.0f)) : 0.0f;
		const float TotalReduction = FMath::Min(ArmorPct + DamageReduction, 0.9f);
		const float FinalDamage = Damage * (1.0f - TotalReduction);
		SetHealth(Health - FinalDamage);
	}
	return Damage;
}

float AAlex_PlayerCharacter::GetStamina() const { return Stamina; }
float AAlex_PlayerCharacter::GetMaxStamina() const { return MaxStamina; }
float AAlex_PlayerCharacter::GetStaminaPercent() const { return MaxStamina > 0.0f ? (Stamina / MaxStamina) : 0.0f; }

void AAlex_PlayerCharacter::SetStamina(float InStamina)
{
	const float Old = Stamina;
	Stamina = FMath::Clamp(InStamina, 0.0f, MaxStamina);
	if (!FMath::IsNearlyEqual(Old, Stamina))
		OnStaminaChanged.Broadcast(Stamina);
}

void AAlex_PlayerCharacter::RecoverStamina(float Amount)
{
	if (Amount > 0.0f) SetStamina(Stamina + Amount);
}

bool AAlex_PlayerCharacter::ConsumeStamina(float Amount)
{
	if (Amount > 0.0f && Stamina >= Amount)
	{
		SetStamina(Stamina - Amount);
		return true;
	}
	return false;
}

// --- Hunger ---
float AAlex_PlayerCharacter::GetHunger() const { return Hunger; }
float AAlex_PlayerCharacter::GetMaxHunger() const { return MaxHunger; }
float AAlex_PlayerCharacter::GetHungerPercent() const { return MaxHunger > 0.0f ? (Hunger / MaxHunger) : 0.0f; }

void AAlex_PlayerCharacter::SetHunger(float InHunger)
{
	const float Old = Hunger;
	Hunger = FMath::Clamp(InHunger, 0.0f, MaxHunger);
	if (!FMath::IsNearlyEqual(Old, Hunger))
		OnHungerChanged.Broadcast(Hunger);
}

void AAlex_PlayerCharacter::RestoreHunger(float Amount)
{
	if (Amount > 0.0f) SetHunger(Hunger + Amount);
}

bool AAlex_PlayerCharacter::ConsumeHunger(float Amount)
{
	if (Amount > 0.0f && Hunger >= Amount)
	{
		SetHunger(Hunger - Amount);
		return true;
	}
	return false;
}

// --- Hydration ---
float AAlex_PlayerCharacter::GetHydration() const { return Hydration; }
float AAlex_PlayerCharacter::GetMaxHydration() const { return MaxHydration; }
float AAlex_PlayerCharacter::GetHydrationPercent() const { return MaxHydration > 0.0f ? (Hydration / MaxHydration) : 0.0f; }

void AAlex_PlayerCharacter::SetHydration(float InHydration)
{
	const float Old = Hydration;
	Hydration = FMath::Clamp(InHydration, 0.0f, MaxHydration);
	if (!FMath::IsNearlyEqual(Old, Hydration))
		OnHydrationChanged.Broadcast(Hydration);
}

void AAlex_PlayerCharacter::RestoreHydration(float Amount)
{
	if (Amount > 0.0f) SetHydration(Hydration + Amount);
}

bool AAlex_PlayerCharacter::ConsumeHydration(float Amount)
{
	if (Amount > 0.0f && Hydration >= Amount)
	{
		SetHydration(Hydration - Amount);
		return true;
	}
	return false;
}

// --- Damage Reduction / Armor (pass-through) ---
float AAlex_PlayerCharacter::GetTotalDamageReduction() const
{
	return Equipment ? Equipment->GetTotalDamageReduction() : 0.0f;
}

float AAlex_PlayerCharacter::GetTotalArmorValue() const
{
	return Equipment ? Equipment->GetTotalArmorValue() : 0.0f;
}

// --- Carry Weight ---
float AAlex_PlayerCharacter::GetMaxCarryWeight() const { return MaxCarryWeight; }

float AAlex_PlayerCharacter::GetCurrentCarriedWeight() const
{
	const UInventoryComponent* Inv = GetInventory();
	const float BagWeight = Inv ? Inv->GetTotalCarriedWeight() : 0.0f;
	const float EquippedWeight = Equipment ? Equipment->GetTotalEquippedWeight() : 0.0f;
	return BagWeight + EquippedWeight;
}

float AAlex_PlayerCharacter::GetCarryWeightPercent() const
{
	if (MaxCarryWeight <= 0.0f) return 0.0f;
	return GetCurrentCarriedWeight() / MaxCarryWeight;
}

void AAlex_PlayerCharacter::SetMaxCarryWeight(float InMaxCarryWeight)
{
	MaxCarryWeight = FMath::Max(0.0f, InMaxCarryWeight);
}

float AAlex_PlayerCharacter::GetMoveSpeed() const { return MoveSpeed; }
void AAlex_PlayerCharacter::SetMoveSpeed(float InMoveSpeed) { MoveSpeed = FMath::Clamp(InMoveSpeed, 0.0f, 600.0f); if (GetCharacterMovement()) GetCharacterMovement()->MaxWalkSpeed = MoveSpeed; }
void AAlex_PlayerCharacter::SetTargetMoveSpeed(float InTargetMoveSpeed) { TargetMoveSpeed = FMath::Clamp(InTargetMoveSpeed, 0.0f, 600.0f); }
float AAlex_PlayerCharacter::GetTargetMoveSpeed() const { return TargetMoveSpeed; }
float AAlex_PlayerCharacter::GetCurrentMoveSpeed() const { return CurrentMoveSpeed; }

void AAlex_PlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (SpeedTransitionTime > 0.0f)
		CurrentMoveSpeed = FMath::FInterpTo(CurrentMoveSpeed, TargetMoveSpeed, DeltaTime, 1.0f / SpeedTransitionTime);
	else
		CurrentMoveSpeed = TargetMoveSpeed;

	ApplyAimMovementConstraints();
	TickStatDrainAndEncumbrance(DeltaTime);
	if (GetCharacterMovement()) GetCharacterMovement()->MaxWalkSpeed = CurrentMoveSpeed;

	UpdateAimPresentation(DeltaTime);
	TickRangedFullAuto(DeltaTime);

	if (PistolADSMeleeSuppressRemain > 0.f)
		PistolADSMeleeSuppressRemain = FMath::Max(0.f, PistolADSMeleeSuppressRemain - DeltaTime);
}

float AAlex_PlayerCharacter::GetRunSpeed() const { return RunSpeed; }
void AAlex_PlayerCharacter::SetRunSpeed(float InRunSpeed) { RunSpeed = FMath::Clamp(InRunSpeed, 0.0f, 600.0f); if (GetCharacterMovement()) GetCharacterMovement()->MaxFlySpeed = RunSpeed; }
void AAlex_PlayerCharacter::SetRunInputHeld(bool bHeld) { bRunInputHeld = bHeld; }

void AAlex_PlayerCharacter::SetAiming(bool bWantAim)
{
	if (bIsAiming == bWantAim) return;

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
			? StaticEnum<EEquipmentSlotType>()->GetNameStringByValue(static_cast<int64>(ActiveWeaponSlot)) : TEXT("?");
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
						const FString WCName = WCEnum ? WCEnum->GetNameStringByValue(static_cast<int64>(Id.WeaponStats.WeaponClass)) : TEXT("?");
						WeaponInfo = FString::Printf(TEXT("WeaponClass=%s (set Pistol on DA for ADS layer)"), *WCName);
					}
					else WeaponInfo = TEXT("Slot item not Weapon type");
				}
				else WeaponInfo = TEXT("Item missing ItemData");
			}
			else WeaponInfo = TEXT("Slot empty");
		}
		else WeaponInfo = TEXT("No Equipment component");

		const FString Msg = FString::Printf(TEXT("AIM %s | WeaponSlot=%s(%d) PistolEquipped=%d | %s | AnimClass=%s"),
			bIsAiming ? TEXT("ON---") : TEXT("OFF--"), *SlotLabel, static_cast<int32>(ActiveWeaponSlot),
			IsActiveWeaponPistol() ? 1 : 0, *WeaponInfo,
			GetMesh() && GetMesh()->GetAnimClass() ? *GetMesh()->GetAnimClass()->GetName() : TEXT("(none)"));
		UE_LOG(LogAlexCombat, Warning, TEXT("%s%s"), CayFortressCombatLog::Prefix(), *Msg);
		CayFortressCombatLog::NotifyScreen(*Msg, 0.f);
	}

	UpdateAimWeaponVisuals();
}

void AAlex_PlayerCharacter::SetPrimaryFireHeld(const bool bHeld)
{
	bPrimaryFireHeld = bHeld;
	if (!bHeld) RangedAutoFireAccum = 0.f;
}

bool AAlex_PlayerCharacter::IsAdsRangedFullAutoEnabled() const
{
	if (!bIsAiming || !IsActiveWeaponRifleStyleForAdsLayer()) return false;
	if (!Equipment) return false;
	const UInventoryItemInstance* Item = Equipment->GetEquippedItem(ActiveWeaponSlot);
	if (!Item || !Item->ItemData) return false;
	const FInventoryItemData& Data = Item->ItemData->ItemData;
	if (Data.ItemType != EInventoryItemType::Weapon) return false;
	if (bRifleStyleAdsFullAutoByDefault) return true;
	return Data.WeaponStats.FireMode == EWeaponFireMode::FullAuto;
}

float AAlex_PlayerCharacter::GetActiveWeaponFireRateShotsPerSecond() const
{
	if (!Equipment) return 0.f;
	const UInventoryItemInstance* Item = Equipment->GetEquippedItem(ActiveWeaponSlot);
	if (!Item || !Item->ItemData) return 0.f;
	const FInventoryItemData& Data = Item->ItemData->ItemData;
	if (Data.ItemType != EInventoryItemType::Weapon) return 0.f;
	return FMath::Max(Data.WeaponStats.FireRate, 0.01f);
}

float AAlex_PlayerCharacter::GetActiveWeaponRangedDamage() const
{
	if (!Equipment) return 0.f;
	const UInventoryItemInstance* Item = Equipment->GetEquippedItem(ActiveWeaponSlot);
	if (!Item || !Item->ItemData) return 0.f;
	const FInventoryItemData& Data = Item->ItemData->ItemData;
	if (Data.ItemType != EInventoryItemType::Weapon) return 0.f;
	return FMath::Max(0.f, Data.WeaponStats.Damage);
}

void AAlex_PlayerCharacter::PlayUniversalGunFireSoundIfConfigured()
{
	if (!UniversalGunFireSound) return;
	UWorld* const World = GetWorld();
	if (!World) return;
	FVector Loc;
	if (!TryGetAimWeaponMuzzleWorldLocation(Loc))
	{
		if (FollowCamera) Loc = FollowCamera->GetComponentLocation();
		else Loc = GetActorLocation();
	}
	UGameplayStatics::PlaySoundAtLocation(this, UniversalGunFireSound, Loc, FRotator::ZeroRotator, FMath::Max(0.f, UniversalGunFireSoundVolumeMultiplier));
}

void AAlex_PlayerCharacter::RequestAdsRangedHitscanAndScoring()
{
	if (!bDeferAdsWeaponHitscanToNextWorldTick)
	{
		PerformAdsRangedHitscanAndScoring();
		return;
	}
	UWorld* const World = GetWorld();
	if (!World) { PerformAdsRangedHitscanAndScoring(); return; }
	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &AAlex_PlayerCharacter::PerformAdsRangedHitscanAndScoring));
}

void AAlex_PlayerCharacter::PerformAdsRangedHitscanAndScoring()
{
	if (!IsValid(this) || IsActorBeingDestroyed()) return;
	FHitResult Hit;
	FVector TraceEnd;
	float MaxCm = 0.f;
	if (!GetAimUiLineTrace(Hit, TraceEnd, MaxCm) || !Hit.bBlockingHit) return;
	AActor* const HitActor = Hit.GetActor();
	if (!HitActor || HitActor == this) return;
	if (Cast<APawn>(HitActor) == nullptr && !HitActor->CanBeDamaged()) return;

	float DamageMult = 1.f;
	if (AEnemyCharacter* const Enemy = Cast<AEnemyCharacter>(HitActor))
	{
		if (Hit.Component.Get() == Enemy->GetMesh())
		{
			DamageMult = FMath::Max(0.f, Enemy->GetWeaponHitDamageMultiplierFromBoneName(Hit.BoneName));
		}
	}
	const float BaseDamage = GetActiveWeaponRangedDamage();
	const float FinalDamage = BaseDamage * DamageMult;
	if (FinalDamage > KINDA_SMALL_NUMBER)
	{
		const FVector ShotDir = (Hit.TraceStart - Hit.ImpactPoint).GetSafeNormal();
		UGameplayStatics::ApplyPointDamage(HitActor, FinalDamage, ShotDir, Hit, GetController(), this, UDamageType::StaticClass());
	}

	AAlex_PlayerController* APC = Cast<AAlex_PlayerController>(GetController());
	if (!APC)
	{
		if (UWorld* const World = GetWorld())
			APC = Cast<AAlex_PlayerController>(UGameplayStatics::GetPlayerController(World, 0));
	}
	if (APC)
	{
		APC->SetLastRangedHitDamageForDebug(FinalDamage);
		APC->AddAccumulatedHitCount(1);
	}
}

void AAlex_PlayerCharacter::TickRangedFullAuto(const float DeltaSeconds)
{
	if (bReloadMontagePlaying) { RangedAutoFireAccum = 0.f; return; }
	if (!bPrimaryFireHeld || !bIsAiming || !IsAdsRangedFullAutoEnabled()) { RangedAutoFireAccum = 0.f; return; }
	const float RoF = GetActiveWeaponFireRateShotsPerSecond();
	if (RoF <= KINDA_SMALL_NUMBER) return;
	const float Interval = 1.f / RoF;
	RangedAutoFireAccum += DeltaSeconds;
	while (RangedAutoFireAccum >= Interval)
	{
		RangedAutoFireAccum -= Interval;
		TryAttack();
	}
}

float AAlex_PlayerCharacter::ComputeAimTraceMaxDistanceCm() const
{
	float MaxCm = FMath::Max(1.f, AimUiTraceDefaultRangeMeters) * 100.f;
	if (Equipment)
	{
		if (const UInventoryItemInstance* WItem = Equipment->GetEquippedItem(ActiveWeaponSlot))
		{
			if (WItem->ItemData)
			{
				const FInventoryItemData& Id = WItem->ItemData->ItemData;
				if (Id.ItemType == EInventoryItemType::Weapon)
					MaxCm = FMath::Max(1.f, Id.WeaponStats.Range) * 100.f;
			}
		}
	}
	return MaxCm;
}

void AAlex_PlayerCharacter::GetAimCameraWorldLocationAndForward(FVector& OutCameraWorldLocation, FVector& OutCameraForward) const
{
	if (FollowCamera)
	{
		OutCameraWorldLocation = FollowCamera->GetComponentLocation();
		OutCameraForward = FollowCamera->GetForwardVector().GetSafeNormal();
	}
	else if (const AController* C = GetController())
	{
		const FRotator Rot = FRotator(C->GetControlRotation().Pitch, C->GetControlRotation().Yaw, 0.f);
		OutCameraForward = Rot.Vector().GetSafeNormal();
		OutCameraWorldLocation = GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
	}
	else
	{
		OutCameraWorldLocation = GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
		OutCameraForward = GetActorForwardVector();
	}
}

bool AAlex_PlayerCharacter::TryGetAimWeaponMuzzleWorldLocation(FVector& OutMuzzleWorldLocation) const
{
	const UStaticMeshComponent* WeaponMesh = nullptr;
	if (AimPistolMeshComp && AimPistolMeshComp->IsVisible() && AimPistolMeshComp->GetStaticMesh())
		WeaponMesh = AimPistolMeshComp;
	else if (AimRifleMeshComp && AimRifleMeshComp->IsVisible() && AimRifleMeshComp->GetStaticMesh())
		WeaponMesh = AimRifleMeshComp;

	if (WeaponMesh && AimWeaponMuzzleSocketName != NAME_None && WeaponMesh->DoesSocketExist(AimWeaponMuzzleSocketName))
	{
		OutMuzzleWorldLocation = WeaponMesh->GetSocketLocation(AimWeaponMuzzleSocketName);
		return true;
	}
	return false;
}

bool AAlex_PlayerCharacter::GetAimUiTraceParameters(FVector& OutStart, FVector& OutDirection, float& OutMaxDistanceCm) const
{
	FHitResult Hit;
	FVector TraceEnd;
	if (!GetAimUiLineTrace(Hit, TraceEnd, OutMaxDistanceCm)) return false;

	FVector CamLoc;
	FVector CamFwd;
	GetAimCameraWorldLocationAndForward(CamLoc, CamFwd);

	const FVector AimPoint = Hit.bBlockingHit ? Hit.ImpactPoint : TraceEnd;

	if (TryGetAimWeaponMuzzleWorldLocation(OutStart))
	{
		const FVector ToAim = AimPoint - OutStart;
		OutDirection = ToAim.GetSafeNormal();
		if (OutDirection.IsNearlyZero()) OutDirection = CamFwd;
	}
	else
	{
		OutStart = CamLoc;
		OutDirection = CamFwd;
	}

	return true;
}

bool AAlex_PlayerCharacter::GetAimUiLineTrace(FHitResult& OutHit, FVector& OutTraceEnd, float& OutMaxDistanceCm) const
{
	FVector CamLoc;
	FVector CamFwd;
	GetAimCameraWorldLocationAndForward(CamLoc, CamFwd);
	OutMaxDistanceCm = ComputeAimTraceMaxDistanceCm();
	OutTraceEnd = CamLoc + CamFwd * OutMaxDistanceCm;

	UWorld* const World = GetWorld();
	if (!World) return false;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(AlexAimUiTrace), false, this);
	Params.bTraceComplex = true;
	Params.AddIgnoredActor(this);

	FHitResult HitVis, HitPawn, HitStatic, HitDynamic, HitWeapon;
	World->LineTraceSingleByChannel(HitVis, CamLoc, OutTraceEnd, ECC_Visibility, Params);
	World->LineTraceSingleByChannel(HitPawn, CamLoc, OutTraceEnd, ECC_Pawn, Params);
	World->LineTraceSingleByChannel(HitStatic, CamLoc, OutTraceEnd, ECC_WorldStatic, Params);
	World->LineTraceSingleByChannel(HitDynamic, CamLoc, OutTraceEnd, ECC_WorldDynamic, Params);
	World->LineTraceSingleByChannel(HitWeapon, CamLoc, OutTraceEnd, CayFortressCollision::WeaponTrace, Params);

	OutHit = FHitResult();
	OutHit.TraceStart = CamLoc;
	OutHit.TraceEnd = OutTraceEnd;
	AlexMergeCloserHit(CamLoc, OutHit, HitVis);
	AlexMergeCloserHit(CamLoc, OutHit, HitPawn);
	AlexMergeCloserHit(CamLoc, OutHit, HitStatic);
	AlexMergeCloserHit(CamLoc, OutHit, HitDynamic);
	AlexMergeCloserHit(CamLoc, OutHit, HitWeapon);
	return true;
}

void AAlex_PlayerCharacter::UpdateAimPresentation(float DeltaTime)
{
	if (!CameraBoom || !FollowCamera) return;
	const float GoalArm = bIsAiming ? AimCameraArmLength : DefaultCameraArmLength;
	const float GoalFov = bIsAiming ? AimFieldOfView : DefaultFieldOfView;
	CurrentCameraArmLengthDisplay = FMath::FInterpTo(CurrentCameraArmLengthDisplay, GoalArm, DeltaTime, AimCameraInterpSpeed);
	CurrentFieldOfViewDisplay = FMath::FInterpTo(CurrentFieldOfViewDisplay, GoalFov, DeltaTime, AimCameraInterpSpeed);
	CameraBoom->TargetArmLength = CurrentCameraArmLengthDisplay;
	FollowCamera->SetFieldOfView(CurrentFieldOfViewDisplay);
}

void AAlex_PlayerCharacter::ApplyAimMovementConstraints()
{
	if (!bIsAiming) return;
	TargetMoveSpeed = FMath::Min(TargetMoveSpeed, AimMaxMoveSpeed);
	CurrentMoveSpeed = FMath::Min(CurrentMoveSpeed, AimMaxMoveSpeed);
}

bool AAlex_PlayerCharacter::IsActiveWeaponPistol() const
{
	if (!Equipment) return false;
	const UInventoryItemInstance* Item = Equipment->GetEquippedItem(ActiveWeaponSlot);
	if (!Item || !Item->ItemData) return false;
	const FInventoryItemData& Data = Item->ItemData->ItemData;
	if (Data.ItemType != EInventoryItemType::Weapon) return false;
	return Data.WeaponStats.WeaponClass == EWeaponClass::Pistol;
}

bool AAlex_PlayerCharacter::IsActiveWeaponRifleStyleForAdsLayer() const
{
	if (!Equipment) return false;
	const UInventoryItemInstance* Item = Equipment->GetEquippedItem(ActiveWeaponSlot);
	if (!Item || !Item->ItemData) return false;
	const FInventoryItemData& Data = Item->ItemData->ItemData;
	if (Data.ItemType != EInventoryItemType::Weapon) return false;
	switch (Data.WeaponStats.WeaponClass)
	{
	case EWeaponClass::Rifle: case EWeaponClass::SMG: case EWeaponClass::LMG: return true;
	default: return false;
	}
}

bool AAlex_PlayerCharacter::CanActiveWeaponConsumeRangedRound() const
{
	if (!Equipment) return false;
	const UInventoryItemInstance* Wpn = Equipment->GetEquippedItem(ActiveWeaponSlot);
	if (!Wpn || !Wpn->ItemData || Wpn->ItemData->ItemData.ItemType != EInventoryItemType::Weapon) return false;
	return Wpn->WeaponMagazineAmmo > 0;
}

void AAlex_PlayerCharacter::ConsumeActiveWeaponMagazineRound()
{
	if (!Equipment) return;
	UInventoryItemInstance* Wpn = Equipment->GetEquippedItem(ActiveWeaponSlot);
	if (Wpn && Wpn->WeaponMagazineAmmo > 0) Wpn->WeaponMagazineAmmo--;
}

void AAlex_PlayerCharacter::GetAmmoHudDisplayValues(int32& OutMagazineAmmo, int32& OutReserveAmmo, bool& bOutUsePistolTypeIcon, bool& bOutHasRangedWeapon) const
{
	OutMagazineAmmo = 0;
	OutReserveAmmo = 0;
	bOutUsePistolTypeIcon = false;
	bOutHasRangedWeapon = false;
	if (!Equipment) return;

	auto IsWeaponInstance = [](const UInventoryItemInstance* I) -> bool { return I && I->ItemData && I->ItemData->ItemData.ItemType == EInventoryItemType::Weapon; };

	const UInventoryItemInstance* Wpn = Equipment->GetEquippedItem(ActiveWeaponSlot);
	if (!IsWeaponInstance(Wpn)) Wpn = nullptr;
	if (!Wpn)
	{
		if (IsWeaponInstance(Equipment->GetEquippedItem(EEquipmentSlotType::Weapon1)))
			Wpn = Equipment->GetEquippedItem(EEquipmentSlotType::Weapon1);
		else if (IsWeaponInstance(Equipment->GetEquippedItem(EEquipmentSlotType::Weapon2)))
			Wpn = Equipment->GetEquippedItem(EEquipmentSlotType::Weapon2);
	}
	if (!Wpn || !Wpn->ItemData) return;
	const FInventoryItemData& Row = Wpn->ItemData->ItemData;
	if (Row.ItemType != EInventoryItemType::Weapon) return;
	bOutHasRangedWeapon = true;
	OutMagazineAmmo = FMath::Max(0, Wpn->WeaponMagazineAmmo);
	bOutUsePistolTypeIcon = (Row.WeaponStats.WeaponClass == EWeaponClass::Pistol);
	if (UInventoryComponent* Inv = GetInventory())
		OutReserveAmmo = Inv->GetTotalReserveRoundsForWeaponInInventory(Wpn->ItemData);
}

void AAlex_PlayerCharacter::ApplyReloadFillFromInventory()
{
	if (!Equipment || !GetInventory()) return;
	UInventoryItemInstance* Wpn = Equipment->GetEquippedItem(ActiveWeaponSlot);
	if (!Wpn || !Wpn->ItemData || Wpn->ItemData->ItemData.ItemType != EInventoryItemType::Weapon) return;
	const int32 Cap = FMath::Max(0, Wpn->ItemData->ItemData.WeaponStats.MagazineCapacity);
	Wpn->WeaponMagazineAmmo = FMath::Clamp(Wpn->WeaponMagazineAmmo, 0, Cap);
	const int32 Room = Cap - Wpn->WeaponMagazineAmmo;
	if (Room <= 0) return;
	const EAmmoType AT = GetInventory()->GetReserveAmmoTypeForWeapon(Wpn->ItemData);
	const int32 Taken = GetInventory()->ConsumeAmmoFromInventoryByType(AT, Room);
	Wpn->WeaponMagazineAmmo += Taken;
}

UAnimMontage* AAlex_PlayerCharacter::SelectRangedAttackMontage() const
{
	if (!Equipment) return nullptr;
	UInventoryItemInstance* Item = Equipment->GetEquippedItem(ActiveWeaponSlot);
	if (!Item || !Item->ItemData) return nullptr;
	const FInventoryItemData& Data = Item->ItemData->ItemData;
	if (Data.ItemType != EInventoryItemType::Weapon) return nullptr;
	switch (Data.WeaponStats.WeaponClass)
	{
	case EWeaponClass::Pistol: return PistolFireMontage.Get();
	default: break;
	}
	return RifleStyleFireMontage.Get();
}

UAnimMontage* AAlex_PlayerCharacter::SelectReloadMontage() const
{
	if (!Equipment) return nullptr;
	UInventoryItemInstance* Item = Equipment->GetEquippedItem(ActiveWeaponSlot);
	if (!Item || !Item->ItemData) return nullptr;
	const FInventoryItemData& Data = Item->ItemData->ItemData;
	if (Data.ItemType != EInventoryItemType::Weapon) return nullptr;
	switch (Data.WeaponStats.WeaponClass)
	{
	case EWeaponClass::Pistol: return PistolReloadMontage.Get();
	default: break;
	}
	return RifleStyleReloadMontage.Get();
}

UAnimMontage* AAlex_PlayerCharacter::SelectMeleeAttackMontage() const
{
	return UnarmedAttackMontage.Get();
}

void AAlex_PlayerCharacter::HealStaleAttackMontageGuard(const bool bLogIfHealed)
{
	if (!bAttackMontagePlaying) return;
	USkeletalMeshComponent* SkelMesh = GetMesh();
	UAnimInstance* AnimInst = SkelMesh ? SkelMesh->GetAnimInstance() : nullptr;
	if (!AnimInst || !ActiveAttackMontageGuard || !AnimInst->Montage_IsActive(ActiveAttackMontageGuard))
	{
		if (bLogIfHealed && CVarAlexCombatLogAttacks.GetValueOnGameThread() != 0)
			UE_LOG(LogAlexCombat, Warning, TEXT("%sATTACK HEAL stale guard"), CayFortressCombatLog::Prefix());
		ActiveAttackMontageGuard = nullptr;
		bAttackMontagePlaying = false;
	}
}

void AAlex_PlayerCharacter::HealStaleReloadMontageGuard(const bool bLogIfHealed)
{
	if (!bReloadMontagePlaying) return;
	USkeletalMeshComponent* SkelMesh = GetMesh();
	UAnimInstance* AnimInst = SkelMesh ? SkelMesh->GetAnimInstance() : nullptr;
	if (!AnimInst || !ActiveReloadMontageGuard || !AnimInst->Montage_IsActive(ActiveReloadMontageGuard))
	{
		if (bLogIfHealed && CVarAlexCombatLogAttacks.GetValueOnGameThread() != 0)
			UE_LOG(LogAlexCombat, Warning, TEXT("%sRELOAD HEAL stale guard"), CayFortressCombatLog::Prefix());
		ActiveReloadMontageGuard = nullptr;
		bReloadMontagePlaying = false;
	}
}

void AAlex_PlayerCharacter::HandleAttackMontageBlendOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage && ActiveAttackMontageGuard == Montage) ActiveAttackMontageGuard = nullptr;
	if (bInterrupted) bAttackMontagePlaying = false;
}

bool AAlex_PlayerCharacter::PlayAttackMontage(UAnimMontage* Montage, bool bStopAllMontagesWhenPlaying, bool bIsMeleeAttackMontage)
{
	const bool bDiag = CVarAlexCombatLogAttacks.GetValueOnGameThread() != 0;
	if (bIsMeleeAttackMontage && bIsAiming) return false;
	if (!Montage || !GetMesh()) return false;

	UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
	if (!AnimInst) return false;

	const float PlayRate = bIsMeleeAttackMontage ? MeleeMontagePlayRate : 1.f;
	float StartTime = 0.f;
	bool bJumpMeleeSection = false;
	if (bIsMeleeAttackMontage && !MeleeMontageStartSectionName.IsNone() && Montage->GetSectionIndex(MeleeMontageStartSectionName) != INDEX_NONE)
		bJumpMeleeSection = true;
	else if (bIsMeleeAttackMontage && MeleeMontageLeadInTrimSeconds > 0.f)
	{
		const float Len = Montage->GetPlayLength();
		if (Len > KINDA_SMALL_NUMBER) StartTime = FMath::Clamp(MeleeMontageLeadInTrimSeconds, 0.f, Len - 0.01f);
	}

	const float PlayLength = AnimInst->Montage_Play(Montage, PlayRate, EMontagePlayReturnType::MontageLength, StartTime, bStopAllMontagesWhenPlaying);
	if (PlayLength <= 0.f) return false;

	if (bJumpMeleeSection) AnimInst->Montage_JumpToSection(MeleeMontageStartSectionName, Montage);

	ActiveAttackMontageGuard = Montage;
	bAttackMontagePlaying = true;

	if (bIsMeleeAttackMontage && MeleePistolADSSuppressSeconds > 0.f)
		PistolADSMeleeSuppressRemain = MeleePistolADSSuppressSeconds;

	if (bIsMeleeAttackMontage && GetWorld())
		LastMeleeAttackTime = GetWorld()->GetTimeSeconds();

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AAlex_PlayerCharacter::HandleAttackMontageEnded);
	AnimInst->Montage_SetEndDelegate(EndDelegate, Montage);

	FOnMontageBlendingOutStarted BlendOutDelegate;
	BlendOutDelegate.BindUObject(this, &AAlex_PlayerCharacter::HandleAttackMontageBlendOut);
	AnimInst->Montage_SetBlendingOutDelegate(BlendOutDelegate, Montage);
	return true;
}

bool AAlex_PlayerCharacter::PlayReloadMontage(UAnimMontage* Montage, const bool bStopAllMontagesWhenPlaying)
{
	if (!Montage || !GetMesh()) return false;

	UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
	if (!AnimInst) return false;

	const float PlayLength = AnimInst->Montage_Play(Montage, ReloadMontagePlayRate, EMontagePlayReturnType::MontageLength, 0.f, bStopAllMontagesWhenPlaying);
	if (PlayLength <= 0.f) return false;

	ActiveReloadMontageGuard = Montage;
	bReloadMontagePlaying = true;
	UpdateAimWeaponVisuals();

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AAlex_PlayerCharacter::HandleReloadMontageEnded);
	AnimInst->Montage_SetEndDelegate(EndDelegate, Montage);

	FOnMontageBlendingOutStarted BlendOutDelegate;
	BlendOutDelegate.BindUObject(this, &AAlex_PlayerCharacter::HandleReloadMontageBlendOut);
	AnimInst->Montage_SetBlendingOutDelegate(BlendOutDelegate, Montage);
	return true;
}

bool AAlex_PlayerCharacter::TryReload()
{
	const bool bDiag = CVarAlexCombatLogAttacks.GetValueOnGameThread() != 0;
	HealStaleReloadMontageGuard(bDiag);
	HealStaleAttackMontageGuard(bDiag);

	if (GetInventory() && GetInventory()->IsInventoryOpen()) return false;
	if (bReloadMontagePlaying) return false;
	if (bAttackMontagePlaying) return false;
	if (bReloadRequiresAiming && !bIsAiming) return false;

	if (!Equipment) return false;
	const UInventoryItemInstance* EquippedWeapon = Equipment->GetEquippedItem(ActiveWeaponSlot);
	if (!EquippedWeapon || !EquippedWeapon->ItemData || EquippedWeapon->ItemData->ItemData.ItemType != EInventoryItemType::Weapon) return false;

	const int32 MagCap = FMath::Max(0, EquippedWeapon->ItemData->ItemData.WeaponStats.MagazineCapacity);
	if (EquippedWeapon->WeaponMagazineAmmo >= MagCap) return false;

	UInventoryComponent* PlayerInv = GetInventory();
	if (!PlayerInv) return false;
	if (PlayerInv->GetTotalReserveRoundsForWeaponInInventory(EquippedWeapon->ItemData) <= 0) return false;

	UAnimMontage* Montage = SelectReloadMontage();
	if (!Montage) return false;

	const bool bStopAll = !bReloadMontageDoNotStopAllMontages;
	SetPrimaryFireHeld(false);
	return PlayReloadMontage(Montage, bStopAll);
}

bool AAlex_PlayerCharacter::TryAttack()
{
	const bool bDiag = CVarAlexCombatLogAttacks.GetValueOnGameThread() != 0;
	HealStaleAttackMontageGuard(bDiag);
	HealStaleReloadMontageGuard(bDiag);

	if (bReloadMontagePlaying) return false;

	UAnimMontage* Montage = nullptr;
	bool bStopAllMontages = true;
	bool bPlayAsMeleeMontage = false;

	if (bIsAiming)
	{
		Montage = SelectRangedAttackMontage();
		if (Montage == UnarmedAttackMontage.Get()) return false;
		if (Montage) { bStopAllMontages = !bRangedMontageDoNotStopAllMontages; bPlayAsMeleeMontage = false; }
		else return false;
	}
	else
	{
		if (MeleeAttackCooldownSeconds > KINDA_SMALL_NUMBER && GetWorld())
		{
			const float Now = GetWorld()->GetTimeSeconds();
			if (Now - LastMeleeAttackTime < MeleeAttackCooldownSeconds) return false;
		}
		Montage = SelectMeleeAttackMontage();
		if (!Montage) return false;
		bStopAllMontages = !bMeleeMontageDoNotStopAllMontages;
		bPlayAsMeleeMontage = true;
	}

	const bool bRangedShot = bIsAiming && !bPlayAsMeleeMontage;
	if (bRangedShot && !CanActiveWeaponConsumeRangedRound()) return false;
	const bool bAutoFollowUp = bAttackMontagePlaying && bRangedShot && IsAdsRangedFullAutoEnabled();

	if (bAttackMontagePlaying && !bAutoFollowUp) return false;

	bool bPlayed = false;
	if (!bAutoFollowUp) bPlayed = PlayAttackMontage(Montage, bStopAllMontages, bPlayAsMeleeMontage);
	else bPlayed = true;

	if (bPlayed && bRangedShot)
	{
		ConsumeActiveWeaponMagazineRound();
		PlayUniversalGunFireSoundIfConfigured();
		RequestAdsRangedHitscanAndScoring();
	}
	return bPlayed;
}

void AAlex_PlayerCharacter::HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage && ActiveAttackMontageGuard == Montage) ActiveAttackMontageGuard = nullptr;
	bAttackMontagePlaying = false;
	OnAttackMontageFinished.Broadcast(bInterrupted);
}

void AAlex_PlayerCharacter::HandleReloadMontageBlendOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage && ActiveReloadMontageGuard == Montage) ActiveReloadMontageGuard = nullptr;
	if (bInterrupted) bReloadMontagePlaying = false;
	UpdateAimWeaponVisuals();
}

void AAlex_PlayerCharacter::HandleReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage && ActiveReloadMontageGuard == Montage) ActiveReloadMontageGuard = nullptr;
	bReloadMontagePlaying = false;
	if (!bInterrupted) ApplyReloadFillFromInventory();
	UpdateAimWeaponVisuals();
	OnReloadMontageFinished.Broadcast(bInterrupted);
}

void AAlex_PlayerCharacter::TickStatDrainAndEncumbrance(float DeltaTime)
{
	// --- Hunger / Hydration passive drain ---
	if (HungerDrainRate > 0.0f)
		SetHunger(Hunger - HungerDrainRate * DeltaTime);
	if (HydrationDrainRate > 0.0f)
		SetHydration(Hydration - HydrationDrainRate * DeltaTime);

	// --- Starvation / Dehydration health drain ---
	if (Hunger <= 0.0f)
		SetHealth(Health - 1.0f * DeltaTime);
	if (Hydration <= 0.0f)
		SetHealth(Health - 2.0f * DeltaTime);

	// --- Encumbrance ---
	const float CarriedWeight = GetCurrentCarriedWeight();
	const float WeightRatio = (MaxCarryWeight > KINDA_SMALL_NUMBER) ? (CarriedWeight / MaxCarryWeight) : 0.0f;

	// Broadcast carry weight change if significant
	static float LastBroadcastWeight = -1.0f;
	if (!FMath::IsNearlyEqual(LastBroadcastWeight, CarriedWeight, 0.1f))
	{
		LastBroadcastWeight = CarriedWeight;
		OnCarryWeightChanged.Broadcast(CarriedWeight);
	}

	if (WeightRatio > 1.0f)
	{
		// Over 100%: cannot move
		TargetMoveSpeed = 0.0f;
		CurrentMoveSpeed = 0.0f;
	}
	else if (WeightRatio > 0.5f && bRunInputHeld)
	{
		// Over 50%: cannot run, force walk speed
		TargetMoveSpeed = FMath::Min(TargetMoveSpeed, MoveSpeed);
	}
}

void AAlex_PlayerCharacter::HandleEquipmentChanged(EEquipmentSlotType SlotType, UInventoryItemInstance* NewItem)
{
	if (SlotType == EEquipmentSlotType::Weapon1 || SlotType == EEquipmentSlotType::Weapon2)
	{
		if (Equipment)
		{
			if (NewItem && NewItem->ItemData && NewItem->ItemData->ItemData.ItemType == EInventoryItemType::Weapon)
				ActiveWeaponSlot = SlotType;
			else if (!NewItem && SlotType == ActiveWeaponSlot)
			{
				const EEquipmentSlotType Other = (SlotType == EEquipmentSlotType::Weapon1) ? EEquipmentSlotType::Weapon2 : EEquipmentSlotType::Weapon1;
				if (UInventoryItemInstance* OtherItem = Equipment->GetEquippedItem(Other))
					if (OtherItem->ItemData && OtherItem->ItemData->ItemData.ItemType == EInventoryItemType::Weapon)
						ActiveWeaponSlot = Other;
			}
		}

		SetPrimaryFireHeld(false);
		if (bReloadMontagePlaying && ActiveReloadMontageGuard && GetMesh())
		{
			if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
				AnimInst->Montage_Stop(0.15f, ActiveReloadMontageGuard);
		}
		ActiveReloadMontageGuard = nullptr;
		bReloadMontagePlaying = false;
		SetAiming(false);
		PistolADSMeleeSuppressRemain = 0.f;
	}
}
