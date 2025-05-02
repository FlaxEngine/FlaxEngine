// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX11

#include "GPUTimerQueryDX11.h"

GPUTimerQueryDX11::GPUTimerQueryDX11(GPUDeviceDX11* device)
    : GPUResourceDX11<GPUTimerQuery>(device, String::Empty)
{
    D3D11_QUERY_DESC queryDesc;
    queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    queryDesc.MiscFlags = 0;

    HRESULT hr = device->GetDevice()->CreateQuery(&queryDesc, &_disjointQuery);
    if (hr != S_OK)
    {
        LOG(Fatal, "Failed to create a timer query.");
    }

    queryDesc.Query = D3D11_QUERY_TIMESTAMP;

    hr = device->GetDevice()->CreateQuery(&queryDesc, &_beginQuery);
    if (hr != S_OK)
    {
        LOG(Fatal, "Failed to create a timer query.");
    }

    hr = device->GetDevice()->CreateQuery(&queryDesc, &_endQuery);
    if (hr != S_OK)
    {
        LOG(Fatal, "Failed to create a timer query.");
    }

    // Set non-zero mem usage (fake)
    _memoryUsage = sizeof(D3D11_QUERY_DESC) * 3;
}

GPUTimerQueryDX11::~GPUTimerQueryDX11()
{
    if (_beginQuery)
        _beginQuery->Release();
    if (_endQuery)
        _endQuery->Release();
    if (_disjointQuery)
        _disjointQuery->Release();
}

void GPUTimerQueryDX11::OnReleaseGPU()
{
    SAFE_RELEASE(_beginQuery);
    SAFE_RELEASE(_endQuery);
    SAFE_RELEASE(_disjointQuery);
}

ID3D11Resource* GPUTimerQueryDX11::GetResource()
{
    return nullptr;
}

void GPUTimerQueryDX11::Begin()
{
    auto context = _device->GetIM();
    context->Begin(_disjointQuery);
    context->End(_beginQuery);

    _endCalled = false;
}

void GPUTimerQueryDX11::End()
{
    if (_endCalled)
        return;

    auto context = _device->GetIM();
    context->End(_endQuery);
    context->End(_disjointQuery);

    _endCalled = true;
    _finalized = false;
}

bool GPUTimerQueryDX11::HasResult()
{
    if (!_endCalled)
        return false;

    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
    return _device->GetIM()->GetData(_disjointQuery, &disjointData, sizeof(disjointData), 0) == S_OK;
}

float GPUTimerQueryDX11::GetResult()
{
    if (!_finalized)
    {
#if BUILD_DEBUG
        ASSERT(HasResult());
#endif

        UINT64 timeStart, timeEnd;
        auto context = _device->GetIM();

        context->GetData(_beginQuery, &timeStart, sizeof(timeStart), 0);
        context->GetData(_endQuery, &timeEnd, sizeof(timeEnd), 0);

        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
        context->GetData(_disjointQuery, &disjointData, sizeof(disjointData), 0);

        if (disjointData.Disjoint == FALSE)
        {
            const UINT64 delta = timeEnd - timeStart;
            _timeDelta = (float)(((double)delta / (double)disjointData.Frequency) * 1000.0);
        }
        else
        {
            _timeDelta = 0.0f;
#if !BUILD_RELEASE
            static bool SingleShotLog = false;
            if (!SingleShotLog)
            {
                SingleShotLog = true;
                LOG(Warning, "Unreliable GPU timer query detected.");
            }
#endif
        }

        _finalized = true;
    }
    return _timeDelta;
}

#endif
