// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GRAVITYFPS_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBuffComponent();
	friend class APlayerCharacter;
	void Heal(float HealAmount, float HealingTime);
	void ReplenishShield(float ShieldAmount);
	void BuffMovement(float BuffBaseSpeed, float BuffTime);
	void SetInitalSpeed(float BaseSpeed);
	void BuffPower(float BuffBasePower, float BuffTime);

protected:
	virtual void BeginPlay() override;
	void HealRampUp(float DeltaTime);

private:
	UPROPERTY()
	class APlayerCharacter* Character;
	
	/*
	* Healing Buff
	*/
	bool bHealing = false;
	float HealingRate = 0.f;
	float AmountToHeal = 0.f;

	/*
	* Speed Buff
	*/
	FTimerHandle MovementBuffTimer;
	void ResetMovement();
	float InitialBaseSpeed;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastMovementBuff(float BaseSpeed, bool bStart);

	

	FTimerHandle PowerBuffTimer;
	void ResetPower();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPowerBuff(float PowerBuff, bool bStart);
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
