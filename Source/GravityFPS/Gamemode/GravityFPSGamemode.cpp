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
	ACharacterPlayerState* VictimPlayerState = VictimController ? Cast<ACharacterPlayerState>(VictimController->PlayerState) : nullptr;
	AGravityFPSGameState* GravityFPSGameState = GetGameState<AGravityFPSGameState>();

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && GravityFPSGameState) {
		TArray<ACharacterPlayerState*> PlayersCurrentlyInTheLead;
		for (auto LeadPlayer : GravityFPSGameState->TopScoringPlayers) {
			PlayersCurrentlyInTheLead.Add(LeadPlayer);
		}
		AttackerPlayerState->AddToScore(100.f);
		GravityFPSGameState->UpdateTopScore(AttackerPlayerState);

		// Adding crown to new leading player
		if (GravityFPSGameState->TopScoringPlayers.Contains(AttackerPlayerState)) {
			APlayerCharacter* Leader = Cast<APlayerCharacter>(AttackerPlayerState);
			if (Leader) {
				Leader->MulticastGainedTheLead();
			}
		}

		// Stripping crown from players that lost the lead
		for (int32 i = 0; i < PlayersCurrentlyInTheLead.Num(); i++) {
			if (!GravityFPSGameState->TopScoringPlayers.Contains(PlayersCurrentlyInTheLead[i])) {
				APlayerCharacter* Loser = Cast<APlayerCharacter>(PlayersCurrentlyInTheLead[i]->GetPawn());
				if (Loser) {
					Loser->MulticastLostTheLead();
				}
			}
		}
	}

	if (VictimPlayerState) {
		VictimPlayerState->AddToDeaths(1);
	}

	if (KilledCharacter) {
		KilledCharacter->Die(false);
	}
		

	APlayerCharacter* AttackerCharacter = AttackerController ? Cast<APlayerCharacter>(AttackerController->GetPawn()) : nullptr;
	if (!AttackerCharacter) return;
	AttackerCharacter->MulticastResetGravityTimer();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It) {
		ACharacterPlayerController* CharacterPlayer = Cast<ACharacterPlayerController>(*It);
		if (CharacterPlayer && AttackerPlayerState && VictimPlayerState) {
			CharacterPlayer->BroadcastKill(AttackerPlayerState, VictimPlayerState, AttackerCharacter->GetWeaponName());
		}
	}
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

void AGravityFPSGamemode::PlayerLeftGame(ACharacterPlayerState* PlayerLeaving)
{
	if (!PlayerLeaving) return;
	AGravityFPSGameState* GravityFPSGameState = GetGameState<AGravityFPSGameState>();
	if (GravityFPSGameState && GravityFPSGameState->TopScoringPlayers.Contains(PlayerLeaving)) {
		GravityFPSGameState->TopScoringPlayers.Remove(PlayerLeaving);
	}
	APlayerCharacter* CharacterLeaving = Cast<APlayerCharacter>(PlayerLeaving->GetPawn());
	if (CharacterLeaving) CharacterLeaving->Die(true);
}
