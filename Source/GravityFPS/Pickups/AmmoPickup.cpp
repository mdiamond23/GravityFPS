// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoPickup.h"
#include "GravityFPS/Player/PlayerCharacter.h"
#include "GravityFPS/GravityFPSComponents/CombatComponent.h"

void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(OtherActor);
	if (PlayerCharacter) {
		UCombatComponent* Combat = PlayerCharacter->GetCombat();
		if (Combat) {
			Combat->PickupAmmo(WeaponType, AmmoToAdd);
		}
	}

	Destroy();
}
