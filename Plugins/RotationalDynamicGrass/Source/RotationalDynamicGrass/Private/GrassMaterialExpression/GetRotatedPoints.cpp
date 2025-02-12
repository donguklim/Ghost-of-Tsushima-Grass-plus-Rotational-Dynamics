#include "GrassMaterialExpression/GetRotatedPoints.h"
#include "MaterialCompiler.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"

UGetRotatedPoints::UGetRotatedPoints(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer)
{
    DefaultInputAngularDisplacement = FVector4f(0, 0, 0, 0);
#if WITH_EDITORONLY_DATA

    Outputs.Reset();
    Outputs.Add(FExpressionOutput(TEXT("P1"), 1, 1, 1, 1, 0));
    Outputs.Add(FExpressionOutput(TEXT("P2"), 1, 1, 1, 1, 0));
    Outputs.Add(FExpressionOutput(TEXT("Side Direction"), 1, 1, 1, 1, 0));

#endif
}


#if WITH_EDITOR

void UGetRotatedPoints::GetExpressionToolTip(TArray<FString>& OutToolTip) {
    OutToolTip.Add(TEXT("Get P1, P2 and Curve side direction after the angular displacement is applied"));
}

void UGetRotatedPoints::GetCaption(TArray<FString>& OutCaptions) const
{
    OutCaptions.Add(TEXT("P1, P2, Curve side direction"));
}

uint32 UGetRotatedPoints::GetOutputType(int32 OutputIndex)
{
    switch (OutputIndex)
    {
    case 0: return MCT_Float3;  // P1 position
    case 1: return MCT_Float3; // P2 position
    case 2: return MCT_Float3;  // Grass side direction
    default: return MCT_Unknown;
    }
}

UMaterialExpressionCustom* UGetRotatedPoints::GetInternalExpression()
{
    if (CustomExpression)
    {
        return CustomExpression;
    }

    CustomExpression = NewObject<UMaterialExpressionCustom>();
    CustomExpression->Inputs[0].InputName = TEXT("InputAngularDisplacement"); // the first input is already added
    CustomExpression->Inputs.Add({ TEXT("InputP1") });
    CustomExpression->Inputs.Add({ TEXT("InputP2") });
    CustomExpression->OutputType = ECustomMaterialOutputType::CMOT_Float3;
    CustomExpression->AdditionalOutputs.Add({ TEXT("P2"), ECustomMaterialOutputType::CMOT_Float3 });
    CustomExpression->AdditionalOutputs.Add({ TEXT("SideDir"), ECustomMaterialOutputType::CMOT_Float3 });
    CustomExpression->IncludeFilePaths.Add("/RotationalDynamicGrass/Shaders/GrassMotionShader.ush");
    return CustomExpression;
}

int32 UGetRotatedPoints::Compile(FMaterialCompiler* Compiler, int32 OutputIndex)
{
    /*
    int32 AngularDispCode = InputAngularDisplacement.GetTracedInput().Expression ? InputAngularDisplacement.Compile(Compiler) :
        Compiler->Constant4(0.0f, 0.0f, 0.0f, 0.0f);

    int32 P1Code = InputP1.Compile(Compiler);
    int32 P2Code = InputP2.Compile(Compiler);


    if (AngularDispCode == INDEX_NONE)
    {
        return Compiler->Errorf(TEXT("Invalid Angular Displacement input"));
    }

    if (P1Code == INDEX_NONE)
    {
        return Compiler->Errorf(TEXT("Invalid P1 input"));
    }

    if (P2Code == INDEX_NONE)
    {
        return Compiler->Errorf(TEXT("Invalid P2 input"));
    }

    Compiler->Custom
    // Include our shader file
    Compiler->IncludeStatement(TEXT("/Game/Shaders/PatternFunctions.ush"));

    // Create the function call that returns a struct
    FString FunctionCall = FString::Printf(TEXT("ComputePatternWithDerivatives(%s, %.9f)"),
        *Compiler->GetParameterCode(UVCode), Scale);

    // Access the appropriate member based on OutputIndex
    switch (OutputIndex)
    {
        case 0: // Pattern
            return Compiler->CustomExpression(
                FString::Printf(TEXT("(%s).Pattern"), *FunctionCall),
                { UVCode },
                MCT_Float
            );

        case 1: // Derivatives
            return Compiler->CustomExpression(
                FString::Printf(TEXT("(%s).Derivatives"), *FunctionCall),
                { UVCode },
                MCT_Float2
            );

        case 2: // Magnitude
            return Compiler->CustomExpression(
                FString::Printf(TEXT("(%s).Magnitude"), *FunctionCall),
                { UVCode },
                MCT_Float
            );

        default:
            return Compiler->Errorf(TEXT("Invalid output index"));
    }
    */

    UMaterialExpressionCustom* InternalExpression = GetInternalExpression();
    if (!InternalExpression)
    {
        return Compiler->Errorf(TEXT("Internal expression is null."));
    }

    InternalExpression->Code = TEXT(R"(
        return GetBezierPoints(InputP1, InputP2, InputAngularDisplacement, P1, P2, SideDir);
    )");
    

    // Just to be safe, clear out the InternalExpression input. This should only be used by GenerateHLSLExpression
    // Remove this if not useing GenerateHLSLExpression.
    InternalExpression->Inputs[0].Input = FExpressionInput();
    InternalExpression->Inputs[1].Input = FExpressionInput();
    InternalExpression->Inputs[2].Input = FExpressionInput();


    int32 AngularDispCode = InputAngularDisplacement.GetTracedInput().Expression ? InputAngularDisplacement.Compile(Compiler) :
        Compiler->Constant4(
            DefaultInputAngularDisplacement.X,
            DefaultInputAngularDisplacement.Y,
            DefaultInputAngularDisplacement.Z,
            DefaultInputAngularDisplacement.W
        );

    int32 P1Code = InputP1.Compile(Compiler);
    int32 P2Code = InputP2.Compile(Compiler);

    // return Compiler->Add(P1Code, P2Code);
    TArray<int32> Inputs{ AngularDispCode, P1Code, P2Code };

    return Compiler->CustomExpression(InternalExpression, OutputIndex, Inputs);
}

bool UGetRotatedPoints::GenerateHLSLExpression(
    FMaterialHLSLGenerator& Generator,
    UE::HLSLTree::FScope& Scope,
    int32 OutputIndex,
    UE::HLSLTree::FExpression const*& OutExpression
) const
{
    
    UMaterialExpressionCustom* InternalExpression = CustomExpression;
    if (!InternalExpression)
    {
        return Generator.Errorf(TEXT("Internal expression is null."));
    }

    InternalExpression->Code = TEXT(R"(
        return GetBezierPoints(InputP1, InputP2, InputAngularDisplacement, P1, P2, SideDir);
    )");

    InternalExpression->Inputs[0].Input = InputAngularDisplacement;
    InternalExpression->Inputs[1].Input = InputP1;
    InternalExpression->Inputs[2].Input = InputP2;

    return InternalExpression->GenerateHLSLExpression(Generator, Scope, OutputIndex, OutExpression);

    /*
    // Adds A and B together, and optionally negates the result.
    const UE::HLSLTree::FExpression* InputAExpression = InputP1.AcquireHLSLExpression(Generator, Scope);
    const UE::HLSLTree::FExpression* InputBExpression = InputP2.AcquireHLSLExpression(Generator, Scope);
    if (!InputAExpression || !InputBExpression)
    {
        return false;
    }

    OutExpression = Generator.GetTree().NewAdd(InputAExpression, InputAExpression);
    */
   

    return true;
}

#endif
