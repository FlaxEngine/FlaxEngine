// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX11

#include "GPUFanceDX11.h"
#include "../RenderToolsDX.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"

GPUFenceDX11::GPUFenceDX11(GPUDeviceDX11* device)
    : GPUFence(), _device(device)
{
    
    //ID3D11Fence that are available on Windows 10 v1703 and later.
    //but just in case
    //lets use the time querry

    ID3D11Device* DX11Device = _device->GetDevice();

    D3D11_QUERY_DESC queryDesc = {};
    queryDesc.Query = D3D11_QUERY_EVENT;

    if (DX11Device->CreateQuery(&queryDesc, &query) != S_OK) 
    {
        query = nullptr;
        return;
    }

    auto DX11DeviceContext = _device->GetIM();
    // Issue the timestamp query
    DX11DeviceContext->Begin(query);
}
void GPUFenceDX11::Signal()
{
    if (query == nullptr)
        return;

    auto DX11DeviceContext = _device->GetIM();

    // Issue the timestamp query
    DX11DeviceContext->End(query);
    
    SignalCalled = true;
}
void GPUFenceDX11::Wait()
{
    // Signal() is checking for queryEnd, queryStart so there is no need for a check
    if (SignalCalled == false)
        return;

    auto DX11DeviceContext = _device->GetIM();


    //check when the query is complete
    BOOL Timestamp = 0;
    while (true)
    {
        HRESULT Result = DX11DeviceContext->GetData(query, &Timestamp, sizeof(Timestamp), 0);
        if (Result == S_OK) 
        {
            break;
        }
        else if (Result == S_FALSE) 
        {
            //pending speepy time ZzZzZ
            Platform::Sleep(1);
        }
        else
        {
            // Handle errors
            LOG_DIRECTX_RESULT(Result);
            return;
        }
    }
}

GPUFenceDX11::~GPUFenceDX11()
{
    if (query)
        query->Release();
}

#endif
