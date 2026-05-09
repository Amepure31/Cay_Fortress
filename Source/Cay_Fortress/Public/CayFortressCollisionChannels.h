// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

/**
 * 项目专用碰撞 / 射线通道（与 Config/DefaultEngine.ini 中 CayFortressWeapon 定义一致）。
 * 若改通道槽位，须同步修改 ini 里 ECC_GameTraceChannel 编号。
 */
namespace CayFortressCollision
{
	/** 武器瞄准 / 命中检测专用 Trace（在 ini 中注册为 CayFortressWeapon）。 */
	static constexpr ECollisionChannel WeaponTrace = ECC_GameTraceChannel1;
}
