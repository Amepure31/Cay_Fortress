#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
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
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAlexAttackMontageFinished, bool, bInterrupted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAlexReloadMontageFinished, bool, bInterrupted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAlexDodgeMontageFinished, bool, bInterrupted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAlexHealthChanged, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAlexStaminaChanged, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAlexHungerChanged, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAlexHydrationChanged, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAlexCarryWeightChanged, float, CurrentWeight);

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	UInventoryComponent* Inventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
	UEquipmentComponent* Equipment;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception", meta = (AllowPrivateAccess = "true"))
	UAIPerceptionStimuliSourceComponent* AIPerceptionStimuliSource;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Aim", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* AimPistolMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Aim", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* AimRifleMeshComp;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (ClampMin = "0", UIMin = "0"))
	float Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (ClampMin = "1", UIMin = "1"))
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (ClampMin = "0", UIMin = "0"))
	float Stamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (ClampMin = "1", UIMin = "1"))
	float MaxStamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (ClampMin = "0", UIMin = "0"))
	float StaminaRecoveryRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (ClampMin = "0", UIMin = "0"))
	float Hunger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (ClampMin = "1", UIMin = "1"))
	float MaxHunger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats|Drain", meta = (ClampMin = "0", UIMin = "0"))
	float HungerDrainRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (ClampMin = "0", UIMin = "0"))
	float Hydration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (ClampMin = "1", UIMin = "1"))
	float MaxHydration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats|Drain", meta = (ClampMin = "0", UIMin = "0"))
	float HydrationDrainRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stats", meta = (ClampMin = "0", UIMin = "0"))
	float MaxCarryWeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Search", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float SearchAbilityCoefficient = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Movement", meta = (ClampMin = "0", ClampMax = "600", UIMin = "0", UIMax = "600"))
	float MoveSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Movement", meta = (ClampMin = "0", ClampMax = "600", UIMin = "0", UIMax = "600"))
	float RunSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Movement", meta = (ClampMin = "0.01", ClampMax = "2.0", UIMin = "0.01", UIMax = "2.0"))
	float SpeedTransitionTime;

	// --- Aiming ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim", meta = (ClampMin = "50.0", UIMin = "50.0"))
	float AimCameraArmLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim", meta = (ClampMin = "30.0", ClampMax = "120.0", UIMin = "30.0", UIMax = "120.0"))
	float AimFieldOfView;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim", meta = (ClampMin = "0.0", UIMax = "600.0", UIMin = "0.0"))
	float AimMaxMoveSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float AimCameraInterpSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim", meta = (ClampMin = "0.05", ClampMax = "1.0", UIMin = "0.05", UIMax = "1.0"))
	float AimLookSpeedScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	bool bAimOffsetPitchUseMeshBase;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	bool bAimOffsetYawUsesMeshBase;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	bool bApplyControllerDesiredRotationWhileAiming;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim", meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"))
	float PistolADSLayerBlendCap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim", meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"))
	float RifleADSLayerBlendCap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	TObjectPtr<UStaticMesh> AimPistolStaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	FName AimPistolAttachSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	FTransform AimPistolRelativeTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	bool bShowAimPistolOnlyWhenPistolEquipped;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	TObjectPtr<UStaticMesh> AimRifleStaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	FName AimRifleAttachSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	FTransform AimRifleRelativeTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	bool bShowAimRifleOnlyWhenRifleEquipped;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Aim")
	bool bIsAiming;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim")
	FName AimWeaponMuzzleSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Aim", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float AimUiTraceDefaultRangeMeters;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
	EEquipmentSlotType ActiveWeaponSlot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee")
	bool bMeleeMontageDoNotStopAllMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged")
	bool bRangedMontageDoNotStopAllMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reload")
	bool bReloadMontageDoNotStopAllMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reload", meta = (ClampMin = "0.25", ClampMax = "4.0", UIMin = "0.25", UIMax = "4.0"))
	float ReloadMontagePlayRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reload")
	bool bReloadRequiresAiming;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged")
	bool bRifleStyleAdsFullAutoByDefault = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged")
	bool bDeferAdsWeaponHitscanToNextWorldTick = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee")
	FName MeleeMontageStartSectionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee", meta = (ClampMin = "0.25", ClampMax = "4.0", UIMin = "0.25", UIMax = "4.0"))
	float MeleeMontagePlayRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee", meta = (ClampMin = "0.0", ClampMax = "3.0", UIMin = "0.0", UIMax = "3.0"))
	float MeleeMontageLeadInTrimSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee", meta = (ClampMin = "0", ClampMax = "2", UIMin = "0", UIMax = "2"))
	float MeleePistolADSSuppressSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee", meta = (ClampMin = "0", ClampMax = "3", UIMin = "0", UIMax = "3"))
	float MeleeAttackCooldownSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> UnarmedAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> RifleStyleFireMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> PistolFireMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> PistolReloadMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> RifleStyleReloadMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> DodgeMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge", meta = (ClampMin = "0", ClampMax = "100", UIMin = "0", UIMax = "100"))
	float DodgeStaminaCost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge", meta = (ClampMin = "0", ClampMax = "2000", UIMin = "0", UIMax = "2000"))
	float DodgeImpulseStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge", meta = (ClampMin = "0", ClampMax = "3", UIMin = "0", UIMax = "3"))
	float DodgeCooldownSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge", meta = (ClampMin = "0.25", ClampMax = "4.0", UIMin = "0.25", UIMax = "4.0"))
	float DodgeMontagePlayRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge")
	FName DodgeMontageStartSectionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge")
	bool bDodgeMontageDoNotStopAllMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Audio")
	TObjectPtr<USoundBase> UniversalGunFireSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Audio", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "4.0", UIMax = "4.0"))
	float UniversalGunFireSoundVolumeMultiplier = 1.f;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnAlexAttackMontageFinished OnAttackMontageFinished;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnAlexReloadMontageFinished OnReloadMontageFinished;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnAlexDodgeMontageFinished OnDodgeMontageFinished;

	UPROPERTY(BlueprintAssignable, Category = "Character|Stats")
	FOnAlexHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Character|Stats")
	FOnAlexStaminaChanged OnStaminaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Character|Stats")
	FOnAlexHungerChanged OnHungerChanged;

	UPROPERTY(BlueprintAssignable, Category = "Character|Stats")
	FOnAlexHydrationChanged OnHydrationChanged;

	UPROPERTY(BlueprintAssignable, Category = "Character|Stats")
	FOnAlexCarryWeightChanged OnCarryWeightChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Movement")
	float TargetMoveSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Movement")
	float CurrentMoveSpeed;

	float CurrentRunSpeed;
	bool bRunInputHeld;
	bool bCachedMoveRotSettingsFromAim = false;
	bool bCachedOrientRotationToMovement = false;
	bool bCachedUseControllerDesiredRotation = false;

	float DefaultCameraArmLength;
	float DefaultFieldOfView;
	float CurrentCameraArmLengthDisplay;
	float CurrentFieldOfViewDisplay;

	bool bAttackMontagePlaying;
	bool bReloadMontagePlaying = false;
	bool bPrimaryFireHeld = false;
	float RangedAutoFireAccum = 0.f;
	float PistolADSMeleeSuppressRemain = 0.f;
	float LastMeleeAttackTime = -1000.f;

	TObjectPtr<UAnimMontage> ActiveAttackMontageGuard;
	TObjectPtr<UAnimMontage> ActiveReloadMontageGuard;

	bool bDodgeMontagePlaying = false;
	bool bIsInvincible = false;
	bool bDodgeComboAvailable = false;
	bool bDodgeIsSection2 = false;
	float LastDodgeTime = -1000.f;
	float DodgeComboWindowSeconds = 1.0f;
	TObjectPtr<UAnimMontage> ActiveDodgeMontageGuard;
	FTimerHandle DodgeComboCloseTimer;

	void LogAnimMontageSkeletonMismatches() const;
	void UpdateAimPistolVisual();
	void UpdateAimRifleVisual();
	void UpdateAimWeaponVisuals();
	void UpdateAimPresentation(float DeltaTime);
	void HealStaleAttackMontageGuard(bool bLogIfHealed);
	void HealStaleReloadMontageGuard(bool bLogIfHealed);
	void HealStaleDodgeMontageGuard(bool bLogIfHealed);
	void HandleAttackMontageBlendOut(UAnimMontage* Montage, bool bInterrupted);
	void HandleDodgeMontageBlendOut(UAnimMontage* Montage, bool bInterrupted);
	void CloseDodgeComboWindow();
	void ApplyAimMovementConstraints();
	void TickStatDrainAndEncumbrance(float DeltaTime);
	bool IsAdsRangedFullAutoEnabled() const;
	float GetActiveWeaponFireRateShotsPerSecond() const;
	float GetActiveWeaponRangedDamage() const;
	void PerformAdsRangedHitscanAndScoring();
	void RequestAdsRangedHitscanAndScoring();
	void PlayUniversalGunFireSoundIfConfigured();
	void TickRangedFullAuto(float DeltaSeconds);
	UAnimMontage* SelectRangedAttackMontage() const;
	UAnimMontage* SelectReloadMontage() const;
	UAnimMontage* SelectMeleeAttackMontage() const;
	bool PlayAttackMontage(UAnimMontage* Montage, bool bStopAllMontagesWhenPlaying, bool bIsMeleeAttackMontage);
	bool PlayReloadMontage(UAnimMontage* Montage, bool bStopAllMontagesWhenPlaying);
	bool PlayDodgeMontage(UAnimMontage* Montage);
	void HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void HandleReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void HandleReloadMontageBlendOut(UAnimMontage* Montage, bool bInterrupted);
	void HandleDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	bool CanActiveWeaponConsumeRangedRound() const;
	void ConsumeActiveWeaponMagazineRound();
	void ApplyReloadFillFromInventory();

	UFUNCTION()
	void HandleEquipmentChanged(EEquipmentSlotType SlotType, UInventoryItemInstance* NewItem);

public:
	UFUNCTION(BlueprintCallable, Category = "Character|Inventory")
	UInventoryComponent* GetInventory() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Equipment")
	UEquipmentComponent* GetEquipment() const { return Equipment; }

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetHealth() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	void SetHealth(float InHealth);

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	void Heal(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetStamina() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetMaxStamina() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetStaminaPercent() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	void SetStamina(float InStamina);

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	void RecoverStamina(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	bool ConsumeStamina(float Amount);

	// --- Hunger ---
	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetHunger() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetMaxHunger() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetHungerPercent() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	void SetHunger(float InHunger);

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	void RestoreHunger(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	bool ConsumeHunger(float Amount);

	// --- Hydration ---
	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetHydration() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetMaxHydration() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetHydrationPercent() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	void SetHydration(float InHydration);

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	void RestoreHydration(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	bool ConsumeHydration(float Amount);

	// --- Damage Reduction / Armor (pass-through) ---
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character|Stats")
	float GetTotalDamageReduction() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character|Stats")
	float GetTotalArmorValue() const;

	// --- Carry Weight ---
	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetMaxCarryWeight() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetCurrentCarriedWeight() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	float GetCarryWeightPercent() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Stats")
	void SetMaxCarryWeight(float InMaxCarryWeight);

	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	float GetMoveSpeed() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	void SetMoveSpeed(float InMoveSpeed);

	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	void SetTargetMoveSpeed(float InTargetMoveSpeed);

	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	float GetTargetMoveSpeed() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	float GetCurrentMoveSpeed() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	float GetRunSpeed() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	void SetRunSpeed(float InRunSpeed);

	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	void SetRunInputHeld(bool bHeld);

	UFUNCTION(BlueprintPure, Category = "Character|Movement")
	bool IsRunInputHeld() const { return bRunInputHeld; }

	UFUNCTION(BlueprintCallable, Category = "Combat|Aim")
	void SetAiming(bool bWantAim);

	UFUNCTION(BlueprintCallable, Category = "Combat|Ranged")
	void SetPrimaryFireHeld(bool bHeld);

	UFUNCTION(BlueprintPure, Category = "Combat|Aim")
	bool IsAiming() const { return bIsAiming; }

	UFUNCTION(BlueprintCallable, Category = "Combat|Aim")
	bool GetAimUiTraceParameters(FVector& OutStart, FVector& OutDirection, float& OutMaxDistanceCm) const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Aim")
	bool GetAimUiLineTrace(FHitResult& OutHit, FVector& OutTraceEnd, float& OutMaxDistanceCm) const;

	UFUNCTION(BlueprintPure, Category = "Combat|Aim")
	void GetAimCameraWorldLocationAndForward(FVector& OutCameraWorldLocation, FVector& OutCameraForward) const;

	UFUNCTION(BlueprintPure, Category = "Combat|Aim")
	bool TryGetAimWeaponMuzzleWorldLocation(FVector& OutMuzzleWorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "Combat|Aim")
	float ComputeAimTraceMaxDistanceCm() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
	bool IsActiveWeaponPistol() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
	bool IsActiveWeaponRifleStyleForAdsLayer() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Melee")
	float GetPistolADSMeleeSuppressRemain() const { return PistolADSMeleeSuppressRemain; }

	UFUNCTION(BlueprintPure, Category = "Combat|Aim")
	float GetPistolADSLayerBlendCap() const { return PistolADSLayerBlendCap; }

	UFUNCTION(BlueprintPure, Category = "Combat|Aim")
	float GetRifleADSLayerBlendCap() const { return RifleADSLayerBlendCap; }

	UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
	void GetAmmoHudDisplayValues(int32& OutMagazineAmmo, int32& OutReserveAmmo, bool& bOutUsePistolTypeIcon, bool& bOutHasRangedWeapon) const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool TryAttack();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool TryReload();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool TryDodge();

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsAttackMontagePlaying() const { return bAttackMontagePlaying; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsReloadMontagePlaying() const { return bReloadMontagePlaying; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsDodgeMontagePlaying() const { return bDodgeMontagePlaying; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsInvincible() const { return bIsInvincible; }

	UFUNCTION(BlueprintPure, Category = "Combat|Dodge")
	float GetDodgeCooldownRemaining() const;
};
