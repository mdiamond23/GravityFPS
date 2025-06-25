// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "GravityFPSGameState.generated.h"

/**
 * 
 */
UCLASS()
class GRAVITYFPS_API AGravityFPSGameState : public AGameState
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void UpdateTopScore(class ACharacterPlayerState* ScoringPlayer);

	UPROPERTY(Replicated)
	TArray<ACharacterPlayerState*> TopScoringPlayers;
private:
	float TopScore = 0.f;
};
