// Fill out your copyright notice in the Description page of Project Settings.


#include "GravityFPSGameState.h"
#include "Net/UnrealNetwork.h"
#include "GravityFPS/PlayerState/CharacterPlayerState.h"

void AGravityFPSGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGravityFPSGameState, TopScoringPlayers);
}

void AGravityFPSGameState::UpdateTopScore(ACharacterPlayerState* ScoringPlayer)
{
	if (TopScoringPlayers.IsEmpty() || TopScore == ScoringPlayer->GetScore()) {
		TopScoringPlayers.AddUnique(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
	else if (ScoringPlayer->GetScore() > TopScore) {
		TopScoringPlayers.Empty();
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}
