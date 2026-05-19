# Cay Fortress — 项目介绍

## 项目概览

**Cay Fortress** 是一款基于 **Unreal Engine 5.7** 开发的单人 FPS/生存游戏项目，使用纯 C++ 与 Blueprint 混合架构。项目完全由个人独立开发，涵盖角色控制、战斗系统、拼图式背包、装备系统、敌人 AI、基地建造、跨关卡持久化、小地图等 17 个子系统。

- **开发周期**: 约 3 个月（持续迭代中）
- **代码规模**: 112 个 C++ 源文件（56 `.h` + 56 `.cpp`），383 个 `UFUNCTION`
- **渲染特性**: D3D12 / SM6 / Lumen GI / 硬件 Ray Tracing / Virtual Shadow Maps / Substrate 材质
- **玩法类型**: FPS 生存射击 + 资源管理 + 基地养成

---

## 环境要求与构建

### 运行环境

- **操作系统**: Windows 10/11 (x64)
- **IDE**: JetBrains Rider 或 Visual Studio 2022
- **Unreal Engine**: 5.7（安装于 `D:\UE\UE_5.7`）

### 命令行构建

```powershell
& "D:\UE\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" Cay_FortressEditor Win64 Development -Project="W:\UE5\Cay_Fortress\Cay_Fortress.uproject" -WaitMutex
```

### 编辑器运行

直接打开 `Cay_Fortress.uproject`，UE 编辑器会自动编译。也可在编辑器中使用 **Live Coding** (Ctrl+Alt+F11) 进行热重载。

### 测试

当前无自动化测试，所有功能验证均在编辑器 PIE (Play In Editor) 中完成。

---

## 技术架构

### 模块结构

单模块 `Cay_Fortress`（Runtime），依赖：
- Public: `Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`, `UMG`, `AIModule`, `NavigationSystem`
- Private: `Slate`, `SlateCore`, `AssetRegistry`, `GameplayTasks`

### 核心设计模式

- **GameInstanceSubsystem** — `UUI_Manager` 和 `UInventoryPersistenceSubsystem` 均继承自 `UGameInstanceSubsystem`，生命周期绑定 GameInstance，跨关卡存活，是 UE5 推荐的全局状态管理模式。
- **事件驱动解耦** — 角色通过 17+ `DYNAMIC_MULTICAST_DELEGATE` 广播状态变化（生命值/体力/饥饿/水分/负重变更、攻击/换弹/闪避蒙太奇结束），UI 绑定委托响应，PlayerCharacter 对 UI 零引用。
- **BlueprintNativeEvent 接口** — 交互系统 (`IInteractableInterface`)、可破坏障碍物 (`IDestructibleObstacle`)、容器刷新 (`ALootContainerActor::RefreshItemsByRoom`) 均使用 `BlueprintNativeEvent`，C++ 提供默认实现，蓝图可覆盖行为。
- **UPrimaryDataAsset 配置** — 物品、武器工作台、训练机、储藏柜的数值均通过 DataAsset 配置，策划可直接在编辑器中调整而不碰代码。

---

## 子系统详解

### 1. 拼图式背包系统（Inventory）

**这是整个项目技术复杂度最高的子系统**，实现了类似《逃离塔科夫》的异形物品网格背包。

**数据结构**：
- `FFItemShapeMask` — 基于位掩码的物品形状表示，支持任意宽高的异形轮廓，物品旋转（0°/90°/180°/270°）通过矩阵转置实现
- `FInventoryGridCell` — 单个网格单元状态（含物品指针和单元格归属）
- `UInventoryItemInstance` — 运行时物品对象，持有形状掩码、旋转四分之一圈数、耐久度、武器弹匣弹药量、**武器改装属性覆写**（工作台改枪后持久保存）

**核心算法**：
- `AddItemAtPosition()` — 带形状冲突检测的放置验证（逐单元格遍历掩码，检测越界和占用）
- `IsSpaceAvailable()` — 支持忽略自身的空间检测（拖动物品时排除自身占用的格子）
- `FindSpaceForItem()` — 自动寻空位放置（扫描 + 合法性检测）
- `DetachItemInstance()` / `AttachItemInstance()` — 物品在背包/装备间转移时不解构 UObject，仅改变归属
- **跨背包移动的正确顺序**：必须先 Detach 再 Add，反序会导致 SlotX/Y 被重置为 -1 而无法放入目标背包

**物品类型体系**（10 种）：武器、护甲、弹药、食物、医疗、能源、材料、配方、任务、其他

**稀有度等级**（6 档）：Normal / Uncommon / Rare / Epic / Legendary / Mythic，每档有独立颜色标识

**武器属性**（12 维）：伤害、射速、弹匣容量、水平/垂直后坐力、射程、射击模式（单发/全自动）、装填时间、腰射精度、ADS 精度、武器类别、操控性

**7 种武器类别 → 6 种弹药类型的映射**：SMG/Rifle/LMG → Rifle 弹药；Pistol/Sniper/Shotgun/Other 各自独立弹药

### 2. 装备系统（Equipment）

5 个装备槽位：头部、胸甲、背包、武器 1、武器 2。每个槽位有独立的网格大小配置。

**容器背包**：护甲物品标记 `bIsContainer=true` 后，内部持有独立的 `UInventoryComponent` 子网格，形成**嵌套背包树**。持久化系统支持完整保留嵌套结构（通过父物品索引映射子物品数组）。

### 3. 弹药子系统

- **弹匣弹药 vs. 散装弹药分离**：武器弹匣内的弹药与背包中散装弹药独立追踪
- `TryUnloadWeaponMagazineToLooseAmmo()` — 将弹匣弹药按单粒转换为散装物品
- **配套弹药自动发放**：装备武器时可通过配置自动获得对应弹药物品
- `DefaultAmmoItemByType` — 运行时通过 AssetRegistry 自动发现匹配的弹药 DataAsset

### 4. 玩家属性系统（5 维度）

| 属性 | 正向变化 | 负向变化 | 边界触发 |
|---|---|---|---|
| 生命值 | 医疗物品治疗 | 敌人攻击（含护甲穿透） | ≤0 死亡 |
| 体力 | 静止自动恢复 | 冲刺消耗 (1.0/s)、闪避消耗 (25/次) | =0 强制步行 |
| 饥饿度 | 食物恢复 | 持续消耗 (HungerDrainRate) | =0 惩罚状态 |
| 水分 | 饮水恢复 | 持续消耗 (HydrationDrainRate) | =0 惩罚状态 |
| 负重 | 丢弃物品 | 背包+装备物品重量累计 | 超限移动减速 |

每个属性变化均广播独立委托，UI 零耦合绑定更新。

### 5. 战斗系统

**近战**：
- `PerformMeleeHitDetection()` — 前方球形扫描（180 范围 / 90 半径），`ECC_Pawn` 通道
- 通过 `UAnimNotify_PlayerMeleeAttack` 在攻击蒙太奇精确帧触发
- 过滤死亡敌人和非敌人目标，可配置伤害/范围/半径

**远程（ADS Hit-scan）**：
- 相机 → 准星方向的 `CayFortressWeapon` 自定义碰撞通道射线检测
- 支持武器全自动连发（Timer 驱动，射速可配置）
- **弹着点骨骼伤害倍率**：命中敌人后通过 Physics Asset 解析命中骨骼 → 映射到身体区域 → 应用倍率（头部 2×，躯干 1×，手臂 0.85×，腿部 0.75×）
- 伤害数字仅对敌人显示（`Cast<AEnemyCharacter>` 守卫），头部浮动红色数字，其他白色

**闪避/翻滚**：
- 消耗 25 体力，可连闪（combo window），触发后进入无敌帧
- 两段蒙太奇拼接播放，结束时广播 `FOnAlexDodgeMontageFinished`

**UI 阻断**：任意 UI 开启时（`bShowMouseCursor == true`），攻击输入直接返回，杜绝界面操作被战斗打断

**换弹**：消耗背包中匹配弹药类型的散装弹药填充弹匣，换弹蒙太奇结束时广播 `FOnAlexReloadMontageFinished`

### 6. 敌人 AI

**行为状态机**（4 状态）：Roam（漫游）→ Alert（警戒）→ Chase（追逐）→ Attack（攻击）

**AI 感知**：
- 视觉：2400cm 半径，80° 半角
- 听觉：500cm 半径（玩家冲刺每 0.5s 以 3m 范围发射噪声事件）
- **连锁警报**：敌人受伤时，500cm 内所有敌人通过 `ForceDetectPlayer()` 立即感知玩家

**行为树 + 控制器协同**：
- Behavior Tree 管理高层决策（状态切换、蒙太奇播放）
- C++ Controller 驱动底层行为（连续追逐、破门检测、攻击循环）
- Blackboard 数据同步：`bCanSeePlayer`, `CurrentBehaviorState`, `SuspicionLevel`, `LastKnownPlayerLocation`, `DoorTarget`

**漫游**：以出生点为锚点，随机选取 `RoamWanderRadius`（1800cm）内可达 NavMesh 点

**警戒**：线性累积怀疑值（0→100），满后转入 Chase。怀疑值在看不到玩家时自动衰减

**追逐系统**（控制器驱动连续追逐）：
- 每 0.3s 重新计算 MoveTo 路径（`bDriveChaseFromController`），无需等待行为树 MoveTo 节点完成即可中途改道
- 丢失视野后切回 Alert 状态（保持 92% 怀疑值），前往 `LastKnownPlayerLocation` 调查

**攻击**：进入 `AttackTriggerRange`（220cm）后触发，`AttackIntervalSeconds`（1.2s）冷却。通过 `UAnimNotify_EnemyAttackDamage` 在攻击蒙太奇精确帧判定伤害

**破门 AI**：
- 追击中通过球形扫描（400cm 半径，`DestructibleObstacle` 通道）检测挡路门
- 导航到门 → 切换 Attack 状态 → 每次攻击 50 伤害（门 HP 150，3 击破坏）
- 门破坏后立即恢复追逐

**其他 AI 特性**：
- 受击硬直（0.35s 眩晕，抑制移动）
- Alert→Chase 尖叫蒙太奇（抑制追逐移动直到播放完毕）
- 攻击后咆哮蒙太奇
- 死亡：胶囊体收缩释放 NavMesh，骨骼网格立即忽略武器射线通道（尸体不挡子弹），全局最多保留 15 具尸体
- 敌人胶囊体和骨骼网格忽略 `ECC_Camera` 通道，防止敌人走在玩家和摄像机之间时摄像机回缩

**导航**：Recast NavMesh 设置为 Dynamic 运行时重建模式，门破坏后自动重新生成导航网格。

### 7. 可破坏障碍物系统

- `IDestructibleObstacle` 接口：`CanBeDestroyedByAI` / `TakeAIAttackDamage` / `IsObstacleDestroyed` / `GetObstacleLocation`（全部 `BlueprintNativeEvent`）
- `ADoorActor` 实现该接口，使用 `DestructibleObstacle` 碰撞对象类型（`ECC_GameTraceChannel2`）
- 门碰撞挖空 NavMesh，破坏后移除碰撞组件，触发动态导航重建
- 门网格忽略 `ECC_Visibility` 通道（AI 视线可穿透门），避免敌人被门遮挡丢失感知
- `BlueprintImplementableEvent`: `OnDoorTakeDamage`, `OnDoorDestroyed`（用于蓝图实现音效/特效）

### 8. 交互系统

**`IInteractableInterface`** — 统一交互接口：
- `CanInteract(AActor* Interactor)` — 判断是否可被交互
- `Interact(AActor* Interactor)` — 执行交互逻辑
- `GetInteractText()` — 返回交互提示文本
- 全部为 `BlueprintNativeEvent`，C++ 提供默认实现，蓝图可覆盖

**`UPlayerInteractComponent`** — 挂载于 PlayerCharacter，每 Tick 执行：
- 球形扫描（250cm 范围，20cm 半径，`ECC_Visibility` 通道）探测可交互目标
- 自动管理交互提示 Widget（跟随目标位置偏移显示）
- 支持调试模式（屏幕显示检测到的可交互对象）
- 按 E 键触发 `TryInteract()`

**实现 `IInteractableInterface` 的类**：
- `ADoorActor` — 玩家可手动开门
- `ALootContainerActor` — 搜索战利品
- `ATeleportNode` — 跨关卡传送
- `AWeaponWorkbenchActor` — 武器改装
- `ATrainingMachineActor` — 属性训练
- `AStorageCabinetActor` — 储藏柜
- `ABedActor` — 床铺（蓝图事件 `BP_OnBedUsed`）

**搜刮进度条系统**：
- 搜刮战利品容器需按住 E 键约 3 秒
- 按住 Shift 加速搜刮（双倍速度）
- `UUI_LootProgressBar` 实时显示进度
- 提前松键取消搜刮，防止战斗中误触
- 进度完成→打开战利品 UI→显示容器物品

### 9. 房间 / 战利品容器系统

**10 种容器类型（`EContainerType`）**：
| 值 | 类型 | 刷新逻辑 |
|---|---|---|
| 0 | 食物箱 (FoodCrate) | 食物 0-3 件，稀有度随机 |
| 1 | 武器箱 (WeaponCrate) | 武器 |
| 2 | 医疗箱 (MedicalCrate) | 医疗物品 0-3 件 |
| 3 | 马桶 (Toilet) | 75% 概率出淡水 |
| 4 | 能源箱 (EnergyCrate) | 能源物品 |
| 5 | 材料箱 (MaterialCrate) | 材料物品 |
| 6 | 弹药箱 (AmmoCrate) | 弹药 |
| 7 | 护甲箱 (ArmorCrate) | 护甲物品 |
| 8 | 装备箱 (EquipmentCrate) | 头盔或护甲 |
| 9 | 工具箱 (ToolKit) | 弹药 + 材料 0-3 件 |

**房间级编排（`ARoomActor`）**：
- `BeginPlay` 自动收集挂载的容器 Actor
- **两阶段刷新**：Phase 1 — 概率激活/停用容器网格体（每种容器类型独立概率，通过 `ContainerRefreshChanceByType` 配置）；Phase 2 — 通过 AssetRegistry 扫描匹配 DataAsset 填充物品
- 追踪房间内所有容器的开箱/搜刮状态，广播 `FOnRoomContainersOpenedStateChanged` / `FOnRoomContainersLootedStateChanged` 委托
- `EvaluateRoomState()` 实时计算 `bAllContainersOpened` / `bAllContainersLooted`

### 10. 基地建造 / 家具系统

三种家具共享交互模式：靠近后按 E 打开专属 UI，走远自动关闭，再按 E 切换关闭。每种家具有独立的 `UPrimaryDataAsset` 配置资产。

**武器工作台（`AWeaponWorkbenchActor`）**：
- 9 种改装类型（伤害、射速、弹匣容量、后坐力、射程、装填速度、腰射精度、ADS 精度、操控性）
- 每级提供叠加型或倍率型加成
- 武器"绑定"到工作台后改装生效，`FWeaponStatOverrides` 覆写写入物品实例，持久化保留
- 支持工作台自身网格升级

**训练机（`ATrainingMachineActor`）**：
- 6 种可训练属性（最大生命/体力/体力恢复/最大饥饿/最大水分/最大负重）
- 升级等级标记 `UPROPERTY(SaveGame)`，**独立于跨关卡持久化系统**，即使删档也会保留训练成果

**储藏柜（`AStorageCabinetActor`）**：
- 网格扩展升级树，每级材料成本递增
- 内部持有独立背包网格，可作为基地仓库

**床（`ABedActor`）**：
- 实现 `IInteractableInterface`
- `BP_OnBedUsed` 蓝图事件用于实现睡眠/存档等逻辑

### 11. 跨关卡持久化（Persistence）

`UInventoryPersistenceSubsystem`（GameInstanceSubsystem）在 OpenLevel/ServerTravel 前捕获完整玩家状态，在新关卡恢复：

- **捕获范围**：所有背包物品（含形状、旋转、堆叠、耐久、弹匣弹药、武器改装覆写、改装等级）、所有装备槽位（含已装备物品完整数据）、容器背包嵌套物品、5 维属性（含最大值和消耗率）、网格尺寸配置
- **序列化结构**：
  - `FPersistedItemData` — 单个物品：DataAsset 路径、堆叠、耐久、网格位置、形状、旋转、弹匣弹药量、武器改装覆写
  - `FPersistedEquipmentSlot` — 装备槽位数据
  - `FPersistedPlayerStats` — 玩家属性（含最大值和消耗率）
  - `FPersistedItemDataArray` — 嵌套背包映射的包装器
- **序列化方式**：使用 `FSoftObjectPath` 引用 DataAsset，通过 UPROPERTY 标记让 UE 序列化系统自动处理
- **触发时机**：`ATeleportNode` 交互 → `CaptureFromPlayer()` → `OpenLevel` → `RestoreToPlayer()`
- `UCayFortressGameInstance` 持有 `PendingTeleportNodeID` 和 `LastLevelName` 用于传送状态协调
- `HasCapturedData()` 可用于检查是否有待恢复的数据

### 12. UI 系统

22 个 Widget 类，继承自 `UUI_Base_Class`，统一 `InitializeUI / OpenUI / CloseUI / UpdateUI` 虚函数管线。

`UUI_Manager`（GameInstanceSubsystem）集中管理所有 UI 的打开/关闭/切换/获取/全局刷新，维护名称→实例映射表和蓝图类注册表，广播 `FOnUIUpdate` 委托通知状态变更。

**完整 Widget 列表**：

| Widget | 功能 |
|---|---|
| `UUI_Inventory` | 主背包网格，含 Debug 按钮（F10 切换可见） |
| `UUI_ItemSlot` | 背包单个格子渲染 |
| `UUI_ItemWidget` | 物品图标/堆叠数/耐久显示 |
| `UUI_ItemTooltip` | 物品悬浮提示（属性详情） |
| `UUI_ItemContextMenu` | 右键菜单（使用/丢弃） |
| `UUI_Equipment` | 装备面板（5 槽位） |
| `UUI_EquipmentSlot` | 单个装备槽位 |
| `UUI_LootContainer` | 战利品容器界面（继承 UUI_Inventory） |
| `UUI_LootProgressBar` | 搜刮进度条（按住 E 累积） |
| `UUI_AmmoHUD` | 弹药 HUD（弹匣/备弹数） |
| `UUI_AimPoint` | 十字准星 |
| `UUI_Minimap` | 小地图（SceneCapture2D 实时渲染） |
| `UUI_DamageNumber` | 浮动伤害数字（漂移+淡出） |
| `UUI_InteractPrompt` | 交互提示文本（跟随目标） |
| `UUI_CharacterProperty` | 角色属性面板 |
| `UUI_WeaponWorkbench` | 武器工作台改装界面 |
| `UUI_WorkbenchWeaponSlot` | 工作台武器槽位 |
| `UUI_TrainingMachine` | 训练机升级界面 |
| `UUI_StorageCabinet` | 储藏柜界面 |
| `UUI_ContainerBackpack` | 容器背包子界面 |
| `UUI_Base_Class` | 抽象基类 |
| `UUI_Manager` | 全局 UI 管理器（非 Widget，是 Subsystem） |

**关键 UI 技术点**：
- 背包拖拽：物品形状感知的放置预览（绿色可放/红色不可放），右击呼出右键菜单，按住 R 卸载弹匣，拖出背包区域丢弃
- 战利品容器：按住进度条防误触机制
- 小地图：SceneCapture2D 正交俯视实时渲染（5000 世界单位宽，60Hz 刷新），仅显示静态世界几何（禁用光照/阴影/骨骼网格的 ShowFlags），M 键 2.5× 缩放，敌人指示器（红色箭头，10Hz 追踪，800cm 向上可见性射线过滤室内敌人），`HitTestInvisible` 避免吞噬鼠标点击
- 伤害数字：世界空间定位浮动，向上漂移 + 淡出动画，自动移除
- UI 输入模式：`FInputModeGameAndUI`（UI 开启时）/ `FInputModeGameOnly`（全部关闭时），`bShowMouseCursor` 追踪全局状态
- 小地图自动隐藏：任意 UI 开启时（`bShowMouseCursor == true`），小地图自动隐藏

### 13. 动画系统

**Player AnimInstance（`AAlex_AnimInstance`）**：
- 21 个 `BlueprintThreadSafe` 属性读取函数
- 手枪/步枪 ADS 分层混合（AnimGraph Layered blend per bone）
- 平滑移动数据（速度/方向/是否滞空）
- 读取 `bIsAiming`、武器类型、ADS 层权重

**Enemy AnimInstance（`UEnemyAnimInstance`）**：
- AI 行为状态同步到动画状态机
- 5 类蒙太奇槽位：尖叫、攻击、攻击后咆哮、受击、死亡
- 行为速度平滑插值

**AnimNotify 类**：
- `UAnimNotify_PlayerMeleeAttack` — 在近战攻击蒙太奇精确帧触发 `PerformMeleeHitDetection()`
- `UAnimNotify_EnemyAttackDamage` — 在敌人攻击蒙太奇精确帧触发伤害判定

**蒙太奇结束委托**：
- `FOnAlexAttackMontageFinished` — 攻击蒙太奇结束（混出或被打断）
- `FOnAlexReloadMontageFinished` — 换弹蒙太奇结束
- `FOnAlexDodgeMontageFinished` — 闪避蒙太奇结束
- 用于门控状态转换（如换弹完成才能再次射击）

### 14. 小地图系统

- `AMinimapCaptureActor` — 顶部正交 `SceneCaptureComponent2D`，5000 世界单位宽，~60Hz 捕获，跟随玩家 XY 坐标
- ShowFlags 配置：仅显示静态世界几何（禁用光照/阴影/后处理/骨骼网格/粒子效果），隐藏玩家 Pawn
- `UUI_Minimap` — Widget 渲染到屏幕右上角，z-order=1，`HitTestInvisible` 避免吞噬鼠标
- 功能：`PlayerIndicator`（旋转箭头，Yaw-90° 修正），敌人标记（红色箭头，10Hz 追踪，仅限室外可见敌人），M 键切换 2.5× 缩放
- 配置：`MapRadius = 200`, `WorldToPixelScale = 0.08`, 400×400 RenderTarget

### 15. 传送系统

- `ATeleportNode` — 实现 `IInteractableInterface`，玩家交互触发跨关卡传送
- 从 DataAsset/蓝图配置读取目标关卡
- 传送前自动调用 `UInventoryPersistenceSubsystem::CaptureFromPlayer()`
- `UCayFortressGameInstance` 存储传送节点 ID 和来源关卡名，用于在新关卡定位玩家

### 16. 调试系统

- `bDebugUIVisible` — PlayerController 上的标志，F10 切换
- Debug 模式下背包/战利品 UI 显示隐藏按钮：
  - `AddItemButton` + `AddItemComboBox` — 任意添加物品（调试用）
  - `RefreshButton` — 刷新容器战利品
- `UUI_Inventory::UpdateDebugButtonVisibility()`（virtual）— 控制按钮可见性
- `UUI_LootContainer` override — 额外控制 RefreshButton

### 17. 碰撞通道

两个自定义碰撞通道（`CayFortressCollisionChannels.h` 与 `Config/DefaultEngine.ini` 须保持一致）：

| 通道 | 类型 | 用途 |
|---|---|---|
| `CayFortressWeapon` | Trace Channel (ECC_GameTraceChannel1) | 武器 hit-scan 命中检测 |
| `DestructibleObstacle` | Object Type (ECC_GameTraceChannel2) | 可破坏障碍物，AI 破门检测 |

---

## 技术亮点

1. **异形拼图背包** — 从零实现的二维网格物品系统，支持不规则形状、旋转、形状感知拖拽预览。核心算法涉及逐单元格掩码遍历的空间检测，是空间推理在游戏编程中的典型应用。

2. **武器改装覆写管线** — `FWeaponStatOverrides` 为每个实例提供独立的属性覆写层，查询时通过 `GetEffectiveWeaponStats()` 动态合并基础值与覆写值，避免修改静态 DataAsset。

3. **嵌套背包树** — 护甲作为容器内嵌独立背包，形成父子背包树结构。持久化系统通过 `TMap<int32, FPersistedItemDataArray>` 键控嵌套关系，完整保留跨关卡容器物品。

4. **控制器驱动连续追逐 AI** — 在 Chase 状态下每 0.3s 由 C++ 控制器主动更新 MoveTo 路径，不被行为树 MoveTo 节点的完成条件阻塞，配合破门检测/攻击/恢复流程，实现追逐中动态适应环境破坏。

5. **GameInstanceSubsystem 双层持久化** — `UUI_Manager` 和 `UInventoryPersistenceSubsystem` 均挂载在 GameInstance 上，在 `OpenLevel`/`ServerTravel` 导致 World 销毁时自动存活，符合 UE5 推荐的跨关卡状态管理范式。

6. **事件驱动 UI 解耦** — PlayerCharacter 通过 12 个属性变更委托 + 3 个蒙太奇结束委托广播状态，UI 端绑定响应，Character 类完全不持有 UI 引用，避免了传统做法中的紧耦合。

7. **DataAsset 驱动的数值配置** — 物品属性、工作台改装曲线、训练机升级数值、储藏柜升级成本全部通过 `UPrimaryDataAsset` 在编辑器中配置，运行时零硬编码数值。

8. **Physics Asset 骨骼伤害映射** — 武器 hit-scan 命中敌人后解析 Physics Asset 骨骼名称，自动映射到身体区域（头/躯干/手臂/腿部），无需为每种敌人手动配置伤害区域。

9. **AI 破门 + 动态导航** — 敌人自主检测并破坏障碍物，门破坏后触发 NavMesh 动态重建，AI 寻路无缝适应关卡变化。

10. **空间感知的小地图敌人标记** — 小地图通过 SceneCapture2D 渲染世界几何 + CPU 端向上可见性射线检测（800cm）过滤室内敌人，实现仅显示室外敌人的智能标记。

---

## 项目文件结构

```
Cay_Fortress/
├── Source/Cay_Fortress/
│   ├── Public/
│   │   ├── Alex_PlayerCharacter.h        # 玩家角色（70 UFUNCTION）
│   │   ├── Alex_PlayerController.h      # 玩家控制器（UI/输入/交互管理）
│   │   ├── Alex_GameMode.h              # 游戏模式
│   │   ├── Alex_AnimInstance.h          # 玩家动画蓝图基类
│   │   ├── CayFortressCollisionChannels.h # 自定义碰撞通道定义
│   │   ├── MinimapCaptureActor.h        # 小地图场景捕获
│   │   ├── DoorActor.h                  # 门/可破坏障碍物
│   │   ├── DestructibleObstacleInterface.h # 可破坏障碍物接口
│   │   ├── Inventory/
│   │   │   ├── InventoryComponent.h      # 背包核心（28 UFUNCTION）
│   │   │   ├── FInventoryItemInstance.h  # 运行时物品实例
│   │   │   ├── InventoryItemDataAsset.h  # 物品数据资产（USTRUCT 定义）
│   │   │   ├── FInventoryGridCell.h      # 网格单元
│   │   │   ├── FItemShapeMask.h          # 异形形状掩码
│   │   │   ├── InventoryItemType.h       # 物品类型枚举
│   │   │   └── InventoryItemRarity.h     # 稀有度枚举
│   │   ├── Equipment/
│   │   │   ├── EquipmentComponent.h      # 装备槽位管理
│   │   │   └── EquipmentTypes.h          # 槽位类型枚举
│   │   ├── Enemy/
│   │   │   ├── EnemyCharacter.h          # 敌人角色
│   │   │   ├── EnemyAIController.h       # AI 控制器
│   │   │   ├── EnemyAnimInstance.h       # 敌人动画蓝图基类
│   │   │   ├── EnemyBehavior.h           # 行为状态枚举
│   │   │   ├── BTS_UpdateSuspicion.h     # 行为树服务（警戒值更新）
│   │   │   ├── EnemyBehaviorBlueprintLibrary.h # 行为蓝图库
│   │   │   └── AnimNotify_EnemyAttackDamage.h  # 敌人攻击判定通知
│   │   ├── BaseBuilding/
│   │   │   ├── WeaponWorkbenchActor.h    # 武器工作台
│   │   │   ├── WeaponWorkbenchTypes.h    # 改装类型/属性覆写
│   │   │   ├── TrainingMachineActor.h    # 训练机
│   │   │   ├── TrainingMachineTypes.h    # 训练属性类型
│   │   │   ├── StorageCabinetActor.h     # 储藏柜
│   │   │   ├── StorageCabinetTypes.h     # 储藏柜类型
│   │   │   ├── BedActor.h               # 床
│   │   │   └── BaseBuildingConfigAssets.h # 家具配置 DataAsset
│   │   ├── LootContainer/
│   │   │   ├── LootContainerActor.h      # 战利品容器
│   │   │   ├── RoomActor.h              # 房间（容器编排）
│   │   │   ├── ContainerType.h          # 容器类型枚举
│   │   │   └── ContainerGenerationTag.h  # 容器生成标签
│   │   ├── UI/
│   │   │   ├── UI_Manager.h             # 全局 UI 管理器（GameInstanceSubsystem）
│   │   │   ├── UI_Base_Class.h          # UI 抽象基类
│   │   │   ├── UI_Inventory.h           # 背包主界面
│   │   │   ├── UI_LootContainer.h       # 战利品容器界面
│   │   │   ├── UI_ItemSlot.h            # 物品格子
│   │   │   ├── UI_ItemWidget.h          # 物品控件
│   │   │   ├── UI_ItemTooltip.h         # 物品提示
│   │   │   ├── UI_ItemContextMenu.h     # 右键菜单
│   │   │   ├── UI_Equipment.h           # 装备面板
│   │   │   ├── UI_EquipmentSlot.h       # 装备槽位
│   │   │   ├── UI_AmmoHUD.h             # 弹药 HUD
│   │   │   ├── UI_AimPoint.h            # 十字准星
│   │   │   ├── UI_Minimap.h             # 小地图
│   │   │   ├── UI_DamageNumber.h        # 伤害数字
│   │   │   ├── UI_InteractPrompt.h      # 交互提示
│   │   │   ├── UI_CharacterProperty.h   # 属性面板
│   │   │   ├── UI_WeaponWorkbench.h     # 武器工作台 UI
│   │   │   ├── UI_WorkbenchWeaponSlot.h # 工作台武器槽
│   │   │   ├── UI_TrainingMachine.h     # 训练机 UI
│   │   │   ├── UI_StorageCabinet.h      # 储藏柜 UI
│   │   │   ├── UI_ContainerBackpack.h   # 容器背包 UI
│   │   │   └── UI_LootProgressBar.h     # 搜刮进度条
│   │   ├── Interaction/
│   │   │   ├── InteractableInterface.h  # 交互接口
│   │   │   └── PlayerInteractComponent.h # 玩家交互组件
│   │   ├── Persistence/
│   │   │   └── InventoryPersistenceSubsystem.h # 跨关卡持久化
│   │   ├── Teleport/
│   │   │   ├── TeleportNode.h           # 传送节点
│   │   │   └── CayFortressGameInstance.h # 自定义 GameInstance
│   │   ├── AnimNotify_PlayerMeleeAttack.h # 玩家近战判定通知
│   │   └── Cay_FortressLog.h            # 日志分类定义
│   └── Private/                          # 对应 .cpp 实现
├── Content/
│   ├── Map/                              # 关卡（Home, TestMap 等）
│   ├── Inventory/InventoryItemDataAsset/ # 物品 DataAsset（Weapon/Armor/Ammo/Food/Medical/Materials 子目录）
│   ├── UI/WBP/                           # UI 蓝图 Widget
│   ├── Input/Actions/                    # Enhanced Input Actions (IA_*.uasset)
│   ├── Input/Contexts/                   # Input Mapping Context (IMC_Player)
│   ├── Characters/Mannequins/            # 角色模型/骨骼/动画
│   ├── LootContainer/BP/                 # 容器蓝图
│   └── Basics/DA/                        # 基础 DataAsset
├── Config/
│   └── DefaultEngine.ini                 # 引擎配置（碰撞通道、GameInstance 类、核心重定向）
├── .idea/                                # Rider 项目文件
├── Cay_Fortress.sln                      # Visual Studio 解决方案
└── Cay_Fortress.uproject                 # UE 项目文件
```

---

## 技术栈

| 领域 | 技术 |
|---|---|
| 语言 | C++（主要逻辑）、Blueprint（UI 绑定和配置） |
| 引擎 | Unreal Engine 5.7 |
| 渲染 | D3D12 / SM6 / Lumen GI / 硬件 Ray Tracing / Virtual Shadow Maps / Substrate 材质 |
| AI | Behavior Tree + Blackboard + AI Perception (Sight + Hearing) + NavMesh Dynamic Rebuild + EQS |
| UI | UMG + Slate + WidgetComponent（世界空间 Widget） |
| 动画 | AnimBlueprint + AnimMontage + AnimNotify + Layered Blend Per Bone + BlueprintThreadSafe 属性访问 |
| 配置 | UPrimaryDataAsset + Blueprint 子类化 |
| 碰撞 | 自定义 Object Channel + Trace Channel |
| 输入 | Enhanced Input System（10+ InputActions，IMC_Player） |
| 持久化 | GameInstanceSubsystem + UPROPERTY 序列化 + FSoftObjectPath |
| 构建 | UnrealBuildTool (UBT) + Live Coding |

---

## 开发注意事项

以下是在开发过程中积累的关键注意事项，修改相关代码时必须遵守：

1. **DataAsset 修改警告**：修改 `InventoryItemDataAsset.h` 中的 USTRUCT（如 `FInventoryItemData`、`FArmorItemStats`、`FFoodItemStats`）前，必须警告用户。结构体布局变更可能破坏已序列化的蓝图 DataAsset 实例。

2. **Slate 输入路由**：禁止在 Slate 事件路由期间调用 `SetInputMode`（如在 `NativeOnPreviewKeyDown`、`NativeOnMouseButtonDown` 中）。如需在 Slate 事件中打开/关闭 UI，通过 `GetWorld()->GetTimerManager().SetTimerForNextTick()` 延迟到下一帧。中途改变输入模式会破坏 Slate 内部状态，导致摄像机和鼠标控制永久丢失。

3. **Unity Build 冲突**：不要在不同 `.cpp` 文件中定义同名的 `static` 函数或匿名命名空间函数。UE 会将模块内的 `.cpp` 文件合并为统一编译单元，重复定义会冲突。将共享内部辅助函数放入 `Private/` 目录下的 `#pragma once` 头文件。

4. **UE5.7 TWeakObjectPtr**：从原始 `T*` 指针赋值给 `TWeakObjectPtr<T>` 时，需确保类型 T 的完整定义可见，前向声明不足。如果头文件中只有前向声明，将函数体移到 `.cpp`。

5. **UE5.7 TObjectPtr**：无 `.IsValid()` 方法，使用 `!= nullptr` 或 `IsValid(Ptr.Get())`。

6. **小地图 Widget 可见性**：`UUI_Minimap` 必须使用 `HitTestInvisible`（非 `Visible`），否则会在右上角区域吞噬鼠标点击。

---

*项目持续开发中。代码仓库：内部 Git 管理。*
