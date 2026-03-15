// Copyright Epic Games, Inc. All Rights Reserved.

#include "DynamicUAVRoutingWheelRear.h"
#include "UObject/ConstructorHelpers.h"

UDynamicUAVRoutingWheelRear::UDynamicUAVRoutingWheelRear()
{
	AxleType = EAxleType::Rear;
	bAffectedByHandbrake = true;
	bAffectedByEngine = true;
}