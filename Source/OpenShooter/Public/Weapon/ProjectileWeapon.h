// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"

#include "ProjectileWeapon.generated.h"

class AProjectile;
/**
 *
 */
UCLASS()
class OPENSHOOTER_API AProjectileWeapon : public AWeapon
{
    GENERATED_BODY()

public:
    virtual void Fire(const FVector& HitTarget) override;

private:
    UPROPERTY(EditAnywhere)
    TSubclassOf<AProjectile> ProjectileClass;
};
