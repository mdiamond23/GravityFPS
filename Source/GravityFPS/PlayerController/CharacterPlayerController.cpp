// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterPlayerController.h"
#include "GravityFPS/HUD/CharacterOverlay.h"
#include "GravityFPS/HUD/PlayerHUD.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GravityFPS/Player/PlayerCharacter.h"
#include "Net/UnrealNetwork.h"
#include "GravityFPS/Gamemode/GravityFPSGamemode.h"
#include "GravityFPS/HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "GravityFPS/GravityFPSComponents/CombatComponent.h"
#include "GravityFPS/GameState/GravityFPSGameState.h"
#include "GravityFPS/PlayerState/CharacterPlayerState.h"
#include "Components/Image.h"
#include "GravityFPS/HUD/ReturnToMainMenu.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "GravityFPS/HUD/DamageMarker.h"



ACharacterPlayerController::ACharacterPlayerController()
{
	RedColor = FSlateColor(FLinearColor::Red);
	WhiteColor = FSlateColor(FLinearColor::White);
}

void ACharacterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	PlayerHUD = !PlayerHUD ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;

	bool bHUDValid = PlayerHUD && 
		PlayerHUD->CharacterOverlay && 
		PlayerHUD->CharacterOverlay->HealthBar && 
		PlayerHUD->CharacterOverlay->HealthText;

	if (bHUDValid) {
		const float HealthPercent = Health / MaxHealth;
		PlayerHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		PlayerHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else {
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ACharacterPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	PlayerHUD = !PlayerHUD ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;

	bool bHUDValid = PlayerHUD &&
		PlayerHUD->CharacterOverlay &&
		PlayerHUD->CharacterOverlay->ShieldBar &&
		PlayerHUD->CharacterOverlay->ShieldText && 
		PlayerHUD->CharacterOverlay->ShieldIcon;

	if (bHUDValid) {
		if (Shield > 0.f) {
			PlayerHUD->CharacterOverlay->ShieldBar->SetVisibility(ESlateVisibility::Visible);
			PlayerHUD->CharacterOverlay->ShieldText->SetVisibility(ESlateVisibility::Visible);
			PlayerHUD->CharacterOverlay->ShieldIcon->SetVisibility(ESlateVisibility::Visible);

			const float ShieldPercent = Shield / MaxShield;
			PlayerHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);
			FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
			PlayerHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
		}
		else {
			PlayerHUD->CharacterOverlay->ShieldBar->SetVisibility(ESlateVisibility::Hidden);
			PlayerHUD->CharacterOverlay->ShieldText->SetVisibility(ESlateVisibility::Hidden);
			PlayerHUD->CharacterOverlay->ShieldIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else {
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
	}
}

void ACharacterPlayerController::SetHUDScore(float Score)
{
	PlayerHUD = !PlayerHUD ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;

	bool bHUDValid = PlayerHUD &&
		PlayerHUD->CharacterOverlay &&
		PlayerHUD->CharacterOverlay->ScoreAmount;

	if (bHUDValid) {
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		PlayerHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else {
		bInitializeScore = true;
		HUDScore = Score;
	}
}

void ACharacterPlayerController::SetHUDDeaths(int32 Deaths)
{
	PlayerHUD = !PlayerHUD ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;

	bool bHUDValid = PlayerHUD &&
		PlayerHUD->CharacterOverlay &&
		PlayerHUD->CharacterOverlay->DeathsAmount;

	if (bHUDValid) {
		FString DeathsText = FString::Printf(TEXT("%d"), Deaths);
		PlayerHUD->CharacterOverlay->DeathsAmount->SetText(FText::FromString(DeathsText));
	}
	else {
		bInitializeDeaths = true;
		HUDDeaths = Deaths;
	}
}

void ACharacterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	PlayerHUD = !PlayerHUD ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;

	bool bHUDValid = PlayerHUD &&
		PlayerHUD->CharacterOverlay &&
		PlayerHUD->CharacterOverlay->WeaponAmmoAmount;

	if (bHUDValid) {
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		PlayerHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else {
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void ACharacterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	PlayerHUD = !PlayerHUD ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;

	bool bHUDValid = PlayerHUD &&
		PlayerHUD->CharacterOverlay &&
		PlayerHUD->CharacterOverlay->CarriedAmmoAmount;

	if (bHUDValid) {
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		PlayerHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else {
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
	}
}

void ACharacterPlayerController::SetHUDMatchCountdown(float CountdownTime, float DeltaTime)
{
	PlayerHUD = !PlayerHUD ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;

	bool bHUDValid = PlayerHUD &&
		PlayerHUD->CharacterOverlay &&
		PlayerHUD->CharacterOverlay->MatchCountdownText;

	if (bHUDValid) {
		if (CountdownTime < 0.f) {
			PlayerHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60);
		int32 Seconds = CountdownTime - Minutes * 60;
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		PlayerHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));

		if (CountdownTime <= 30.f) {
			CountdownBlinkAccumulator += DeltaTime; // DeltaTime passed in from Tick

			if (CountdownBlinkAccumulator >= 0.5f) // Blinks every 0.5s
			{
				bCountdownRed = !bCountdownRed;
				FSlateColor Color = bCountdownRed ? RedColor : WhiteColor;
				PlayerHUD->CharacterOverlay->MatchCountdownText->SetColorAndOpacity(Color);

				CountdownBlinkAccumulator = 0.f;
			}
			// Do nothing in the else. Just let the color "stick" until the next toggle.
		}
		else
		{
			// Always set to white if not blinking, and reset blink state
			PlayerHUD->CharacterOverlay->MatchCountdownText->SetColorAndOpacity(WhiteColor);
			CountdownBlinkAccumulator = 0.f;
			bCountdownRed = false;
		}

	}
}

void ACharacterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	PlayerHUD = !PlayerHUD ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;

	bool bHUDValid = PlayerHUD &&
		PlayerHUD->Announcement &&
		PlayerHUD->Announcement->WarmupTime;

	if (bHUDValid) {
		if (CountdownTime < 0.f) {
			PlayerHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60);
		int32 Seconds = CountdownTime - Minutes * 60;
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		PlayerHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void ACharacterPlayerController::SetHUDGravityBar(float Percent)
{
	PlayerHUD = !PlayerHUD ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;

	bool bHUDValid = PlayerHUD &&
		PlayerHUD->CharacterOverlay &&
		PlayerHUD->CharacterOverlay->GravityBar;

	if (bHUDValid) {
		PlayerHUD->CharacterOverlay->GravityBar->SetPercent(Percent);
	}
	else {
		bInitialzeGravity = true;
		HUDGravity = Percent;
	}
}


void ACharacterPlayerController::SetHUDTime(float DeltaTime)
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown) TimeLeft = WarmupTime + MatchTime + CooldownTime - GetServerTime() + LevelStartingTime;

	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft - GetServerTime());

	if (HasAuthority()) {
		Gamemode = !Gamemode ? Cast<AGravityFPSGamemode>(UGameplayStatics::GetGameMode(this)) : Gamemode;
		if (Gamemode) {
			SecondsLeft = FMath::CeilToInt(Gamemode->GetCountdownTime() + LevelStartingTime);
		}
	}

	if (CountdownInt != SecondsLeft) {
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown) {
			SetHUDAnnouncementCountdown(TimeLeft);
		}
	}
	else if (MatchState == MatchState::InProgress) {
		SetHUDMatchCountdown(TimeLeft, DeltaTime);
	}

	CountdownInt = SecondsLeft;
}

void ACharacterPlayerController::HighPingWarning()
{
	PlayerHUD = !PlayerHUD ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;

	bool bHUDValid = PlayerHUD &&
		PlayerHUD->CharacterOverlay &&
		PlayerHUD->CharacterOverlay->HighPingImage && 
		PlayerHUD->CharacterOverlay->HighPingAnimation;

	if (bHUDValid) {
		PlayerHUD->CharacterOverlay->HighPingImage->SetOpacity(1.f);
		PlayerHUD->CharacterOverlay->PlayAnimation(
			PlayerHUD->CharacterOverlay->HighPingAnimation,
			0.f,
			7
		);
	}
}

void ACharacterPlayerController::StopHighPingWarning()
{
	PlayerHUD = !PlayerHUD ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;

	bool bHUDValid = PlayerHUD &&
		PlayerHUD->CharacterOverlay &&
		PlayerHUD->CharacterOverlay->HighPingImage &&
		PlayerHUD->CharacterOverlay->HighPingAnimation;

	if (bHUDValid) {
		PlayerHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if (PlayerHUD->CharacterOverlay->IsAnimationPlaying(PlayerHUD->CharacterOverlay->HighPingAnimation)) {
			PlayerHUD->CharacterOverlay->StopAnimation(PlayerHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}

void ACharacterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	PlayerCharacter = Cast<APlayerCharacter>(InPawn);
	if (PlayerCharacter) {
		PlayerCharacter->StopCameraShake();
		SetHUDHealth(PlayerCharacter->GetHealth(), PlayerCharacter->GetMaxHealth());
		SetHUDShield(PlayerCharacter->GetShield(), PlayerCharacter->GetMaxShield());
	}
}

void ACharacterPlayerController::OnUnPossess()
{
	if (PlayerCharacter)
    {
		PlayerCharacter->StopCameraShake();
		PlayerCharacter = nullptr;
    }
    Super::OnUnPossess();
}

void ACharacterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (MenuMappingContext)
				Subsystem->AddMappingContext(MenuMappingContext, 1);
		}
	}
	PlayerHUD = Cast<APlayerHUD>(GetHUD());
	ServerCheckMatchState();
}

void ACharacterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CheckTimeSync(DeltaTime);
	SetHUDTime(DeltaTime);
	PollInit();
	CheckPing(DeltaTime);
}

void ACharacterPlayerController::CheckPing(float DeltaTime)
{
	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime > CheckPingFrequency) {
		if (PlayerState) {
			float PingMs = PlayerState->GetPingInMilliseconds();
			if (PingMs > HighPingThreshold) {
				HighPingWarning();
				PingAnimationRunningTime = 0.f;
			}
			ServerReportPingStatus(PingMs > HighPingThreshold);
		}
		HighPingRunningTime = 0.f;
	}

	bool bHighPingAnimationPlaying =
		PlayerHUD &&
		PlayerHUD->CharacterOverlay &&
		PlayerHUD->CharacterOverlay->HighPingAnimation &&
		PlayerHUD->CharacterOverlay->IsAnimationPlaying(PlayerHUD->CharacterOverlay->HighPingAnimation);
	if (bHighPingAnimationPlaying) {
		PingAnimationRunningTime += DeltaTime;
	}

	if (PingAnimationRunningTime > HighPingDuration) {
		StopHighPingWarning();
	}
}

void ACharacterPlayerController::ShowReturnToMainMenu()
{
	if (!ReturnToMainMenuWidget) return;
	if (!ReturnToMainMenu) {
		ReturnToMainMenu = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuWidget);
	}
	if (ReturnToMainMenu) {
		bReturnToMainMenuOpen = !bReturnToMainMenuOpen;
		if (bReturnToMainMenuOpen) {
			ReturnToMainMenu->MenuSetup();
		}
		else {
			ReturnToMainMenu->MenuTearDown();
		}
	}
}

void ACharacterPlayerController::BroadcastKill(APlayerState* AttackerPlayerState, APlayerState* VictimPlayerState, const FString& WeaponName)
{
	ClientKillAnnoucement(AttackerPlayerState, VictimPlayerState, WeaponName);
}

void ACharacterPlayerController::ShowDamageMarker(float DamageAmount, bool bShield)
{
	if (!DamageMarkerClass) return;

	UDamageMarker* Marker = CreateWidget<UDamageMarker>(this, DamageMarkerClass);
	if (!Marker || !Marker->DamageText || !Marker->Fade) return;

	Marker->SetDamageAmount(DamageAmount);
	Marker->DamageText->SetColorAndOpacity(bShield ? FLinearColor::Yellow : FLinearColor::Blue);

	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport) {
        GEngine->GameViewport->GetViewportSize(ViewportSize);
		UE_LOG(LogTemp, Warning, TEXT("Viewport Size: %s"), *ViewportSize.ToString());
	}

	float XPos = ViewportSize.X * 0.505f;
	float YPos = ViewportSize.Y * 0.425f;

	Marker->AddToViewport();
	Marker->SetPositionInViewport(FVector2D(XPos, YPos), true);

	if (Marker->Fade)
	{
		Marker->PlayFadeAnimation();
	}
}

void ACharacterPlayerController::ClientKillAnnoucement_Implementation(APlayerState* AttackerPlayerState, APlayerState* VictimPlayerState, const FString& WeaponName)
{
	APlayerState* Self = GetPlayerState<APlayerState>();

	if (AttackerPlayerState && VictimPlayerState && Self)
	{
		PlayerHUD = !PlayerHUD ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
		if (PlayerHUD) {
			if (AttackerPlayerState == Self && VictimPlayerState != Self) {
				PlayerHUD->AddKillAnnoucement("You", VictimPlayerState->GetPlayerName(), WeaponName);
				return;
			}
			else if (VictimPlayerState == Self && AttackerPlayerState != Self) {
				PlayerHUD->AddKillAnnoucement(AttackerPlayerState->GetPlayerName(), "you", WeaponName);
				return;
			}
			if (AttackerPlayerState == VictimPlayerState && AttackerPlayerState == Self) {
				PlayerHUD->AddKillAnnoucement("You", "yourself", WeaponName);
				return;
			}
			if (AttackerPlayerState == VictimPlayerState) {
				PlayerHUD->AddKillAnnoucement(AttackerPlayerState->GetPlayerName(), "themselves", WeaponName);
				return;
			}
			PlayerHUD->AddKillAnnoucement(AttackerPlayerState->GetPlayerName(), VictimPlayerState->GetPlayerName(), WeaponName);
		}
	}
}

// is the thing too high
void ACharacterPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}

void ACharacterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACharacterPlayerController, MatchState);
}

void ACharacterPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
}

void ACharacterPlayerController::HandleMatchHasStarted()
{
	PlayerHUD = !PlayerHUD ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	if (PlayerHUD && PlayerHUD->CharacterOverlay == nullptr) {
		PlayerHUD->AddCharacterOverlay();
		if (PlayerHUD->Announcement) {
			PlayerHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ACharacterPlayerController::HandleCooldown()
{
	PlayerHUD = !PlayerHUD ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	if (PlayerHUD && PlayerHUD->CharacterOverlay)
	{
		PlayerHUD->CharacterOverlay->RemoveFromParent();
		PlayerHUD->CharacterOverlay = nullptr;

		bool bHUDValid = PlayerHUD->Announcement &&
			PlayerHUD->Announcement->AnnouncementText &&
			PlayerHUD->Announcement->InfoText;

		if (bHUDValid) {
			PlayerHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText("New Match Starts In:");
			PlayerHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			AGravityFPSGameState* GravityFPSGameState = Cast<AGravityFPSGameState>(UGameplayStatics::GetGameState(this));
			ACharacterPlayerState* CharacterPlayerState = GetPlayerState<ACharacterPlayerState>();

			if (GravityFPSGameState) {
				GravityFPSGameState->TopScoringPlayers;
				FString InfoTextString;
				if (GravityFPSGameState->TopScoringPlayers.IsEmpty()) {
					InfoTextString = FString("No one won.");
				}
				else if (GravityFPSGameState->TopScoringPlayers.Num() == 1) {
					if (GravityFPSGameState->TopScoringPlayers[0] == CharacterPlayerState) {
						InfoTextString = FString("You won!");
					}
					else {
						InfoTextString = FString::Printf(TEXT("%s won!"), *GravityFPSGameState->TopScoringPlayers[0]->GetPlayerName());
					}
				}
				else if (GravityFPSGameState->TopScoringPlayers.Num() > 1) {
					InfoTextString = FString("Players tied for the win:\n");
					for (const auto& TiedPlayer : GravityFPSGameState->TopScoringPlayers) {
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}
				}
				PlayerHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}
		}
	}

	PlayerCharacter = !PlayerCharacter ? Cast<APlayerCharacter>(GetPawn()) : PlayerCharacter;
	if (PlayerCharacter) {
		PlayerCharacter->bDisableGameplay = true;
		if (PlayerCharacter->GetCombat()) PlayerCharacter->GetCombat()->FireButtonPressed(false);
	}
}

void ACharacterPlayerController::ServerCheckMatchState_Implementation()
{
	AGravityFPSGamemode* GravityFPSGameMode = Cast<AGravityFPSGamemode>(UGameplayStatics::GetGameMode(this));
	
	if (GravityFPSGameMode) {
		WarmupTime = GravityFPSGameMode->WarmupTime;
		MatchTime = GravityFPSGameMode->MatchTime;
		CooldownTime = GravityFPSGameMode->CooldownTime;
		LevelStartingTime = GravityFPSGameMode->LevelStartingTime;
		MatchState = GravityFPSGameMode->GetMatchState();
		ClientJoinMidGame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);

		if (PlayerHUD && MatchState == MatchState::WaitingToStart && PlayerHUD->Announcement == nullptr) {
			PlayerHUD->AddAnnouncement();
		}
	}
}

void ACharacterPlayerController::ClientJoinMidGame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float LevelStarting)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = LevelStarting;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);

	if (PlayerHUD && MatchState == MatchState::WaitingToStart && PlayerHUD->Announcement == nullptr) {
		PlayerHUD->AddAnnouncement();
	}
}

void ACharacterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController()) {
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ACharacterPlayerController::PollInit()
{
	if (!CharacterOverlay) {
		if (PlayerHUD && PlayerHUD->CharacterOverlay) {
			CharacterOverlay = PlayerHUD->CharacterOverlay;
			if (CharacterOverlay) {
				if (bInitializeHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);
				if (bInitializeShield) SetHUDShield(HUDShield, HUDMaxShield);
				if (bInitializeScore) SetHUDScore(HUDScore);
				if (bInitializeDeaths) SetHUDDeaths(HUDDeaths);
				if (bInitializeWeaponAmmo) SetHUDWeaponAmmo(HUDWeaponAmmo);
				if (bInitializeCarriedAmmo) SetHUDCarriedAmmo(HUDCarriedAmmo);
				if (bInitialzeGravity) SetHUDGravityBar(HUDGravity);
			}
		}
	}
}

void ACharacterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent) return;
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (QuitAction)
		{
			EnhancedInputComponent->BindAction(QuitAction, ETriggerEvent::Started, this, &ACharacterPlayerController::ShowReturnToMainMenu);
		}
	}
}


void ACharacterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ACharacterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	SingleTripTime = (0.5f * RoundTripTime);
	float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float ACharacterPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ACharacterPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress) {
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown) {
		HandleCooldown();
	}
}

void ACharacterPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::WaitingToStart) {
		if (PlayerHUD && PlayerHUD->Announcement)
			PlayerHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
	}

	if (MatchState == MatchState::InProgress) {
		HandleMatchHasStarted();
	} 
	else if (MatchState == MatchState::Cooldown) {
		HandleCooldown();
	}
}