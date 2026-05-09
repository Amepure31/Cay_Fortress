#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Equipment/EquipmentTypes.h"
#include "Alex_PlayerController.generated.h"

class UInventoryComponent;
class UEquipmentComponent;
class UInventoryItemInstance;
class UPlayerInteractComponent;
class UUserWidget;
class ALootContainerActor;

UCLASS()
class CAY_FORTRESS_API AAlex_PlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AAlex_PlayerController();

	/** 累计命中（默认远程瞄准开火且摄像机射线打到可受伤目标时 +1）。 */
	UFUNCTION(BlueprintCallable, Category = "Combat|UI")
	void AddAccumulatedHitCount(int32 Delta = 1);

	UFUNCTION(BlueprintPure, Category = "Combat|UI")
	int32 GetAccumulatedHitCount() const { return AccumulatedHitCount; }

	/** 瞄准远程命中后刷新左上角「上次命中伤害」调试文本。 */
	UFUNCTION(BlueprintCallable, Category = "Combat|UI")
	void SetLastRangedHitDamageForDebug(float Amount);

	UFUNCTION(BlueprintPure, Category = "Combat|UI")
	float GetLastRangedHitDamageForDebug() const { return LastRangedHitDamage; }

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Jump();
	void Run(const FInputActionValue& Value);
	void StopRun();
	void AimStarted(const FInputActionValue& Value);
	void AimStopped(const FInputActionValue& Value);
	void AttackPressed(const FInputActionValue& Value);
	void AttackReleased(const FInputActionValue& Value);
	void ReloadPressed(const FInputActionValue& Value);
	void ToggleInventory();
	void Interact();
	/** @param bShowEquipment 为 false 时仅显示背包（如搜刮容器），不创建/显示装备栏。 */
	bool EnsureInventoryUIVisible(bool bShowEquipment = true);
	/** 与背包同时显示装备栏（创建/入视口/可见性/格子同步）。 */
	void EnsureEquipmentUIVisibleWithInventory();
	void OpenLootContainerUI(ALootContainerActor* LootContainer);
	void CloseLootContainerUI(bool bCloseInventoryIfOpenedByLootInteraction);
	void OpenContainerBackpackUI(UInventoryItemInstance* ContainerItem);
	void CloseContainerBackpackUI(bool bCloseInventoryIfOpenedByContainerBackpack);

protected:
	UFUNCTION()
	void OnInventoryToggled(bool bIsOpen);

	UFUNCTION()
	void OnInventoryAmmoRefresh();

	UFUNCTION()
	void OnEquipmentAmmoRefresh(EEquipmentSlotType SlotType, UInventoryItemInstance* NewItem);

	UFUNCTION()
	void OnContainerOpenRequested(UInventoryItemInstance* ContainerItem);

	UFUNCTION()
	void OnContainerBackpackRequestedFromEquipment(UInventoryItemInstance* ContainerItem);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* RunAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* AttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Interaction, meta = (AllowPrivateAccess = "true"))
	class UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Interaction, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> LootContainerWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	class UInputAction* InventoryAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> InventoryWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Equipment, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> EquipmentWidgetClass;

	/**
	 * 瞄准准星 Widget 类（须继承 UUI_AimPoint；用 UserWidget 避免在 PlayerController 头文件中拉 UMG 依赖，减轻 Live Coding 重编崩溃风险）。
	 * 未指定则不创建。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Aim", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> AimPointWidgetClass;

	/** 弹药 HUD（须继承 UUI_AmmoHUD）；未指定则不创建 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|UI", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> AmmoHudWidgetClass;

	/** 角色属性 HUD（须继承 UUI_CharacterProperty）；未指定则不创建 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|UI", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> CharacterPropertyWidgetClass;

	/** 容器背包 Widget 类（须继承 UUI_ContainerBackpack）；未指定则不创建 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> ContainerBackpackWidgetClass;

	UPROPERTY()
	UUserWidget* InventoryWidget;

	UPROPERTY()
	UUserWidget* EquipmentWidget;

	UPROPERTY()
	UInventoryComponent* InventoryComponent;

	UPROPERTY()
	UEquipmentComponent* EquipmentComponent;

	UPROPERTY()
	UPlayerInteractComponent* PlayerInteractComponent;

	UPROPERTY()
	UUserWidget* LootContainerWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget> AimPointWidget;

	UPROPERTY()
	UUserWidget* AmmoHudWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget> CharacterPropertyWidget;

	UPROPERTY()
	UUserWidget* ContainerBackpackWidget;

	UPROPERTY(Transient)
	TWeakObjectPtr<UInventoryItemInstance> ActiveContainerBackpackItem;

	bool bContainerBackpackActive;
	bool bInventoryOpenedByContainerBackpack;

	UPROPERTY(Transient)
	int32 AccumulatedHitCount = 0;

	UPROPERTY(Transient)
	float LastRangedHitDamage = 0.f;

	/** 是否在本地玩家视口左上角附近显示「累计命中」（OnScreenDebug）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|UI")
	bool bShowAccumulatedHitCounterOnScreen = true;

	/** 是否在累计命中下方显示「上次命中伤害」。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|UI")
	bool bShowLastRangedHitDamageOnScreen = true;

	/** 命中数字在屏幕上的缩放（仅 OnScreenDebug 文本）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|UI", meta = (ClampMin = "0.5", ClampMax = "3"))
	float HitCounterOnScreenTextScale = 1.35f;

	/** 「上次命中伤害」行相对命中行的缩放（默认同上）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|UI", meta = (ClampMin = "0.5", ClampMax = "3"))
	float LastHitDamageOnScreenTextScale = 1.35f;

	UPROPERTY(Transient)
	TWeakObjectPtr<ALootContainerActor> ActiveLootContainer;

	bool bLootInteractionActive;
	bool bInventoryOpenedByLootInteraction;

	bool bIsToggling;
	
	float LastToggleTime;
	bool bToggleCooldown;

	float UIUpdateThrottle;

	void EnsureAimPointWidgetCreated();
	void EnsureAmmoHudWidgetCreated();
	void EnsureCharacterPropertyWidgetCreated();
	void DrawDebugAccumulatedHitCount() const;

	void AttachToPawnInventoryAndEquipment(APawn* InPawn);
};