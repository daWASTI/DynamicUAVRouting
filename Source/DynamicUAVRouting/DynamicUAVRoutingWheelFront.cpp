// Copyright Epic Games, Inc. All Rights Reserved.

#include "DynamicUAVRoutingWheelFront.h"
#include "UObject/ConstructorHelpers.h"

UDynamicUAVRoutingWheelFront::UDynamicUAVRoutingWheelFront()
{
	AxleType = EAxleType::Front;
	bAffectedBySteering = true;
	MaxSteerAngle = 40.f;
}