// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CharacterPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class GRAVITYFPS_API ACharacterPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;
	/*
	* Replication Functions
	*/
	virtual void OnRep_Score() override;
	UFUNCTION()
	virtual void OnRep_Deaths();
	void AddToScore(float ScoreAmount);
	void AddToDeaths(int32 DeathsAmount);

private:
	UPROPERTY()
	class APlayerCharacter* Character;
	UPROPERTY()
	class ACharacterPlayerController* Controller;
	UPROPERTY(ReplicatedUsing = OnRep_Deaths)
	int32 Deaths;
};
