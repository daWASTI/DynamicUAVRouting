// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DynamicUAVRoutingPawn.h"
#include "DynamicUAVRoutingSportsCar.generated.h"

/**
 *  Sports car wheeled vehicle implementation
 */
UCLASS(abstract)
class ADynamicUAVRoutingSportsCar : public ADynamicUAVRoutingPawn
{
	GENERATED_BODY()
	
public:

	ADynamicUAVRoutingSportsCar();
};
