// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterPlayerState.h"
#include "GravityFPS/Player/PlayerCharacter.h"
#include "GravityFPS/PlayerController/CharacterPlayerController.h"
#include "Net/UnrealNetwork.h"

void ACharacterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACharacterPlayerState, Deaths);
}

void ACharacterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	Character = !Character ? Cast<APlayerCharacter>(GetPawn()) : Character;
	
	if (Character) {
		Controller = !Controller ? Cast<ACharacterPlayerController>(Character->Controller) : Controller;
		if (Controller) {
			Controller->SetHUDScore(GetScore());
		}
	}
}

void ACharacterPlayerState::OnRep_Deaths()
{
	Character = !Character ? Cast<APlayerCharacter>(GetPawn()) : Character;
	if (Character) {
		Controller = !Controller ? Cast<ACharacterPlayerController>(Character->Controller) : Controller;
		if (Controller) {
			Controller->SetHUDDeaths(Deaths);
		}
	}
}

void ACharacterPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);
	Character = !Character ? Cast<APlayerCharacter>(GetPawn()) : Character;
	if (Character) {
		Controller = !Controller ? Cast<ACharacterPlayerController>(Character->Controller) : Controller;
		if (Controller) {
			Controller->SetHUDScore(GetScore());
		}
	}
}

void ACharacterPlayerState::AddToDeaths(int32 DeathAmount)
{
	Deaths += DeathAmount;
	Character = !Character ? Cast<APlayerCharacter>(GetPawn()) : Character;
	if (Character) {
		Controller = !Controller ? Cast<ACharacterPlayerController>(Character->Controller) : Controller;
		if (Controller) {
			Controller->SetHUDDeaths(Deaths);
		}
	}
}
