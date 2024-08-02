// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"

#include "LobbyGameMode.generated.h"

/**
 *
 */
UCLASS()
class OPENSHOOTER_API ALobbyGameMode : public AGameMode
{
    GENERATED_BODY()

    virtual void PostLogin(APlayerController* NewPlayer) override;
};
