// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseGrass/BaseGrassActor.h"

// Sets default values
ABaseGrassActor::ABaseGrassActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ABaseGrassActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABaseGrassActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

