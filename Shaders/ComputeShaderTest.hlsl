RWTexture2D<float4> _MainTex[2] : register(u0, space0);
[numthreads(8,8,1)]
void CSMain(uint2 id : SV_DISPATCHTHREADID)
{
    _MainTex[1][id] = dot((_MainTex[0][id * 2] + _MainTex[0][id * 2 + uint2(0, 1)] + _MainTex[0][id * 2 + uint2(1, 0)] + _MainTex[0][id * 2 + 1]), float4(0.3333,0.3333,0.3333,0)) * 0.25;
}