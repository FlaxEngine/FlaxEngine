// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX11

#include "GPUFanceDX11.h"
#include "../RenderToolsDX.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"

GPUFenceDX11::GPUFenceDX11(GPUDeviceDX11* device, const StringView& name)
    : GPUResourceDX11<GPUFence>(device, name)
{
    //ID3D11Fence that are available on Windows 10 v1703 and later.
    //but just in case
    //lets use the time querry

    ID3D11Device* DX11Device = device->GetDevice();

    D3D11_QUERY_DESC queryDesc = {};
    queryDesc.Query = D3D11_QUERY_TIMESTAMP;

    if (DX11Device->CreateQuery(&queryDesc, &queryStart) != S_OK) 
    {
        queryStart = nullptr;
        return;
    }
    if (DX11Device->CreateQuery(&queryDesc, &queryEnd) != S_OK)
    {
        queryStart->Release();
        queryStart = nullptr;
        queryEnd = nullptr;
        return;
    }

    if (queryStart == nullptr || queryEnd == nullptr)
        return;
    auto DX11DeviceContext = _device->GetIM();
    // Issue the timestamp query
    DX11DeviceContext->Begin(queryStart);
}
void GPUFenceDX11::Signal()
{
    if (queryStart == nullptr || queryEnd == nullptr)
        return;

    auto DX11DeviceContext = _device->GetIM();

    // Issue the timestamp query
    DX11DeviceContext->End(queryEnd);
    
    SignalCalled = true;
}
void GPUFenceDX11::Wait()
{
    // Signal() is checking for queryEnd, queryStart so there is no need for a check
    if (SignalCalled == false)
        return;

    auto DX11DeviceContext = _device->GetIM();


    //check when the query is complete
    UINT64 startTimestamp = 0;
    UINT64 endTimestamp = 0;

    while (DX11DeviceContext->GetData(queryStart, &startTimestamp, sizeof(UINT64), 0) != S_OK) {
        Platform::Sleep(1);
    }

    while (DX11DeviceContext->GetData(queryEnd, &endTimestamp, sizeof(UINT64), 0) != S_OK) {
        Platform::Sleep(1);
    }
}

GPUFenceDX11::~GPUFenceDX11()
{
    if (queryStart)
        queryStart->Release();
    if (queryEnd)
        queryEnd->Release();
}

ID3D11Resource* GPUFenceDX11::GetResource()
{
    return nullptr;
}


#endif
