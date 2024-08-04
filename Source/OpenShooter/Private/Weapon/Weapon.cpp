// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "Weapon/Weapon.h"

#include "Character/OpenShooterCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"

// Sets default values
AWeapon::AWeapon()
{
    PrimaryActorTick.bCanEverTick = false;

    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    SetRootComponent(WeaponMesh);

    // Set the collision response for the weapon mesh
    WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
    WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

    // Disable collision for the weapon mesh initially
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Create the area sphere for the weapon to collect
    AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
    AreaSphere->SetupAttachment(WeaponMesh);

    // Set the collision response for the area sphere to ignore all channels because we will handle the collision in the server
    AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
    AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);

    PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
    PickupWidget->SetupAttachment(RootComponent);
}

void AWeapon::ShowPickupWidget(const bool bShow) const
{
    if (PickupWidget)
    {
        PickupWidget->SetVisibility(bShow);
    }
}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
    Super::BeginPlay();

    // We enable the collision for the area sphere only on the server
    if (HasAuthority())
    {
        AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
    }

    // Hide the pickup widget initially
    if (PickupWidget)
        PickupWidget->SetVisibility(false);
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // Set the overlapping weapon for the character
    if (AOpenShooterCharacter* Character = Cast<AOpenShooterCharacter>(OtherActor))
        Character->SetOverlappingWeapon(this);
}

void AWeapon::OnSphereEndOverlap(
    UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    // Set the overlapping weapon for the character
    if (AOpenShooterCharacter* Character = Cast<AOpenShooterCharacter>(OtherActor))
        Character->SetOverlappingWeapon(nullptr);
}
