// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickUp.generated.h"

UCLASS()
class GRAVITYFPS_API APickUp : public AActor
{
	GENERATED_BODY()
	
public:	
	APickUp();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

private:
	UPROPERTY(EditAnywhere)
	class USphereComponent* OverlapSphere; 

	UPROPERTY(EditAnywhere)
	class USoundCue* PickupSound;

	UPROPERTY(EditAnywhere)
	class UStaticMeshComponent* PickupMesh;

	/*
	* Weapon Spinning
	*/

	UPROPERTY(EditAnywhere, Category = "Sway")
	float LocationLagSpeed = 10.f;

	UPROPERTY(EditAnywhere, Category = "Sway")
	float SwayAmplitdue = 2.f;

	UPROPERTY(EditAnywhere, Category = "Sway")
	float SwayFrequency = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Sway")
	float SpinSpeed = 60.f;

	void Rotate(float DeltaTime);

	UPROPERTY(VisibleAnywhere)
	class UNiagaraComponent* PickupEffectComponent;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* PickupEffect;

	FTimerHandle BindOverlapTimer;
	float BindOverlapTime = .25f;
	void BindOverlapTimerFinished();
public:
};
