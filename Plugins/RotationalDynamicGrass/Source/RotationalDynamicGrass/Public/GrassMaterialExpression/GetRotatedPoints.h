#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionCustom.h"
#include "MaterialHLSLGenerator.h"
#include "GetRotatedPoints.generated.h"

UCLASS(Abstract)
class ROTATIONALDYNAMICGRASS_API UGetRotatedPoints : public UMaterialExpression
{
	GENERATED_BODY()

public:
    UGetRotatedPoints(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
    //~ Begin UMaterialExpression Interface
    virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
    virtual bool GenerateHLSLExpression(
        FMaterialHLSLGenerator& Generator,
        UE::HLSLTree::FScope& Scope,
        int32 OutputIndex,
        UE::HLSLTree::FExpression const*& OutExpression
    ) const override;
    UMaterialExpressionCustom* GetInternalExpression();
    virtual void GetCaption(TArray<FString>& OutCaptions) const override;
    virtual FText GetCreationName() const override { return FText::FromString(TEXT("Get rotated Bezier Points")); }
    virtual void GetExpressionToolTip(TArray<FString>& OutToolTip) override;
    virtual uint32 GetOutputType(int32 OutputIndex) override;


    //~ End UMaterialExpression Interface
#endif // WITH_EDITOR

    // Input pins
    UPROPERTY(
        meta = (
            RequiredInput = "true", 
            ToolTip = "float4 value that is 3D angular displacement vector of P0 pivot appended by 2D angular displacement value of P1 pivot"
       )
    )
    FExpressionInput InputAngularDisplacement;

    UPROPERTY(
        meta = (
            RequiredInput = "true",
            ToolTip = "float3 vector of P1 Bezier point location"
            )
    )
    FExpressionInput InputP1;

    UPROPERTY(
        meta = (
            RequiredInput = "true",
            ToolTip = "float3 vector of P2 Bezier point location"
        )
    )
    FExpressionInput InputP2;

    UPROPERTY(EditAnywhere, Category = MaterialExpressionMyNode, meta = (OverridingInputProperty = "InputAngularDisplacement"))
    FVector4f DefaultInputAngularDisplacement;

    UPROPERTY()
    TObjectPtr<UMaterialExpressionCustom> CustomExpression = nullptr;
	
};
