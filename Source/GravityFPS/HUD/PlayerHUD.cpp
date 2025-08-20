// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerHUD.h"
#include "GameFramework/PlayerController.h"
#include "CharacterOverlay.h"
#include "Announcement.h"
#include "KillAnouncement.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/HorizontalBox.h"
#include "Components/CanvasPanelSlot.h"


void APlayerHUD::BeginPlay()
{
	Super::BeginPlay();
}

void APlayerHUD::AddCharacterOverlay()
{
	APlayerController* PlayerController = GetOwningPlayerController();

	if (PlayerController && CharacterOverlayClass && CharacterOverlay == nullptr) {
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
		CharacterOverlay->AddToViewport();
	}
}

void APlayerHUD::AddAnnouncement()
{
	APlayerController* PlayerController = GetOwningPlayerController();

	if (PlayerController && AnnouncementClass && Announcement == nullptr) {
		Announcement = CreateWidget<UAnnouncement>(PlayerController, AnnouncementClass);
		Announcement->AddToViewport();
	}
}

void APlayerHUD::AddKillAnnoucement(const FString& AttackerName, const FString& VictimName, const FString& WeaponName)
{
	OwningPlayer = !OwningPlayer ? GetOwningPlayerController() : OwningPlayer;
	if (OwningPlayer && KillAnnouncementClass) {
		UKillAnouncement* KillAnnoucementWidget = CreateWidget<UKillAnouncement>(OwningPlayer, KillAnnouncementClass);
		if (KillAnnoucementWidget) {
			KillAnnoucementWidget->SetKillAnnoucementText(AttackerName, VictimName, WeaponName);
			KillAnnoucementWidget->AddToViewport();
			
			for (auto Msg : KillMessages) {
				if (Msg && Msg->AnnoucementBox) {
                    UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Msg->AnnoucementBox);
					if (CanvasSlot) {
						FVector2D Position = CanvasSlot->GetPosition();
						FVector2D NewPosition(
							CanvasSlot->GetPosition().X,
							Position.Y + CanvasSlot->GetSize().Y
						);
						CanvasSlot->SetPosition(NewPosition);
					}
					
				}
			}

			KillMessages.Add(KillAnnoucementWidget);

			FTimerHandle KillMsgTimer;
			FTimerDelegate KillMsgDelegate;
			KillMsgDelegate.BindUFunction(this, FName("KillAnnoucementTimerFinished"), KillAnnoucementWidget);
			GetWorldTimerManager().SetTimer(
				KillMsgTimer,
				KillMsgDelegate,
				KillAnnoucementTime,
				false
			);
		}
	}
}


void APlayerHUD::KillAnnoucementTimerFinished(UKillAnouncement* MsgToRemove)
{
	if (MsgToRemove)
	{
		MsgToRemove->RemoveFromParent();
	}
}

void APlayerHUD::DrawHUD()
{
	Super::DrawHUD();

	FVector2D ViewportSize;

	if (GEngine) {
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D ViewportCenter(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
		float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;

		if (HUDPackage.CrosshairsCenter) 
			DrawCrosshair(HUDPackage.CrosshairsCenter, ViewportCenter, FVector2D(0.f, 0.f), HUDPackage.CrosshairsColor);
		if (HUDPackage.CrosshairsLeft) 
			DrawCrosshair(HUDPackage.CrosshairsLeft, ViewportCenter, FVector2D(-SpreadScaled, 0.f), HUDPackage.CrosshairsColor);
		if (HUDPackage.CrosshairsRight) 
			DrawCrosshair(HUDPackage.CrosshairsRight, ViewportCenter, FVector2D(SpreadScaled, 0.f), HUDPackage.CrosshairsColor);
		if (HUDPackage.CrosshairsTop) 
			DrawCrosshair(HUDPackage.CrosshairsTop, ViewportCenter, FVector2D(0.f, -SpreadScaled), HUDPackage.CrosshairsColor);
		if (HUDPackage.CrosshairsBottom) 
			DrawCrosshair(HUDPackage.CrosshairsBottom, ViewportCenter, FVector2D(0.f, SpreadScaled), HUDPackage.CrosshairsColor);
	}
}

void APlayerHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();
	const FVector2D DrawPoint(
		ViewportCenter.X - (TextureWidth / 2.f) + Spread.X,
		ViewportCenter.Y - (TextureHeight / 2.f) + Spread.Y
	);

	DrawTexture(
		Texture,
		DrawPoint.X,
		DrawPoint.Y,
		TextureWidth,
		TextureHeight,
		0.f,
		0.f,
		1.f, 
		1.f,
		CrosshairColor
	);
}
