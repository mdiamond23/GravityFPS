// Fill out your copyright notice in the Description page of Project Settings.


#include "GravityFPSGamemode.h"
#include "GravityFPS/Player/PlayerCharacter.h"
#include "GravityFPS/PlayerController/CharacterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "GravityFPS/PlayerState/CharacterPlayerState.h"
#include "GravityFPS/GameState/GravityFPSGameState.h"

namespace MatchState {
	const FName Cooldown = FName("Cooldown");
}

AGravityFPSGamemode::AGravityFPSGamemode()
{
	bDelayedStart = true;
}

void AGravityFPSGamemode::BeginPlay()
{
	Super::BeginPlay();
	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void AGravityFPSGamemode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart) { 
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime; 
		if (CountdownTime <= 0.f) {
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress) {
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f) {
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f) {
			RestartGame();
		}
	}
}

void AGravityFPSGamemode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It) {
		ACharacterPlayerController* Player = Cast<ACharacterPlayerController>(*It);
		if (Player) {
			Player->OnMatchStateSet(MatchState);
		}
	}
}


void AGravityFPSGamemode::PlayerKilled(APlayerCharacter* KilledCharacter, ACharacterPlayerController* VictimController, ACharacterPlayerController* AttackerController)
{
	ACharacterPlayerState* AttackerPlayerState = AttackerController ? Cast<ACharacterPlayerState>(AttackerController->PlayerState) : nullptr;
	ACharacterPlayerState* VictimPlayerState = AttackerController ? Cast<ACharacterPlayerState>(VictimController->PlayerState) : nullptr;
	AGravityFPSGameState* GravityFPSGameState = GetGameState<AGravityFPSGameState>();

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && GravityFPSGameState) {
		AttackerPlayerState->AddToScore(100.f);
		GravityFPSGameState->UpdateTopScore(AttackerPlayerState);
	}

	if (VictimPlayerState) {
		VictimPlayerState->AddToDeaths(1);
	}

	if (KilledCharacter)
		KilledCharacter->Die();
}

void AGravityFPSGamemode::RequestRespawn(ACharacter* KilledCharacter, AController* KilledController)
{
	if (KilledCharacter) {
		KilledCharacter->Reset();
		KilledCharacter->Destroy();
	}
	if (KilledController) {
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(KilledController, PlayerStarts[Selection]);
	}
}
