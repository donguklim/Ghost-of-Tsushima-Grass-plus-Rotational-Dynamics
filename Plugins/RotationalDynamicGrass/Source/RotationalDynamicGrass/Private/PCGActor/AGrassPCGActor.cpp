// Fill out your copyright notice in the Description page of Project Settings.

#include "PCGActor/AGrassPCGActor.h"
#include "Components/BoxComponent.h"
#include "PCGComponent.h"
#include "NiagaraDataChannel.h"
#include "NiagaraDataChannelPublic.h"
#include "NiagaraDataChannelAccessor.h"

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

    NiagaraGrassDataChannel = CreateDefaultSubobject<UNiagaraDataChannelAsset>(TEXT("Grass NDC"));

    SetNDCWriter();

    BaseWind = FVector(1.0f, 1.0f, 0.1f);

}

void AAGrassPCGActor::SetNDCWriter() {

    if (NiagaraGrassDataChannel == nullptr) {
        return;
    }
    if (NiagaraGrassDataChannel->Get() == nullptr) {
        return;
    }

    if (NDCWriter != nullptr) {
        //NDCWriter->Cleanup();
    }

    FNiagaraDataChannelSearchParameters NiagarSearchParameters{ RootSceneComponent };
    NDCWriter = UNiagaraDataChannelLibrary::CreateDataChannelWriter(
        GetWorld(),
        NiagaraGrassDataChannel->Get(),
        NiagarSearchParameters,
        1,
        true,
        true,
        true,
        TEXT("Grass PCG Actor NDC Writer")
    );
}

void AAGrassPCGActor::WriteToNDC() {
    if (NiagaraGrassDataChannel == nullptr) {
        return;
    }
    if (NiagaraGrassDataChannel->Get() == nullptr) {
        return;
    }

    if (NDCWriter == nullptr) {
        SetNDCWriter();
    }

    NDCWriter->WriteVector(TEXT("BaseWind"), 0, BaseWind);
}

// Called when the game starts or when spawned
void AAGrassPCGActor::BeginPlay()
{
	Super::BeginPlay();
    WriteToNDC();
}

// Called every frame
void AAGrassPCGActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
