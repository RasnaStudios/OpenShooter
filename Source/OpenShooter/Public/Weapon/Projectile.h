// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Projectile.generated.h"

UCLASS()
class OPENSHOOTER_API AProjectile : public AActor
{
    GENERATED_BODY()

public:
    AProjectile();
    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components|Projectile", meta = (AllowPrivateAccess = "true"))
    class UBoxComponent* CollisionBox;
};
