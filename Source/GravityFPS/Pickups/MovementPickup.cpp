// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementPickup.h"
#include "GravityFPS/Player/PlayerCharacter.h"
#include "GravityFPS/GravityFPSComponents/BuffComponent.h"

void AMovementPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(OtherActor);
	if (PlayerCharacter) {
		UBuffComponent* Buff = PlayerCharacter->GetBuff();
		if (Buff) Buff->BuffMovement(BaseSpeedBuff, MovementBuffTime);
	}

	Destroy();
}
