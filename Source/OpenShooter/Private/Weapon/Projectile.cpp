// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "Weapon/Projectile.h"

#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectile::AProjectile()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    SetRootComponent(CollisionBox);

    // Set up collision
    CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);      // enable collision events
    CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);                // ignore all channels
    CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);     // but block visibility channel
    CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);    // and block world static channel

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->MaxSpeed = 15000.f;
    ProjectileMovement->InitialSpeed = 15000.f;
}

void AProjectile::BeginPlay()
{
    Super::BeginPlay();

    if (Tracer)
    {
        TracerComponent =
            UGameplayStatics::SpawnEmitterAttached(Tracer, GetRootComponent(), NAME_None, GetActorLocation(), GetActorRotation(),
                EAttachLocation::KeepWorldPosition,    // this will follow along with the collision box
                false);
    }
}

void AProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}
