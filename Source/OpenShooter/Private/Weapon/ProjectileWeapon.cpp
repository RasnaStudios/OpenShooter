// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "Weapon/ProjectileWeapon.h"

#include "Kismet/GameplayStatics.h"
#include "Weapon/Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
    Super::Fire(HitTarget);

    if (!HasAuthority())
        return;    // Only run on server

    APawn* InstigatorPawn = Cast<APawn>(GetOwner());

    const FTransform SocketTransform = GetMesh()->GetSocketTransform(FName("MuzzleFlashSocket"));
    const FVector ToTarget =
        HitTarget - SocketTransform.GetLocation();    // From the muzzle to hit location from TraceUnderCrosshair
    const FRotator TargetRotation = ToTarget.Rotation();
    if (ProjectileClass && InstigatorPawn)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = GetOwner();
        SpawnParams.Instigator = InstigatorPawn;

        if (GetWorld())
        {
            AProjectile* Projectile =
                GetWorld()->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
        }
    }
}
