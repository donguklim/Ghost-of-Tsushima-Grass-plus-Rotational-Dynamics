// Copyright Epic Games, Inc. All Rights Reserved.

#include "GrassFieldGameMode.h"
#include "GrassFieldCharacter.h"
#include "UObject/ConstructorHelpers.h"

AGrassFieldGameMode::AGrassFieldGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
