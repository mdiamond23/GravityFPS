// Fill out your copyright notice in the Description page of Project Settings.


#include "Magazine.h"

// Sets default values
AMagazine::AMagazine()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    MagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MagMesh"));
    SetRootComponent(MagMesh);
    MagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MagMesh->SetSimulatePhysics(false);

    MagMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    MagMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

// Called when the game starts or when spawned
void AMagazine::BeginPlay()
{
	Super::BeginPlay();
	
}


void AMagazine::SetSimulate(bool bSimulate)
{
    MagMesh->SetSimulatePhysics(bSimulate);
}

void AMagazine::SetCollision(ECollisionEnabled::Type CollisionType)
{
    MagMesh->SetCollisionEnabled(CollisionType);
}

void AMagazine::SetVisible(bool bVisible)
{
    MagMesh->SetVisibility(bVisible);
}


