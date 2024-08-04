// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "CombatComponent.generated.h"

class AWeapon;
class AOpenShooterCharacter;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class OPENSHOOTER_API UCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UCombatComponent();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // The character that owns this combat component is a friend of this class
    // This is to allow the character to access the protected and private members of this class
    friend class AOpenShooterCharacter;

    // Equip a weapon
    void EquipWeapon(AWeapon* Weapon);

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

private:
    AOpenShooterCharacter* Character;

    AWeapon* EquippedWeapon;
};
