// Copyright Epic Games, Inc. All Rights Reserved.

#include "GrassGameMode.h"
#include "GrassCharacter.h"
#include "UObject/ConstructorHelpers.h"

AGrassGameMode::AGrassGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
