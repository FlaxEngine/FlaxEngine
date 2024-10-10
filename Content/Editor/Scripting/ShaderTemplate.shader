%copyright%#include "./Flax/Common.hlsl"

META_CB_BEGIN(0, Data)
float4 Color;
META_CB_END

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Fullscreen(Quad_VS2PS input) : SV_Target
{
    // Solid color fill from the constant buffer passed from code
    return Color;
}
