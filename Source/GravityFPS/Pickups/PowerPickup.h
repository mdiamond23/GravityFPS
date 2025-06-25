// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PickUp.h"
#include "PowerPickup.generated.h"

/**
 * 
 */
UCLASS()
class GRAVITYFPS_API APowerPickup : public APickUp
{
	GENERATED_BODY()
	
protected:
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	) override;

private:
	UPROPERTY(EditAnywhere)
	float PowerMultiplier = 2.f;

	UPROPERTY(EditAnywhere)
	float PowerBuffTime = 15.f;
};
