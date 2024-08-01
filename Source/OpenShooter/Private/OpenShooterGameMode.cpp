// Copyright Epic Games, Inc. All Rights Reserved.

#include "OpenShooterGameMode.h"

#include "UObject/ConstructorHelpers.h"

AOpenShooterGameMode::AOpenShooterGameMode()
{
    // set default pawn class to our Blueprinted character
    static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Characters/Epic/BP_EpicCharacter"));
    if (PlayerPawnBPClass.Class != NULL)
    {
        DefaultPawnClass = PlayerPawnBPClass.Class;
    }
}
