// [Quad Overdraw implementation based on https://blog.selfshadow.com/2012/11/12/counting-quads/ by Stephen Hill]
#ifndef __QUAD_OVERDRAW__
#define __QUAD_OVERDRAW__

RWTexture2D<uint> lockUAV : register(u0);
RWTexture2D<uint> overdrawUAV : register(u1);
RWTexture2D<uint> liveCountUAV : register(u2);

void DoQuadOverdraw(float4 svPos, uint primId)
{
    uint2 quad = svPos.xy * 0.5;
    uint prevID;
    uint unlockedID = 0xffffffff;
    bool processed = false;
    int lockCount = 0;
    int pixelCount = 0;

    for (int i = 0; i < 64; i++)
    {
        if (!processed)
            InterlockedCompareExchange(lockUAV[quad], unlockedID, primId, prevID);
        [branch]
        if (prevID == unlockedID)
        {
            if (++lockCount == 4)
            {
                // Retrieve live pixel count (minus 1) in quad
                InterlockedAnd(liveCountUAV[quad], 0, pixelCount);

                // Unlock for other quads
                InterlockedExchange(lockUAV[quad], unlockedID, prevID);
            }
            processed = true;
        }
        if (prevID == primId && !processed)
        {
            InterlockedAdd(liveCountUAV[quad], 1);
            processed = true;
        }
    }

    if (lockCount)
    {
        InterlockedAdd(overdrawUAV[quad], 1);
    }
}

#endif
