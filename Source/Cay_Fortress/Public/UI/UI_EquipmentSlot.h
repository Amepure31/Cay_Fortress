#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/TimerHandle.h"
#include "Styling/SlateBrush.h"
#include "Equipment/EquipmentTypes.h"
#include "UI_EquipmentSlot.generated.h"

class UImage;
class UTextBlock;
class USizeBox;
class UEquipmentComponent;
class UInventoryComponent;
class UInventoryItemInstance;
class UUI_Equipment;
class UDragDropOperation;

UCLASS()
class CAY_FORTRESS_API UUI_EquipmentSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	EEquipmentSlotType SlotType;

	/**
	 * 默认开启：不按格子公式改写 SlotSizeBox / 背景 Brush 尺寸，位置与大小由 UMG 设计器决定。
	 * 关闭后由装备面板按背包单格像素与槽位格数统一缩放（与旧自动堆叠布局一致）。
	 * 若绑定 SlotBackground：默认着色在 UMG 中设置，装备后按物品稀有度改色（与背包格子一致）；提示图仅 UMG，有装备时隐藏。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Layout")
	bool bUseDesignerSlotSize = true;

	/**
	 * 空槽背景 tint 后备值：应与 SlotBackground 在 UMG 里设的 Color 一致（布局未就绪或首次刷新前使用）。
	 * 布局稳定后会从 SlotBackground 再读一次并缓存。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Appearance", meta = (DisplayName = "空槽背景色(后备)"))
	FLinearColor DesignerEmptyBackgroundColor = FLinearColor(0.52f, 0.46f, 0.42f, 1.0f);

	/** 可选：设置后用于固定槽位像素尺寸（与背包单格对齐）。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<USizeBox> SlotSizeBox;

	/** 可选：槽位底图；UMG 设默认颜色与贴图，装备后代码按稀有度改色。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UImage> SlotBackground;

	/** 可选：空槽提示图在 UMG 中手动设贴图与布局；有装备时由代码隐藏。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UImage> SlotHintIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UTextBlock> SlotLabel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UImage> HoverOverlay;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void SetOwningEquipmentPanel(UUI_Equipment* InPanel) { OwningPanel = InPanel; }

	/** 按槽位类型设置格子尺寸与物品图标；背景默认色 UMG，装备后稀有度色。 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void ApplySlotChromeAndSize();

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void RefreshSlotVisual();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	UPROPERTY()
	TObjectPtr<UUI_Equipment> OwningPanel;

	/** 从 UMG SlotBackground 读到的空槽色（几何有效后写入）。 */
	FLinearColor CachedManualSlotBackgroundColor = FLinearColor::White;

	/** 当前非拖拽高亮下的背景色（手动默认或稀有度）。 */
	FLinearColor CachedBackgroundBaseForDrag = FLinearColor::White;

	bool bHasCachedManualSlotBackgroundColor = false;

	/** 装备前 UMG 的 SlotBackground 整份 Brush，卸下时还原，避免稀有度色与深色底图相乘。 */
	bool bHasStoredDesignerSlotBackgroundBrush = false;
	FSlateBrush StoredDesignerSlotBackgroundBrush;

	int32 CachedSlotPixelWidth = 0;
	int32 CachedSlotPixelHeight = 0;

	FTimerHandle DesignerLayoutSyncTimer;

	void EnsureDesignerLayoutSyncTimer();
	void DesignerLayoutSyncTimerElapsed();

	void SetHoverActive(bool bActive);
	void RestoreSlotBackgroundBaseAfterDrag();

	/** 手动尺寸模式：按当前布局计算内容像素（几何与 SizeBox 取较大值），并刷新物品图标 Brush 尺寸。 */
	void SyncDesignerIconBrushesToLayout(bool bForce = false);
};
