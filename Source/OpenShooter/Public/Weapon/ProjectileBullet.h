// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"

#include "ProjectileBullet.generated.h"

/**
 *
 */
UCLASS()
class OPENSHOOTER_API AProjectileBullet : public AProjectile
{
    GENERATED_BODY()

protected:
    virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
        FVector NormalImpulse, const FHitResult& Hit) override;
};
