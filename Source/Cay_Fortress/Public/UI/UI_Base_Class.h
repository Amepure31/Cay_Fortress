// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_Base_Class.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUIUpdateDelegate);

/**
 * UI基类
 * 所有UI控件的基类
 */
UCLASS()
class CAY_FORTRESS_API UUI_Base_Class : public UUserWidget
{
	GENERATED_BODY()
	
public:
	/**
	 * 初始化UI
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	virtual void InitializeUI();

	/**
	 * 打开UI
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	virtual void OpenUI();

	/**
	 * 关闭UI
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	virtual void CloseUI();

	/**
	 * 更新UI
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	virtual void UpdateUI();

	/**
	 * 检查UI是否打开
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	bool IsUIOpen() const;

	/**
	 * 设置UI名称
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetUIName(const FName& InUIName);

	/**
	 * 获取UI名称
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	FName GetUIName() const;

protected:
	/** UI名称 */
	UPROPERTY()
	FName UIName;

	/** UI是否打开 */
	UPROPERTY()
	bool bIsOpen;

	/** UI更新委托 */
	UPROPERTY(BlueprintAssignable, Category = "UI|Events")
	FOnUIUpdateDelegate OnUIUpdateDelegate;
};
