// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CharacterPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

/**
 * 
 */
UCLASS()
class GRAVITYFPS_API ACharacterPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ACharacterPlayerController();
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDScore(float Score);
	void SetHUDDeaths(int32 Deaths);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(float CountdownTime, float DeltaTime);
	void SetHUDAnnouncementCountdown(float CountdownTime);
	void SetHUDGravityBar(float Percent);
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//Synced with server world clock
	virtual float GetServerTime();
	// Sync with server clock asap
	virtual void ReceivedPlayer() override;
	void OnMatchStateSet(FName State);

	float SingleTripTime = 0.f;
	FHighPingDelegate HighPingDelegate;

	void BroadcastKill(APlayerState* AttackerPlayerState, APlayerState* VictimPlayerState, const FString& WeaponName);
	void ShowDamageMarker(float DamageAmout, bool bShield);
protected:
	virtual void BeginPlay() override;
	void SetHUDTime(float DeltaTime);
	void PollInit();
	virtual void SetupInputComponent() override;
	/*
	* Sync time between client and server
	*/

	// Requests current server time, passing in the client's time when the request was sent
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	// Reports current server time to the client in response to ServerRequestServerTime
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	// diffrence between client and server time
	float ClientServerDelta = 0.f; 

	UPROPERTY(EditAnywhere, Category = "Time")
	float TimeSyncFrequency = 5.f;
	float TimeSyncRunningTime = 0.f;
	void CheckTimeSync(float DeltaTime);
	void HandleMatchHasStarted();
	void HandleCooldown();

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();
	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(FName StateOfMatch, float Warmup, float Match, float Cooldown, float LevelStarting);

	void HighPingWarning();
	void StopHighPingWarning();
	void CheckPing(float DeltaTime);
	void ShowReturnToMainMenu();

	UFUNCTION(Client, Reliable)
	void ClientKillAnnoucement(APlayerState* AttackerPlayerState, APlayerState* VictimPlayerState, const FString& WeaponName);

	void ResetGravityTimer();
private:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* QuitAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputMappingContext* MenuMappingContext;

	UPROPERTY(EditAnywhere, Category = "HUD")
	TSubclassOf<class UUserWidget> ReturnToMainMenuWidget;
	UPROPERTY()
	class UReturnToMainMenu* ReturnToMainMenu;
	bool bReturnToMainMenuOpen = false;

	UPROPERTY(EditAnywhere, Category = Effects)
	TSubclassOf<class UDamageMarker> DamageMarkerClass;
	
	UPROPERTY()
	class APlayerHUD* PlayerHUD;
	UPROPERTY()
	class AGravityFPSGamemode* Gamemode;

	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float CooldownTime = 0.f;
	float LevelStartingTime = 0.f;
	uint32 CountdownInt = 0;
	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);

	float HUDHealth;
	float HUDMaxHealth;
	float HUDShield;
	float HUDMaxShield;
	float HUDScore;
	float HUDGravity;
	int32 HUDDeaths;
	int32 HUDWeaponAmmo;
	int32 HUDCarriedAmmo;

	bool bInitializeHealth = false;
	bool bInitializeShield = false;
	bool bInitializeScore = false;
	bool bInitializeDeaths = false;
	bool bInitializeWeaponAmmo = false;
	bool bInitializeCarriedAmmo = false;
	bool bInitialzeGravity = false;

	FSlateColor RedColor;
	FSlateColor WhiteColor;
	float CountdownBlinkAccumulator = 0.f;
	bool bCountdownRed = false;

	float HighPingRunningTime = 0.f;
	float PingAnimationRunningTime = 0.f;
	UPROPERTY(EditAnywhere)
	float HighPingDuration = 5.f;
	UPROPERTY(EditAnywhere)
	float CheckPingFrequency = 20.f;
	UPROPERTY(EditAnywhere)
	float HighPingThreshold = 120.f;

	UPROPERTY()
	UMaterialInstanceDynamic* GravityBarMID;

	UPROPERTY()
	class APlayerCharacter* PlayerCharacter;

};

 