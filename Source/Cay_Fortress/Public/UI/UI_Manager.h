// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UI/UI_Base_Class.h"
#include "UI_Manager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIUpdate, const FName&, UIName);

/**
 * UI管理器
 * 负责管理所有UI的打开、关闭、更新
 */
UCLASS()
class CAY_FORTRESS_API UUI_Manager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	/**
	 * 打开UI
	 * @param UIName UI名称
	 * @param bAddToViewport 是否添加到视口
	 * @return UI实例指针
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	UUI_Base_Class* OpenUI(const FName& UIName, bool bAddToViewport = true);

	/**
	 * 关闭UI
	 * @param UIName UI名称
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void CloseUI(const FName& UIName);

	/**
	 * 切换UI显示状态
	 * @param UIName UI名称
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleUI(const FName& UIName);

	/**
	 * 获取UI实例
	 * @param UIName UI名称
	 * @return UI实例指针
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	UUI_Base_Class* GetUI(const FName& UIName) const;

	/**
	 * 更新所有UI
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateAllUI();

	/**
	 * UI更新委托
	 * 当UI状态改变时触发
	 */
	UPROPERTY(BlueprintAssignable, Category = "UI|Events")
	FOnUIUpdate OnUIUpdate;

protected:
	/** 已打开的UI列表 */
	TMap<FName, UUI_Base_Class*> OpenedUIs;

	/** UI类映射表 */
	TMap<FName, TSubclassOf<UUI_Base_Class>> UIClasses;
};
