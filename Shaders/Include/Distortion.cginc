#ifndef UNITY_POSTFX_DISTORTION
#define UNITY_POSTFX_DISTORTION

float2 Distort(float2 uv, float4 _Distortion_CenterScale, float4 _Distortion_Amount)
{

    
        uv = (uv - 0.5) * _Distortion_Amount.z + 0.5;
        float2 ruv = _Distortion_CenterScale.zw * (uv - 0.5 - _Distortion_CenterScale.xy);
        float ru = length(float2(ruv));

        [branch]
        if (_Distortion_Amount.w > 0.0)
        {
            float wu = ru * _Distortion_Amount.x;
            ru = tan(wu) * (1.0 / (ru * _Distortion_Amount.y));
            uv = uv + ruv * (ru - 1.0);
        }
        else
        {
            ru = (1.0 / ru) * _Distortion_Amount.x * atan(ru * _Distortion_Amount.y);
            uv = uv + ruv * (ru - 1.0);
        }
    
    return uv;
}

float2 DistortCheck(float2 uv, bool l, float4 _Distortion_CenterScale, float4 _Distortion_Amount)
{
	return l ? uv : Distort(uv, _Distortion_CenterScale, _Distortion_Amount);
}

#endif // UNITY_POSTFX_DISTORTION
