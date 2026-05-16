#include "UI/UI_WeaponWorkbench.h"
#include "UI/UI_WorkbenchWeaponSlot.h"
#include "BaseBuilding/WeaponWorkbenchActor.h"
#include "BaseBuilding/StorageCabinetActor.h"
#include "BaseBuilding/StorageCabinetTypes.h"
#include "BaseBuilding/BaseBuildingConfigAssets.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/InventoryItemType.h"
#include "Alex_PlayerController.h"
#include "Alex_PlayerCharacter.h"
#include "Inventory/InventoryComponent.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogWeaponWorkbenchUI, Log, All);

template<typename T>
static T* TryLoadDataAsset(const FSoftObjectPath& Path)
{
	if (!Path.IsValid()) return nullptr;
	UObject* Loaded = Path.TryLoad();
	if (!Loaded) return nullptr;
	if (T* Direct = Cast<T>(Loaded)) return Direct;
	if (UBlueprint* BP = Cast<UBlueprint>(Loaded))
		if (BP->GeneratedClass && BP->GeneratedClass->IsChildOf(T::StaticClass()))
			return Cast<T>(BP->GeneratedClass->GetDefaultObject());
	return nullptr;
}

void UUI_WeaponWorkbench::NativeConstruct()
{
	Super::NativeConstruct();
	UE_LOG(LogWeaponWorkbenchUI, Log, TEXT("[WorkbenchUI] NativeConstruct — WeaponSlot=%s, UpgradeButton=%s, CabinetUpgradeButton=%s"),
		WeaponSlot ? TEXT("bound") : TEXT("NULL"),
		UpgradeButton ? TEXT("bound") : TEXT("NULL"),
		CabinetUpgradeButton ? TEXT("bound") : TEXT("NULL"));
	if (WeaponSlot)
		WeaponSlot->SetParentWorkbench(this);
	if (UpgradeButton)
		UpgradeButton->OnClicked.AddDynamic(this, &UUI_WeaponWorkbench::OnUpgradeButtonClicked);
	if (CabinetUpgradeButton)
		CabinetUpgradeButton->OnClicked.AddDynamic(this, &UUI_WeaponWorkbench::OnCabinetUpgradeButtonClicked);
}

void UUI_WeaponWorkbench::BindWeaponWorkbench(AWeaponWorkbenchActor* InWorkbench)
{
	BoundWorkbench = InWorkbench;
	UE_LOG(LogWeaponWorkbenchUI, Log, TEXT("[WorkbenchUI] BindWeaponWorkbench — Actor=%s, pre-bound weapon=%s"),
		InWorkbench ? *InWorkbench->GetName() : TEXT("null"),
		(InWorkbench && InWorkbench->BoundWeapon) ? *InWorkbench->BoundWeapon->GetName() : TEXT("null"));
	if (WeaponSlot && InWorkbench && InWorkbench->BoundWeapon)
		WeaponSlot->RefreshWithWeapon(InWorkbench->BoundWeapon);
	RefreshDisplay();
	BP_OnWorkbenchBound();
}

void UUI_WeaponWorkbench::OnWeaponSlotChanged(UInventoryItemInstance* Weapon)
{
	UE_LOG(LogWeaponWorkbenchUI, Log, TEXT("[WorkbenchUI] OnWeaponSlotChanged — Weapon=%s, BoundWorkbench=%s"),
		Weapon ? *Weapon->GetName() : TEXT("null (clearing)"),
		BoundWorkbench ? *BoundWorkbench->GetName() : TEXT("null"));
	if (BoundWorkbench)
		BoundWorkbench->BindWeapon(Weapon);
	UpdateCostDisplay();
	UpdateButtonColors();
	BP_OnWeaponBound(Weapon);
}

void UUI_WeaponWorkbench::SetActiveModType(EWeaponModType ModType)
{
	ActiveModType = ModType;
	UpdateCostDisplay();
	UpdateButtonColors();
	BP_OnActiveModTypeChanged(ModType);
}

// ========== Cost display ==========

void UUI_WeaponWorkbench::UpdateCostDisplay()
{
	UInventoryItemInstance* Weapon = WeaponSlot ? WeaponSlot->GetWeapon() : nullptr;
	UE_LOG(LogWeaponWorkbenchUI, Verbose, TEXT("[WorkbenchUI] UpdateCostDisplay — Weapon=%s"),
		Weapon ? *Weapon->GetName() : TEXT("null"));
	if (!Weapon)
	{
		UE_LOG(LogWeaponWorkbenchUI, Verbose, TEXT("[WorkbenchUI] UpdateCostDisplay — no weapon, hiding costs"));
		if (CostImage) CostImage->SetVisibility(ESlateVisibility::Collapsed);
		if (CostText) CostText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	FWeaponModConfig Cfg;
	if (GetModConfig(ActiveModType, Cfg) && Cfg.CostItemPath.IsValid() && Cfg.CostAmountPerLevel > 0)
	{
		UInventoryItemDataAsset* DA = TryLoadDataAsset<UInventoryItemDataAsset>(Cfg.CostItemPath);
		if (CostImage && DA && DA->ItemData.Icon)
		{
			FSlateBrush Brush;
			Brush.SetResourceObject(DA->ItemData.Icon);
			Brush.ImageSize = FVector2D(64, 64);
			Brush.DrawAs = ESlateBrushDrawType::Image;
			CostImage->SetBrush(Brush);
			CostImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else if (CostImage)
			CostImage->SetVisibility(ESlateVisibility::Collapsed);

		if (CostText)
		{
			CostText->SetText(FText::Format(FText::FromString(TEXT("x{0}")), FText::AsNumber(Cfg.CostAmountPerLevel)));
			CostText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}
	else
	{
		if (CostImage) CostImage->SetVisibility(ESlateVisibility::Collapsed);
		if (CostText) CostText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UUI_WeaponWorkbench::SetCostSlotWidget(int32 Index, UImage* ImageWidget, UTextBlock* TextWidget, const FSoftObjectPath& ItemPath, int32 Amount)
{
	UInventoryItemDataAsset* DA = TryLoadDataAsset<UInventoryItemDataAsset>(ItemPath);
	if (ImageWidget)
	{
		if (DA && DA->ItemData.Icon)
		{
			int32 OccW = DA->ItemData.Width, OccH = DA->ItemData.Height;
			const int32 MaskLen = DA->ItemData.ShapeMaskData.Num();
			if (MaskLen >= OccW * OccH)
			{
				int32 MinX = OccW, MaxX = 0, MinY = OccH, MaxY = 0;
				for (int32 Y = 0; Y < OccH; ++Y)
					for (int32 X = 0; X < OccW; ++X)
						if (DA->ItemData.ShapeMaskData[Y * OccW + X] != 0)
							{ MinX = FMath::Min(MinX, X); MaxX = FMath::Max(MaxX, X); MinY = FMath::Min(MinY, Y); MaxY = FMath::Max(MaxY, Y); }
				if (MaxX >= MinX && MaxY >= MinY) { OccW = MaxX - MinX + 1; OccH = MaxY - MinY + 1; }
			}
			if (OccW <= 0) OccW = 1; if (OccH <= 0) OccH = 1;
			const float Scale = FMath::Min(100.0f / OccW, 100.0f / OccH);

			FSlateBrush Brush;
			Brush.SetResourceObject(DA->ItemData.Icon);
			Brush.ImageSize = FVector2D(OccW * Scale, OccH * Scale);
			Brush.DrawAs = ESlateBrushDrawType::Image;
			ImageWidget->SetBrush(Brush);
			ImageWidget->SetDesiredSizeOverride(FVector2D(OccW * Scale, OccH * Scale));
			ImageWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
			ImageWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (TextWidget)
	{
		if (DA && Amount > 0)
		{
			TextWidget->SetText(FText::Format(FText::FromString(TEXT("x{0}")), FText::AsNumber(Amount)));
			TextWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
			TextWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UUI_WeaponWorkbench::UpdateCabinetCostDisplay()
{
	TArray<UImage*> Images = { CabinetCostImage1, CabinetCostImage2, CabinetCostImage3 };
	TArray<UTextBlock*> Texts = { CabinetCostText1, CabinetCostText2, CabinetCostText3 };

	TArray<AStorageCabinetActor*> Cabinets = GetNearbyStorageCabinets();
	TArray<FStorageCabinetCostEntry> Costs;
	for (AStorageCabinetActor* C : Cabinets)
		if (C && C->GetCurrentUpgradeCosts(Costs) && Costs.Num() > 0)
			break;

	for (int32 i = 0; i < 3; ++i)
	{
		if (i < Costs.Num())
			SetCostSlotWidget(i, Images[i], Texts[i], Costs[i].ItemPath, Costs[i].Amount);
		else
		{
			if (Images[i]) Images[i]->SetVisibility(ESlateVisibility::Collapsed);
			if (Texts[i]) Texts[i]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UUI_WeaponWorkbench::UpdateButtonColors()
{
	if (UpgradeButton)
	{
		UInventoryItemInstance* Weapon = WeaponSlot ? WeaponSlot->GetWeapon() : nullptr;
		const bool bCan = Weapon && CanApplyMod();
		UpgradeButton->SetColorAndOpacity(bCan
			? FLinearColor(0.1f, 0.8f, 0.1f, 1.0f)
			: FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
	}
	if (CabinetUpgradeButton)
	{
		bool bAny = false;
		for (AStorageCabinetActor* C : GetNearbyStorageCabinets())
			if (CanUpgradeStorageCabinet(C)) { bAny = true; break; }
		CabinetUpgradeButton->SetColorAndOpacity(bAny
			? FLinearColor(0.1f, 0.8f, 0.1f, 1.0f)
			: FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
	}
}

// ========== Refresh ==========

void UUI_WeaponWorkbench::RefreshDisplay()
{
	UpdateCostDisplay();
	UpdateCabinetCostDisplay();
	UpdateButtonColors();
}

// ========== Weapon mod ==========

void UUI_WeaponWorkbench::OnUpgradeButtonClicked()
{
	if (TryApplyWeaponMod())
	{
		UInventoryItemInstance* Weapon = WeaponSlot ? WeaponSlot->GetWeapon() : nullptr;
		UpdateCostDisplay();
		UpdateButtonColors();
		BP_OnWeaponModApplied(Weapon, ActiveModType, GetWeaponModLevel(Weapon, ActiveModType));
	}
}

bool UUI_WeaponWorkbench::TryApplyWeaponMod()
{
	if (!BoundWorkbench) return false;
	UInventoryItemInstance* Weapon = WeaponSlot ? WeaponSlot->GetWeapon() : nullptr;
	if (!Weapon) return false;
	AAlex_PlayerController* PC = GetOwningPlayer<AAlex_PlayerController>();
	if (!PC || !PC->GetPawn()) return false;
	if (BoundWorkbench->ApplyWeaponMod(PC->GetPawn(), Weapon, ActiveModType))
	{
		PC->RefreshPlayerInventoryUI();
		return true;
	}
	return false;
}

int32 UUI_WeaponWorkbench::GetWeaponModLevel(const UInventoryItemInstance* Weapon, EWeaponModType ModType)
{
	return AWeaponWorkbenchActor::GetWeaponModLevel(Weapon, ModType);
}

bool UUI_WeaponWorkbench::GetModConfig(EWeaponModType ModType, FWeaponModConfig& OutConfig) const
{
	return BoundWorkbench ? BoundWorkbench->GetModConfig(ModType, OutConfig) : false;
}

bool UUI_WeaponWorkbench::CanApplyMod() const
{
	if (!BoundWorkbench) return false;
	UInventoryItemInstance* Weapon = WeaponSlot ? WeaponSlot->GetWeapon() : nullptr;
	if (!Weapon) return false;
	AAlex_PlayerController* PC = GetOwningPlayer<AAlex_PlayerController>();
	if (!PC) return false;
	return BoundWorkbench->CanApplyMod(PC->GetPawn(), Weapon, ActiveModType);
}

TArray<FWeaponModConfig> UUI_WeaponWorkbench::GetAllConfigs() const
{
	if (!BoundWorkbench) return {};
	auto* Cfg = TryLoadDataAsset<UWeaponWorkbenchConfigAsset>(BoundWorkbench->WorkbenchConfigAssetPath);
	return Cfg ? Cfg->ModConfigs : BoundWorkbench->ModConfigs;
}

// ========== Storage cabinet upgrade ==========

void UUI_WeaponWorkbench::OnCabinetUpgradeButtonClicked()
{
	for (AStorageCabinetActor* Cabinet : GetNearbyStorageCabinets())
	{
		if (TryUpgradeStorageCabinet(Cabinet))
		{
			UpdateCabinetCostDisplay();
			UpdateButtonColors();
			BP_OnStorageCabinetUpgraded(Cabinet, Cabinet->GetUpgradeLevel());
			return;
		}
	}
}

TArray<AStorageCabinetActor*> UUI_WeaponWorkbench::GetNearbyStorageCabinets() const
{
	TArray<AStorageCabinetActor*> Result;
	if (!GetWorld()) return Result;
	for (TActorIterator<AStorageCabinetActor> It(GetWorld()); It; ++It)
		Result.Add(*It);
	return Result;
}

bool UUI_WeaponWorkbench::TryUpgradeStorageCabinet(AStorageCabinetActor* Cabinet)
{
	if (!BoundWorkbench || !Cabinet) return false;
	AAlex_PlayerController* PC = GetOwningPlayer<AAlex_PlayerController>();
	if (!PC || !PC->GetPawn()) return false;
	return BoundWorkbench->UpgradeStorageCabinet(Cabinet, PC->GetPawn());
}

bool UUI_WeaponWorkbench::CanUpgradeStorageCabinet(AStorageCabinetActor* Cabinet) const
{
	if (!BoundWorkbench || !Cabinet || !Cabinet->GetInventoryComponent()) return false;
	AAlex_PlayerController* PC = GetOwningPlayer<AAlex_PlayerController>();
	if (!PC || !PC->GetPawn()) return false;

	TArray<UInventoryComponent*> Sources;
	if (AAlex_PlayerCharacter* Player = Cast<AAlex_PlayerCharacter>(PC->GetPawn()))
		if (UInventoryComponent* PlayerInv = Player->FindComponentByClass<UInventoryComponent>())
			Sources.Add(PlayerInv);
	if (BoundWorkbench->GetInventoryComponent())
		Sources.Add(BoundWorkbench->GetInventoryComponent());
	if (Cabinet->GetInventoryComponent())
		Sources.AddUnique(Cabinet->GetInventoryComponent());
	for (TActorIterator<AStorageCabinetActor> It(GetWorld()); It; ++It)
	{
		if (*It == Cabinet) continue;
		if (UInventoryComponent* CabInv = (*It)->GetInventoryComponent())
			Sources.AddUnique(CabInv);
	}
	return Cabinet->CanUpgrade(Sources);
}

void UUI_WeaponWorkbench::RequestClose()
{
	AAlex_PlayerController* PC = GetOwningPlayer<AAlex_PlayerController>();
	if (PC) PC->CloseWeaponWorkbenchUI(false);
}
