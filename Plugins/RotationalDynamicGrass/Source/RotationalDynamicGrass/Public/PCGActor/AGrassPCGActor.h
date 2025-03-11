// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AGrassPCGActor.generated.h"

class UPCGComponent;
class UBoxComponent;
class UNiagaraDataChannelAsset;
class UNiagaraDataChannelWriter;


UCLASS()
class ROTATIONALDYNAMICGRASS_API AAGrassPCGActor : public AActor
{
	GENERATED_BODY()

protected:
    UNiagaraDataChannelWriter* NDCWriter;
	
public:	
	// Sets default values for this actor's properties
	AAGrassPCGActor();

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = PCG)
    UPCGComponent* PCGComponent;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Box)
    UBoxComponent* BaseBox;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = NiagaraGrass)
    float VoronoiPointNoiseThreshold;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = NiagaraGrass)
    float ZeroGrassVoronoiPointThreshold;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = NiagaraGrass)
    float MinClumpDensity;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = NiagaraGrass)
    float GrassLengthScaleMin;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = NiagaraGrass)
    float GrassLengthScaleMax;

    UPROPERTY(BlueprintReadOnly, Category = NiagaraGrass)
    int GenId;

protected:
    USceneComponent* RootSceneComponent;
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
