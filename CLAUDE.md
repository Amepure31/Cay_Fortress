# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build / Run

- **UE version**: 5.7 (installed at `D:\UE\UE_5.7`)
- **IDE**: Rider (`.idea/`) or VS (`Cay_Fortress.sln`, `.vs/`) — both present
- **Build from CLI** (Win64 Development Editor):
  ```powershell
  & "D:\UE\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" Cay_FortressEditor Win64 Development -Project="W:\UE5\Cay_Fortress\Cay_Fortress.uproject" -WaitMutex
  ```
  Or open `Cay_Fortress.uproject` in UE Editor and use **Live Coding** (Ctrl+Alt+F11) for hot-reload.
- **No automated tests exist** — all verification is done in-editor via PIE (Play In Editor)

## Critical Rules

- **DataAsset modifications**: Before modifying any USTRUCT in `InventoryItemDataAsset.h` (e.g., `FInventoryItemData`, `FArmorItemStats`, `FFoodItemStats`), warn the user. Changing struct layout can corrupt existing Blueprint DataAsset instances that were serialized with the old layout.
- **Slate input routing**: NEVER call `SetInputMode` during Slate event routing (`NativeOnPreviewKeyDown`, `NativeOnMouseButtonDown`, etc.). If a Slate event handler needs to open/close UI, defer the operation to the next tick via `GetWorld()->GetTimerManager().SetTimerForNextTick()`. Changing input mode mid-routing corrupts Slate's internal state and causes permanent loss of camera/mouse control.
- **Unity Build collisions**: Do not define the same `static` function or anonymous-namespace function across multiple `.cpp` files in the same module. UE merges `.cpp` files into unified translation units, so duplicate definitions collide. Put shared internal helpers in a `#pragma once` header under `Private/` instead.
- **UE5.7 TWeakObjectPtr**: `TWeakObjectPtr<T>` assignment from raw `T*` pointer requires the full type definition of T to be visible at the assignment site. Forward declarations are insufficient. If the type is only forward-declared in a header, move the function body to the .cpp file.
- **UE5.7 TObjectPtr**: Does not have `.IsValid()` — use `!= nullptr` or `IsValid(Ptr.Get())`.
- **Minimap Widget visibility**: `UUI_Minimap` must use `HitTestInvisible` (not `Visible`), otherwise it eats mouse clicks in the top-right corner area. It auto-hides when any other UI is open (`bShowMouseCursor == true`).
- **All UE_LOG calls are commented out** — do not add new ones without asking.

## Architecture

Single module `Cay_Fortress` (Runtime, LoadingPhase: Default). Public deps: `Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`, `UMG`, `AIModule`, `NavigationSystem`. Private deps: `Slate`, `SlateCore`, `AssetRegistry`, `GameplayTasks`.

### Player

- **`AAlex_PlayerCharacter`** — the player pawn. Owns `UInventoryComponent` (bag), `UEquipmentComponent` (slots), `UAIPerceptionStimuliSourceComponent` (Sight + Hearing), and two `UStaticMeshComponent` (aim pistol/rifle visuals). Handles movement speed interpolation, aiming (camera arm length + FOV lerp), melee attacks (unarmed montage), ranged attacks (hit-scan via camera → crosshair), reload (consumes matching ammo from inventory), health/stamina, dodge/roll, and sprint noise emission.
- **`AAlex_PlayerController`** — owns all UI widget lifecycle (create/add/remove for inventory, equipment, loot container, aim point, ammo HUD, minimap, damage numbers). Uses Enhanced Input with IA_* assets and IMC_Player. Manages loot/furniture interaction state machines. Central `bShowMouseCursor` flag controls whether game input is blocked (attacks, movement blocked; minimap hidden).
- **`AAlex_GameMode`** — sets default pawn and controller via `DefaultPawnClassRef` / `PlayerControllerClassRef` (configured in blueprints).
- **`AAlex_AnimInstance`** — animation blueprint C++ base (reads `bIsAiming`, weapon type, ADS layer weights).

### Player Stats

12 stats tracked on `AAlex_PlayerCharacter` with dynamic delegates (`FOnAlex*Changed`) for UI binding:
- **Health** — damage from enemies, healed via medical items
- **Stamina** — consumed by sprint (1.0/sec) and dodge (25 per use), recovers when not sprinting
- **Hunger** — drains over time (`HungerDrainRate`), causes penalties at 0
- **Hydration** — drains over time (`HydrationDrainRate`), causes penalties at 0
- **Carry Weight** — total inventory weight vs. max; exceeding max applies encumbrance (speed penalty)

### Inventory (jigsaw-style grid)

- **`UInventoryComponent`** — core bag logic. 2D grid (`TArray<FFInventoryGridCell> Grid`), item list (`TArray<UInventoryItemInstance> Items`). Supports irregular shapes via `FFItemShapeMask`. Add/remove/move items, stack merging, find-space-first-fit. Key functions: `AddExistingItemInstance()`, `DetachItemInstance()` (sets SlotX/Y to -1), `AttachItemInstance()` (re-attaches with new position). When moving items between inventories, detach FIRST then add — reversing the order leaves SlotX/Y at -1.
- **`UInventoryItemInstance`** — runtime `UObject` for each bag item. Holds `ItemData` pointer, `StackSize`, grid position, shape, durability, `WeaponMagazineAmmo`, `WeaponModLevels`. Armor items can hold nested `ContainerInventory`.
- **`UInventoryItemDataAsset`** — `UPrimaryDataAsset` subclass. Subdirectories per type: `Weapon/`, `Armor/`, `Ammo/`, `Food/`, `Medical/`, `Materials/`.
- **Key enums**: `EInventoryItemType` (Weapon=0, Armor=1, Material=3, Food=7, Medical=8, Energy=9, Ammo=10), `EWeaponClass`, `EAmmoType`. `EWeaponClass` → `EAmmoType`: SMG/Rifle/LMG → Rifle ammo.

### Ammo Subsystem

- Weapon magazines hold ammo separately from loose ammo in inventory. `TryUnloadWeaponMagazineToLooseAmmo()` extracts magazine contents into loose ammo items.
- Loose ammo items have `bCountAsAmmoForReserve=true` so the ammo HUD counts them for reserve display.
- `DefaultAmmoItemByType` auto-discovers matching ammo DataAssets at runtime via AssetRegistry.
- Companion ammo grant: when equipping a weapon, the system can auto-grant matching ammo items.

### Equipment

- **`UEquipmentComponent`** — 5 slots: Head, Chest, Backpack, Weapon1, Weapon2. `EquipItemFromInventory` / `UnequipItemToInventory`. `GetTotalDamageReduction()` / `GetTotalArmorValue()`.
- **`EEquipmentSlotType`** enum in `EquipmentTypes.h`.
- **Container backpacks**: Armor items with `bIsContainer=true` double as independent inventories with weight reduction ratios. Accessed via equipment slot UI.
- `FOnEquipmentChanged` delegate broadcasts on any equip/unequip.

### Combat

- **Ranged**: `PerformAdsRangedHitscanAndScoring()` — camera→crosshair line trace on `CayFortressWeapon` channel. Per-bone damage multipliers on enemies. Damage numbers only shown for enemies (`Cast<AEnemyCharacter>` guard).
- **Melee**: `PerformMeleeHitDetection()` — sphere sweep forward (`ECC_Pawn`), 20 base damage (configurable `MeleeDamage`), 180 range, 90 radius. Triggered by `AnimNotify_PlayerMeleeAttack` in the unarmed attack montage. Dead enemies and non-enemy hit actors are skipped. `MeleeDamage`/`MeleeRange`/`MeleeRadius` properties on character.
- **Attack blocked**: When `bShowMouseCursor` is true (any UI open), `AttackPressed` returns early.
- **Sprint stamina**: `SprintStaminaDrainRate = 1.0f/sec`. No stamina recovery while sprinting. At 0 stamina, forces walk speed.
- **Dodge**: `TryDodge()` costs 25 stamina, has combo window, grants invincibility.
- **Montage delegates**: `FOnAlexAttackMontageFinished`, `FOnAlexReloadMontageFinished`, `FOnAlexDodgeMontageFinished` — broadcast on montage end (blend out or interrupt), used to gate state transitions.

### AnimNotify Classes

- **`UAnimNotify_PlayerMeleeAttack`** — triggers `PerformMeleeHitDetection()` during the unarmed attack montage.
- **`UAnimNotify_EnemyAttackDamage`** — triggers enemy attack damage application during enemy attack montages.

### Loot Hold-Progress System

Looting requires holding the interact key (E) for a configurable duration (default ~3s). Holding Shift doubles looting speed (2×). Progress is displayed via `UUI_LootProgressBar` widget. Releasing the key before completion cancels the loot. This prevents accidental looting during combat.

### Enemy AI

- **`AEnemyCharacter`** — health, death sequence, per-bone-hit multipliers (head 2×). On death, skeletal mesh immediately ignores `WeaponTrace` channel so corpses don't block shots. `TakeDamage` alerts nearby enemies within 500cm via `ForceDetectPlayer`. Max 15 enemy corpses in world (oldest destroyed when exceeded). Capsule shrinks on death to free navmesh.
- **`AEnemyAIController`** — Sight (2400cm, 80° half-angle) + Hearing (500cm) perception. **Behavior states**: Roam → Alert → Chase → Attack. Roam picks random navmesh points within `RoamWanderRadius` (1800cm) from spawn anchor. Alert builds suspicion from 0 to `SuspicionMax` (100) before transitioning to Chase. Chase uses controller-driven MoveTo updates every 0.3s (`bDriveChaseFromController`). Attack triggers when within `AttackTriggerRange` (220cm), with `AttackIntervalSeconds` (1.2s) cooldown.
- **Chase persistence**: When sight is lost during Chase, switches to Alert at 92% suspicion. Uses `LastKnownPlayerLocation` for investigation.
- **Door-breaking AI**: AI detects blocking doors via sphere sweep (400cm radius, `DestructibleObstacle` channel). Sets `CurrentDoorTarget`, navigates to door, enters Attack state. Each hit deals 50 damage; doors have 150 HP (3 hits). On door destruction, AI resumes Chase.
- **Hit-react stun**: Taking damage briefly stuns the AI (`HitReactStunSeconds = 0.35s`), suppressing chase movement during the flinch montage.
- **Scream intro**: Alert→Chase transition plays a scream montage (suppresses chase movement until complete).
- **Enemy alert cascade**: When an enemy takes damage, all enemies within 5m get `ForceDetectPlayer`. Player sprinting emits `ReportNoiseEvent` at 3m range every 0.5s.
- **Camera spring arm**: Enemy capsule and skeletal mesh ignore `ECC_Camera` to prevent camera retraction when enemies walk between camera and player.

### Door / Destructible Obstacle

- **`ADoorActor`** — implements `IDestructibleObstacle`. Has 150 HP, destroyed after taking 150 damage (enemies deal 50/hit). Uses `DestructibleObstacle` collision object type. On destruction, removes collision (unblocks navmesh, triggers dynamic nav rebuild). `BlueprintImplementableEvent`s: `OnDoorTakeDamage`, `OnDoorDestroyed` (for audio/effects in BP).
- **`IDestructibleObstacle`** — interface for AI-breakable obstacles: `CanBeDestroyedByAI`, `TakeAIAttackDamage`, `IsObstacleDestroyed`, `GetObstacleLocation`. All `BlueprintNativeEvent`.

### Persistence (Cross-Level Save)

- **`UInventoryPersistenceSubsystem`** (`UGameInstanceSubsystem`) — survives `UWorld` destruction during `OpenLevel`. Captures inventory, equipment, player stats, and container-backpack items into UPROPERTY structs before level transition. Restores on the other side via `CaptureFromPlayer()` / `RestoreToPlayer()`. Check `HasCapturedData()` before attempting restore.
- **Serialized structs**: `FPersistedItemData` (per-item: DataAsset path, stack, durability, grid pos, shape, rotation, magazine ammo, weapon mod overrides), `FPersistedEquipmentSlot`, `FPersistedPlayerStats` (health, stamina, hunger, hydration + maxes + drain rates), `FPersistedItemDataArray` (wrapper for TMap serialization).
- **`UCayFortressGameInstance`** — holds `PendingTeleportNodeID` and `LastLevelName` for cross-level teleport state. Configured in `DefaultEngine.ini` as `GameInstanceClass`.

### Teleport

- **`ATeleportNode`** — implements `IInteractableInterface` for cross-level travel. Reads target level from DataAsset/blueprint config. Before teleport, triggers `UInventoryPersistenceSubsystem::CaptureFromPlayer()`. After level load, `RestoreToPlayer()` recovers all state.

### Base Building (Furniture)

Three furniture types with shared pattern: walk-away auto-closes UI (checked each Tick), pressing E again toggles close, close functions accept `bCloseInventoryIfOpenedBy*` parameter. Each has a config DataAsset (`UPrimaryDataAsset` subclass).
- **`AWeaponWorkbenchActor`** — weapon modification system. 9 mod types (`EWeaponModType`): Damage, FireRate, MagCapacity, Recoil, Range, ReloadSpeed, HipFire, ADS, Handling. Weapons are "bound" to the workbench; mods apply `FWeaponStatOverrides` to the item instance. Has grid upgrade system. Config: `BP_TrainingMachineConfig_DA` or similar weapon-workbench DA.
- **`ATrainingMachineActor`** — stat upgrade system. 6 stat types (`ETrainingStatType`): MaxHealth, MaxStamina, StaminaRecovery, MaxHunger, MaxHydration, MaxCarryWeight. Upgrade levels stored with `UPROPERTY(SaveGame)` — persist across sessions independently of the inventory persistence subsystem.
- **`AStorageCabinetActor`** — grid expansion upgrade tree with escalating costs. Expands the cabinet's inventory grid dimensions.

### Room Actor

- **`ARoomActor`** — manages all `ALootContainerActor` instances attached to a room. On `BeginPlay`, auto-collects attached containers, optionally runs two-phase refresh: (1) `RefreshContainerPresence()` — probabilistic activation of containers (per-type override chances via `ContainerRefreshChanceByType`), (2) `RefreshItemsInActiveContainers()` — fills active containers with loot.
- Tracks opened/looted state for all managed containers via delegate bindings (`FOnRoomContainersOpenedStateChanged`, `FOnRoomContainersLootedStateChanged`). `EvaluateRoomState()` recalculates `bAllContainersOpened` / `bAllContainersLooted`.

### Minimap

- **`AMinimapCaptureActor`** — top-down orthographic SceneCapture2D, 5000 world units wide, captures at ~60 Hz. Follows player XY. Shows only static world geometry (lighting/shadows/skeletal meshes disabled via ShowFlags). Hidden actors: player pawn.
- **`UUI_Minimap`** — widget at z-order=1, always visible (HitTestInvisible), hides when `bShowMouseCursor` is true. M key toggles 2.5x zoom. Has `PlayerIndicator` (rotating arrow, Yaw-90° correction) and enemy markers (red arrows, tracked at 10 Hz, filtered by outdoor visibility). MapRadius=200, WorldToPixelScale=0.08 for 400×400 RT.
- Enemy indicators skipped if outside map range or if upward visibility trace to 800cm is blocked (indoor enemies).

### Interaction / Furniture

- **`IInteractableInterface`** — `CanInteract`, `Interact`, `GetInteractText` (all `BlueprintNativeEvent`).
- **`UPlayerInteractComponent`** — sphere sweep each tick (250cm range, 20cm radius, `ECC_Visibility`).
- **Furniture** (`ATrainingMachineActor`, `AWeaponWorkbenchActor`, `AStorageCabinetActor`): Walk-away closes UI (checked each Tick). Pressing E again toggles close. Close functions accept `bCloseInventoryIfOpenedBy*` parameter.
- **`ALootContainerActor`** — has `EContainerType` determining refresh loot table. `RefreshItemsByRoom_Implementation` scans AssetRegistry for DataAssets matching the container type.

### Container Types (EContainerType)

| Value | Enum | Refresh Logic |
|-------|------|---------------|
| 0 | FoodCrate | Food items, 0-3, rarity roll |
| 1 | WeaponCrate | Weapon items |
| 2 | MedicalCrate | Medical items, 0-3 |
| 3 | Toilet | 75% FreshWater |
| 4 | EnergyCrate | Energy items |
| 5 | MaterialCrate | Material items |
| 6 | AmmoCrate | Ammo items |
| 7 | ArmorCrate | Armor items |
| 8 | EquipmentCrate | Helmet or Armor |
| 9 | ToolKit | Ammo + Material items, 0-3 |

### UI

- **`UUI_Manager`** (`UGameInstanceSubsystem`) — central UI lifecycle manager. `OpenUI(FName, bAddToViewport)`, `CloseUI(FName)`, `ToggleUI(FName)`, `GetUI(FName)`, `UpdateAllUI()`. Maintains `TMap<FName, UUI_Base_Class*> OpenedUIs` and a class registry. Broadcasts `FOnUIUpdate` delegate on state changes.
- **`UUI_Base_Class`** — abstract base inheriting `UUserWidget`.
- **`UUI_Inventory`** — main grid widget, renders `UUI_ItemSlot` cells. Has `AddItemButton` + `AddItemComboBox` (debug-only, hidden unless `bDebugUIVisible`). Context menu on right-click (Use/Discard). Medical items have "治疗" button.
- **`UUI_LootContainer`** — extends `UUI_Inventory`, adds `RefreshButton` (debug-only).
- **`UUI_ItemContextMenu`** — Use/Discard popup. Use supports Weapon/Armor (equip), Food (consume), Medical (heal via `RequestConsumeMedical`).
- **`UUI_Minimap`** — minimap widget, always visible, M to zoom. `UpdateDebugButtonVisibility()` must be `virtual` in base.
- **`UUI_DamageNumber`** — floating damage text widget, floats up + fades out, auto-removes.
- **UI Input Mode**: All managed by PlayerController. `FInputModeGameAndUI` when open, `FInputModeGameOnly` when all closed. `bShowMouseCursor` tracks state.

### Debug System

- **`bDebugUIVisible`** on PlayerController, toggled by `DebugUIAction` (F10). When true, `AddItemButton`, `AddItemComboBox`, and `RefreshButton` become visible on inventory/loot UIs.
- `UUI_Inventory::UpdateDebugButtonVisibility()` (virtual) — checks `APC->IsDebugUIVisible()` and sets button visibility. `UUI_LootContainer` overrides to also handle `RefreshButton`.

### Collision

Custom trace channels (defined in `CayFortressCollisionChannels.h`, must match `Config/DefaultEngine.ini`):
- `CayFortressWeapon` = `ECC_GameTraceChannel1` — weapon hit-scan traces
- `DestructibleObstacle` = `ECC_GameTraceChannel2` — object type for doors/obstacles; AI uses this channel for door detection sweeps

## Blueprint/config coordination

- Input actions: `Content/Input/Actions/` (IA_*.uasset), contexts: `Content/Input/Contexts/`.
- Widget blueprints: `Content/UI/WBP/` subclasses of C++ UI classes.
- Item DataAssets: `Content/Inventory/InventoryItemDataAsset/` subdirectories.
- Furniture blueprints: `Content/LootContainer/BP/` (BP_LCA_ToolKit, BP_LCA_MedicalCabinet, etc.).
- Minimap assets: `BP_MinimapCapture` inherits `AMinimapCaptureActor`, `WBP_UI_Minimap` inherits `UUI_Minimap`.
- GameInstance: `BP_CayFortressGameInstance` (or configured in `DefaultEngine.ini`).
- Core redirect: `BP_Alex` → `Alex_PlayerCharacter` (in `DefaultEngine.ini` `[CoreRedirects]`).
