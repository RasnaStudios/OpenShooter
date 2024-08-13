// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "Weapon/Projectile.h"

#include "Components/BoxComponent.h"

AProjectile::AProjectile()
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    SetRootComponent(CollisionBox);

    // Set up collision
    CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);      // enable collision events
    CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);                // ignore all channels
    CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);     // but block visibility channel
    CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);    // and block world static channel
}

void AProjectile::BeginPlay()
{
    Super::BeginPlay();
}

void AProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}
