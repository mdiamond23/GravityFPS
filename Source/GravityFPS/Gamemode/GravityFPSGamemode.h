// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GravityFPSGamemode.generated.h"

namespace MatchState {
	extern GRAVITYFPS_API const FName Cooldown; // Match duration is over. Show winner, stats and cooldown timer
}

/**
 * 
 */
UCLASS()
class GRAVITYFPS_API AGravityFPSGamemode : public AGameMode
{
	GENERATED_BODY()
	
public:
	AGravityFPSGamemode(); 
	virtual void Tick(float DeltaTime) override;
	virtual void PlayerKilled(class APlayerCharacter* KilledCharacter, class ACharacterPlayerController* VictimController, ACharacterPlayerController* AttackerController);
	virtual void RequestRespawn(ACharacter* KilledCharacter, AController* KilledController);
	
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;
	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;
	float LevelStartingTime = 0.f;
protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;
private:
	float CountdownTime = 0.f;
public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
};
