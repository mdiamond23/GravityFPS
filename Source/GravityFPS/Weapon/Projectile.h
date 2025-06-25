// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class GRAVITYFPS_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();

	/*
	* Used With Server Side Rewind
	*/

	bool bUseServerSideRewind = false;
	FVector_NetQuantize TraceStart;
	FVector_NetQuantize100 InitialVelocity;
	UPROPERTY(EditAnywhere)
	float InitialSpeed = 35000.f;
	float Damage = 20.f;
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalInpulse, const FHitResult& Hit);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastImpactFX(const FVector& ImpactLocation, const FRotator& ImpactRotation);



	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;
	UPROPERTY()
	class UNiagaraComponent* TracerComponent;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;
public:	
	virtual void Tick(float DeltaTime) override;
	FORCEINLINE void MultiplyDamage(float DamageMultiplier) { Damage *= DamageMultiplier; }
	//virtual void Destroyed() override;

private:
	


	UPROPERTY(EditAnywhere, Category = "Effects")
	class UNiagaraSystem* Tracer;

	UPROPERTY(EditAnywhere, Category = "Effects")
	class UNiagaraSystem* ImpactParticle;

	UPROPERTY(EditAnywhere, Category = "Effects")
	class USoundCue* ImpactSound;
};
