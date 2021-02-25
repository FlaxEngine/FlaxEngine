// This code was auto-generated. Do not modify it.

#include "Engine/Scripting/BinaryModule.h"
#include "FlaxEngine.Gen.h"

StaticallyLinkedBinaryModuleInitializer StaticallyLinkedBinaryModuleFlaxEngine(GetBinaryModuleFlaxEngine);

extern "C" BinaryModule* GetBinaryModuleFlaxEngine()
{
    static NativeBinaryModule module("FlaxEngine", MAssemblyOptions());
    return &module;
}
