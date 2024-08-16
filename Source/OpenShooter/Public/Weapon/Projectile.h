// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Projectile.generated.h"

class UProjectileMovementComponent;

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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Projectile", meta = (AllowPrivateAccess = "true"))
    UProjectileMovementComponent* ProjectileMovement;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components|Projectile", meta = (AllowPrivateAccess = "true"))
    UParticleSystem* Tracer;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components|Projectile", meta = (AllowPrivateAccess = "true"))
    UParticleSystemComponent* TracerComponent;
};
