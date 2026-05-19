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
class AStorageCabinetActor;
class ATrainingMachineActor;
class AWeaponWorkbenchActor;
class UUI_LootProgressBar;

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

	/** 在指定世界位置弹出伤害跳字。bHeadshot=true 时文字变红。 */
	UFUNCTION(BlueprintCallable, Category = "Combat|UI")
	void ShowDamageNumberAtLocation(FVector WorldLocation, float Damage, bool bHeadshot);

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
	void Dodge();
	void ToggleInventory();
	void InteractStarted();
	void InteractCompleted();
	void StartLootProgress(ALootContainerActor* Container);
	void CancelLootProgress();
	void CompleteLootProgress();
	void TransferAllItemsFromContainer();
	/** @param bShowEquipment 为 false 时仅显示背包（如搜刮容器），不创建/显示装备栏。 */
	bool EnsureInventoryUIVisible(bool bShowEquipment = true);
	/** 与背包同时显示装备栏（创建/入视口/可见性/格子同步）。 */
	void EnsureEquipmentUIVisibleWithInventory();
	void OpenLootContainerUI(ALootContainerActor* LootContainer);
	void CloseLootContainerUI(bool bCloseInventoryIfOpenedByLootInteraction);
	void OpenContainerBackpackUI(UInventoryItemInstance* ContainerItem);
	void CloseContainerBackpackUI(bool bCloseInventoryIfOpenedByContainerBackpack);

public:
	void OpenStorageCabinetUI(AStorageCabinetActor* Cabinet);
	void CloseStorageCabinetUI(bool bCloseInventoryIfOpenedByCabinet);
	void OpenTrainingMachineUI(ATrainingMachineActor* Machine);
	void CloseTrainingMachineUI(bool bCloseInventoryIfOpenedByMachine);
	void OpenWeaponWorkbenchUI(AWeaponWorkbenchActor* Workbench);
	void CloseWeaponWorkbenchUI(bool bCloseInventoryIfOpenedByWorkbench);

	/** 刷新玩家背包 UI（外部调用，如家具消耗物品后） */
	void RefreshPlayerInventoryUI();

	void EnsureMinimapWidgetCreated();
	void ToggleMinimap();

	void ToggleDebugUI();

public:
	UFUNCTION(BlueprintCallable, Category = "Debug")
	bool IsDebugUIVisible() const { return bDebugUIVisible; }

private:

private:
	void CloseAllFurnitureUI();

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* DodgeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Interaction, meta = (AllowPrivateAccess = "true"))
	class UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Interaction, meta = (AllowPrivateAccess = "true"))
	float LootHoldDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Interaction, meta = (AllowPrivateAccess = "true"))
	float LootSpeedUpMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Interaction, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> LootProgressBarWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Interaction, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> LootContainerWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	class UInputAction* InventoryAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	class UInputAction* MinimapAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug", meta = (AllowPrivateAccess = "true"))
	class UInputAction* DebugUIAction;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> StorageCabinetWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> TrainingMachineWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> WeaponWorkbenchWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Furniture", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> DamageNumberWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> MinimapWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AMinimapCaptureActor> MinimapCaptureActorClass;

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
	TObjectPtr<UUserWidget> MinimapWidget;

	UPROPERTY(Transient)
	TWeakObjectPtr<class AMinimapCaptureActor> MinimapCaptureActor;

	UPROPERTY()
	UUserWidget* ContainerBackpackWidget;

	UPROPERTY(Transient)
	TWeakObjectPtr<UInventoryItemInstance> ActiveContainerBackpackItem;

	bool bContainerBackpackActive;
	bool bInventoryOpenedByContainerBackpack;

	bool bMinimapVisible;
	bool bDebugUIVisible = false;


	UPROPERTY(Transient)
	int32 AccumulatedHitCount = 0;

	UPROPERTY(Transient)
	float LastRangedHitDamage = 0.f;

	/** 是否在本地玩家视口左上角附近显示「累计命中」（OnScreenDebug）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|UI")
	bool bShowAccumulatedHitCounterOnScreen = false;

	/** 是否在累计命中下方显示「上次命中伤害」。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|UI")
	bool bShowLastRangedHitDamageOnScreen = false;

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
	float LootContainerOpenTime;

	UPROPERTY(BlueprintReadOnly, Category = "Furniture", meta = (AllowPrivateAccess = "true"))
	TWeakObjectPtr<AStorageCabinetActor> ActiveStorageCabinet;

	UPROPERTY(BlueprintReadOnly, Category = "Furniture", meta = (AllowPrivateAccess = "true"))
	TWeakObjectPtr<ATrainingMachineActor> ActiveTrainingMachine;

	UPROPERTY(BlueprintReadOnly, Category = "Furniture", meta = (AllowPrivateAccess = "true"))
	TWeakObjectPtr<AWeaponWorkbenchActor> ActiveWeaponWorkbench;

	UPROPERTY()
	TObjectPtr<UUserWidget> StorageCabinetWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget> TrainingMachineWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget> WeaponWorkbenchWidget;

	bool bStorageCabinetActive;
	bool bInventoryOpenedByCabinet;

	bool bTrainingMachineActive;
	bool bInventoryOpenedByTrainingMachine;

	bool bWeaponWorkbenchActive;
	bool bInventoryOpenedByWeaponWorkbench;

	bool bIsLootProgressActive;
	bool bLootRunSpeedUp;
	float LootProgressElapsed;

	UPROPERTY(Transient)
	TWeakObjectPtr<ALootContainerActor> LootProgressTarget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> LootProgressWidget;

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