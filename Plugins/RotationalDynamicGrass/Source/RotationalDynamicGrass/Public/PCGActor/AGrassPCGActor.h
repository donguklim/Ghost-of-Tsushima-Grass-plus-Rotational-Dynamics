// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AGrassPCGActor.generated.h"

class UPCGComponent;

UCLASS()
class ROTATIONALDYNAMICGRASS_API AAGrassPCGActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAGrassPCGActor();

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = PCG)
    UPCGComponent* PCGComponent;

protected:
    USceneComponent* RootSceneComponent;
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
