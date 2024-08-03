// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Weapon.generated.h"

class USphereComponent;

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
    EWS_Initial UMETA(DisplayName = "Initial"),
    EWS_Equipped UMETA(DisplayName = "Equipped"),
    EWS_Dropped UMETA(DisplayName = "Dropped"),

    EWS_MAX UMETA(DisplayName = "DefaultMax"),
};

UCLASS()
class OPENSHOOTER_API AWeapon : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AWeapon();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

private:
    UPROPERTY(EditAnywhere, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
    USkeletalMeshComponent* WeaponMesh;

    UPROPERTY(EditAnywhere, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
    USphereComponent* AreaSphere;

    UPROPERTY(VisibleAnywhere)
    EWeaponState WeaponState;
};
