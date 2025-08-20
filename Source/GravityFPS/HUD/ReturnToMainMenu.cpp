// Fill out your copyright notice in the Description page of Project Settings.


#include "ReturnToMainMenu.h"
#include "GameFramework/PlayerController.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "GameFramework/GameModeBase.h"
#include "GravityFPS/Player/PlayerCharacter.h"

void UReturnToMainMenu::MenuSetup()
{
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);
	//bIsFocusable = true;

	UWorld* World = GetWorld();
	if (World) {
		PlayerController = !PlayerController ? World->GetFirstPlayerController() : PlayerController;
		if (PlayerController) {
			FInputModeGameAndUI InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}

	if (ReturnButton && !ReturnButton->OnClicked.IsBound()) {
		ReturnButton->OnClicked.AddDynamic(this, &UReturnToMainMenu::ReturnButtonClicked);
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if (MultiplayerSessionsSubsystem)
		{
			MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.RemoveDynamic(this, &UReturnToMainMenu::OnDestroySession);
			MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &UReturnToMainMenu::OnDestroySession);
		}
	}
}

bool UReturnToMainMenu::Initialize()
{
	if (!Super::Initialize()) return false;

	return true;
}

void UReturnToMainMenu::OnDestroySession(bool bWasSucessful)
{
	if (!bWasSucessful) {
		ReturnButton->SetIsEnabled(true); 
		return;
	}
	UWorld* World = GetWorld();
	if (World) {
		AGameModeBase* GameMode = World->GetAuthGameMode<AGameModeBase>();
		if (GameMode) {
			GameMode->ReturnToMainMenuHost();
		}
		else {
			PlayerController = !PlayerController ? World->GetFirstPlayerController() : PlayerController;
			if (PlayerController) {
				PlayerController->ClientReturnToMainMenuWithTextReason(FText());
			}
		}
	}
}

void UReturnToMainMenu::MenuTearDown()
{
	RemoveFromParent();

	UWorld* World = GetWorld();
	if (World) {
		PlayerController = !PlayerController ? World->GetFirstPlayerController() : PlayerController;
		if (PlayerController) {
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}

	if (ReturnButton && ReturnButton->OnClicked.IsBound()) {
		ReturnButton->OnClicked.RemoveDynamic(this, &UReturnToMainMenu::ReturnButtonClicked);
	}
	if (MultiplayerSessionsSubsystem) {
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.RemoveDynamic(this, &UReturnToMainMenu::OnDestroySession);
	}

}

void UReturnToMainMenu::ReturnButtonClicked()
{
	// TODO: Session destroy/return-to-main bug: see ChatGPT thread for details. Reproduce/test in clean project.
	ReturnButton->SetIsEnabled(false);
	//MultiplayerSessionsSubsystem->DestroySession();
	UWorld* World = GetWorld();
	if (World) {
		APlayerController* FirstPlayerController = World->GetFirstPlayerController();
		if (FirstPlayerController) {
			APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(FirstPlayerController->GetPawn());
			if (PlayerCharacter) {
				PlayerCharacter->ServerLeaveGame();
				PlayerCharacter->OnLeftGame.AddDynamic(this, &UReturnToMainMenu::OnPlayerLeftGame);
			}
			else {
				ReturnButton->SetIsEnabled(true);
			}
		}
	}
}

void UReturnToMainMenu::OnPlayerLeftGame()
{
	UE_LOG(LogTemp, Warning, TEXT("UReturnToMainMenu::OnPlayerLeftGame received!"))
		if (MultiplayerSessionsSubsystem)
		{
			UE_LOG(LogTemp, Warning, TEXT("MultiplayerSessionSubsystem not null"))
				// Bind OnDestroySession before calling DestroySession
				if (!MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.IsAlreadyBound(this, &UReturnToMainMenu::OnDestroySession))
				{
					UE_LOG(LogTemp, Warning, TEXT("Rebound on destory session complemete"))
					MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &UReturnToMainMenu::OnDestroySession);
				}
			MultiplayerSessionsSubsystem->DestroySession();
		}
}