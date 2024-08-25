// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Character/OpenShooterPlayerController.h"
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "OpenShooterGameMode.generated.h"

class AOpenShooterCharacter;
class AOpenShooterPlayerController;

UCLASS(minimalapi)
class AOpenShooterGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    virtual void PlayerEliminated(AOpenShooterCharacter* EliminatedCharacter, AOpenShooterPlayerController* VictimController,
        AOpenShooterPlayerController* AttackerController);

    virtual void RequestRespawn(ACharacter* EliminatedCharacter, AOpenShooterPlayerController* PlayerController);
};
