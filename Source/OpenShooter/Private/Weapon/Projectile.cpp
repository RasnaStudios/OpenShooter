// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "Weapon/Projectile.h"

#include "Character/OpenShooterCharacter.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "OpenShooter.h"
#include "Sound/SoundCue.h"

AProjectile::AProjectile()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;    // this replicates the actor, but the position is dictated by the server

    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    SetRootComponent(CollisionBox);

    // Set up collision
    CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);      // enable collision events
    CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);                // ignore all channels
    CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);     // but block visibility channel
    CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);    // and block world static channel
    CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);    // and block pawn channel

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->MaxSpeed = 15000.f;
    ProjectileMovement->InitialSpeed = 15000.f;
}

void AProjectile::BeginPlay()
{
    Super::BeginPlay();

    // To see where the projectile is spawned. It should spawn at the muzzle of the gun, not at the center
    // DrawDebugSphere(GetWorld(), GetActorLocation(), 10.f, 12, FColor::Red, true, 5.f, 0, 1.f);

    bCharacterHit = false;    // reset this otherwise it will always be true. It's set to true in the OnHit function

    if (Tracer)
    {
        TracerComponent =
            UGameplayStatics::SpawnEmitterAttached(Tracer, GetRootComponent(), NAME_None, GetActorLocation(), GetActorRotation(),
                EAttachLocation::KeepWorldPosition,    // this will follow along with the collision box
                false);
    }
    if (HasAuthority())
    {    // only the server should handle the hit events
        CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
    }
}

void AProjectile::Destroyed()
{
    // We let the character spawn its own impact effects in the OnHit function
    if (bCharacterHit)
        return;
    // Otherwise, we spawn the default impact effects
    if (ImpactParticles)
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
    if (ImpactSound)
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, GetActorLocation());
    Super::Destroyed();
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
    FVector NormalImpulse, const FHitResult& Hit)
{
    // We need this to set the HitCharacter so that we can spawn the impact effects
    if (AOpenShooterCharacter* Character = Cast<AOpenShooterCharacter>(OtherActor))
    {
        Character->SetHitImpactPoint(Hit.ImpactPoint);
        bCharacterHit = true;
    }

    Destroy();
}

void AProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}
