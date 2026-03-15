// Copyright Epic Games, Inc. All Rights Reserved.

#include "DynamicUAVRoutingGameMode.h"
#include "DynamicUAVRoutingPlayerController.h"

ADynamicUAVRoutingGameMode::ADynamicUAVRoutingGameMode()
{
	PlayerControllerClass = ADynamicUAVRoutingPlayerController::StaticClass();
}
