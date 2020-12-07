// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

class IShaderResourceOGL
{
public:

    virtual void Bind(int32 slotIndex) = 0;
};
