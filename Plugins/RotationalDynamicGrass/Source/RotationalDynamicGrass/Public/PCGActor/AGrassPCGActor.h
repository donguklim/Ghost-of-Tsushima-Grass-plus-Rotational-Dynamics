// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PCGGraph.h"
#include "AGrassPCGActor.generated.h"

UCLASS()
class ROTATIONALDYNAMICGRASS_API AAGrassPCGActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAGrassPCGActor();

    UPROPERTY(EditAnywhere)
    UPCGGraph* Graph;

protected:
    USceneComponent* SceneRoot;
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
