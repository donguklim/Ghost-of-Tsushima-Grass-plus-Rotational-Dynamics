// Fill out your copyright notice in the Description page of Project Settings.

#include "PCGActor/AGrassPCGActor.h"
#include "Components/BoxComponent.h"
#include "PCGComponent.h"

// Sets default values
AAGrassPCGActor::AAGrassPCGActor()
{
    PrimaryActorTick.bCanEverTick = false;
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(RootSceneComponent);

    BaseBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Box 0"));
    BaseBox->SetupAttachment(RootSceneComponent);

    BaseBox->InitBoxExtent(FVector(1000.0f, 1000.0f, 1000.0f));

    // Set the box to have no collision by default
    BaseBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    PCGComponent = CreateDefaultSubobject<UPCGComponent>(TEXT("PCG Component"));
    PCGComponent->SetIsPartitioned(true);
    PCGComponent->GenerationTrigger = EPCGComponentGenerationTrigger::GenerateAtRuntime;

}

// Called when the game starts or when spawned
void AAGrassPCGActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAGrassPCGActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

