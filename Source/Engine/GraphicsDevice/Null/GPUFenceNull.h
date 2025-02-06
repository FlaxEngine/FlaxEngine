// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUFence.h"

/// <summary>
/// GPU buffer for GPUFence backend.
/// </summary>
/// <seealso cref="GPUFence" />
class GPUFenceNull : public GPUFence
{
private:
public:
    ~GPUFenceNull() {};

    virtual void Signal() override final {};
    virtual void Wait() override final {};

};
