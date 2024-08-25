// Copyright Epic Games, Inc. All Rights Reserved.

#include "OpenShooterGameMode.h"

#include "Character/OpenShooterCharacter.h"
#include "UObject/ConstructorHelpers.h"

void AOpenShooterGameMode::PlayerEliminated(AOpenShooterCharacter* EliminatedCharacter,
    AOpenShooterPlayerController* VictimController, AOpenShooterPlayerController* AttackerController)
{
    if (EliminatedCharacter)
        EliminatedCharacter->MulticastEliminate();
}
