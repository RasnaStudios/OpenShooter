// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "Weapon/ProjectileBullet.h"

#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
    FVector NormalImpulse, const FHitResult& Hit)
{
    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
        if (AController* OwnerController = OwnerCharacter->GetController())
            UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());

    // Parent does Destroy, so we need to put this last
    Super::OnHit(HitComponent, OtherActor, OtherComponent, NormalImpulse, Hit);
}
