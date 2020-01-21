cbuffer TAAConstBuffer : register(b0)
{
	float4x4 _InvNonJitterVP;
	float4x4 _InvLastVp;
	float4 _FinalBlendParameters;
	float4 _CameraDepthTexture_TexelSize;
    float4 _ScreenParams;
    float4 _ZBufferParams;
	//Align
	float3 _TemporalClipBounding;
	float _Sharpness;
	//Align
	float2 _Jitter;
	float2 _LastJitter;
};

Texture2D<float4> _MainTex : register(t0);
Texture2D<float> _DepthTex : register(t1);
Texture2D<float2> _MotionVectorTex : register(t2);
Texture2D<float> _LastDepthTex : register(t3);
Texture2D<float2> _LastMotionVectorTex : register(t4);
Texture2D<float4> _HistoryTex : register(t5);

SamplerState pointWrapSampler  : register(s0);
SamplerState pointClampSampler  : register(s1);
SamplerState linearWrapSampler  : register(s2);
SamplerState linearClampSampler  : register(s3);
SamplerState anisotropicWrapSampler  : register(s4);
SamplerState anisotropicClampSampler  : register(s5);


        struct appdata
		{
			float4 vertex : POSITION;
			float2 texcoord : TEXCOORD0;
		};

		struct v2f
		{
			float4 vertex : SV_POSITION;
			float2 texcoord : TEXCOORD0;
		};

		v2f vert(appdata v)
		{
			v2f o;
			o.vertex = v.vertex;
			o.texcoord = v.texcoord;
			return o;
		}
        #define HALF_MAX_MINUS1 65472.0 // (2 - 2^-9) * 2^15
    #ifndef AA_VARIANCE
    #define AA_VARIANCE 1
    #endif

    #ifndef AA_Filter
    #define AA_Filter 1
    #endif
    
    inline float2 LinearEyeDepth( float2 z )
    {
        return 1.0 / (_ZBufferParams.z * z + _ZBufferParams.w);
    }

    inline float2 Linear01Depth( float2 z )
{
    return 1.0 / (_ZBufferParams.x * z + _ZBufferParams.y);
}
    float Luma4(float3 Color)
    {
        return (Color.g * 2) + (Color.r + Color.b);
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    float3 RGBToYCoCg(float3 RGB)
    {
        const float3x3 mat = float3x3(0.25,0.5,0.25,0.5,0,-0.5,-0.25,0.5,-0.25);
        float3 col =mul(mat, RGB);
        return col;
    }
    
    float3 YCoCgToRGB(float3 YCoCg)
    {
        const float3x3 mat = float3x3(1,1,-1,1,0,1,1,-1,-1);
        return mul(mat, YCoCg);
    }

    float4 RGBToYCoCg(float4 RGB)
    {
        return float4(RGBToYCoCg(RGB.xyz), RGB.w);
    }



    float4 YCoCgToRGB(float4 YCoCg)
    {
        return float4(YCoCgToRGB(YCoCg.xyz), YCoCg.w); 
    }
    float Luma(float3 Color)
    {
        return (Color.g * 0.5) + (Color.r + Color.b) * 0.25;
    }
    #define TONE_BOUND 0.5
    float3 Tonemap(float3 x) 
    { 
        float luma = Luma(x);
        [flatten]
        if(luma <= TONE_BOUND) return x;
        else return x * (TONE_BOUND * TONE_BOUND - luma) / (luma * (2 * TONE_BOUND - 1 - luma));
        //return x * weight;
    }

    float3 TonemapInvert(float3 x) { 
        float luma = Luma(x);
        [flatten]
        if(luma <= TONE_BOUND) return x;
        else return x * (TONE_BOUND * TONE_BOUND - (2 * TONE_BOUND - 1) * luma) / (luma * (1 - luma));
    }

    float Pow2(float x)
    {
        return x * x;
    }

    float HdrWeight4(float3 Color, const float Exposure) 
    {
        return rcp(Luma4(Color) * Exposure + 4);
    }

    float3 ClipToAABB(float3 color, float3 minimum, float3 maximum)
    {
        // Note: only clips towards aabb center (but fast!)
        float3 center = 0.5 * (maximum + minimum);
        float3 extents = 0.5 * (maximum - minimum);

        // This is actually `distance`, however the keyword is reserved
        float3 offset = color.rgb - center;

        float3 ts = abs(extents / (offset + 0.0001));
        float t = saturate(min(min(ts.x, ts.y), ts.z));
        color.rgb = center + offset * t;
        return color;
    }

    static const int2 _OffsetArray[8] = {
        int2(-1, -1),
        int2(0, -1),
        int2(1, -1),
        int2(-1, 0),
        int2(1, 1),
        int2(1, 0),
        int2(-1, 1),
        int2(0, -1)
    };
    #define COMPARE_DEPTH(a, b) step(b, a)

float2 ReprojectedMotionVectorUV(float2 uv, out float outDepth)
    {
        float neighborhood;
        const float2 k = _CameraDepthTexture_TexelSize.xy;
        uint i;
        outDepth = _DepthTex.SampleLevel(linearClampSampler, uv, 0).x;
        float3 result = float3(0, 0,  outDepth);
        
        [unroll]
        for(i = 0; i < 8; ++i){
            neighborhood = _DepthTex.SampleLevel(linearClampSampler, uv, 0, _OffsetArray[i]).x;
            result = lerp(result, float3(_OffsetArray[i], neighborhood), COMPARE_DEPTH(neighborhood, result.z));
        }
        return uv + result.xy * k;
    }

#define SAMPLE_TEXTURE2D_OFFSET(tex, samp, uv, offset) tex.SampleLevel(samp, uv, 0, offset)

float4 frag(v2f i) : SV_TARGET
{
   const float ExposureScale = 10;

        float2 uv = (i.texcoord - _Jitter);
        float2 screenSize = _ScreenParams.xy;
        float depth;

        float2 closest = ReprojectedMotionVectorUV(i.texcoord, depth);
        float2 velocity = _MotionVectorTex.SampleLevel(linearClampSampler, closest, 0).xy;// SAMPLE_TEXTURE2D(_CameraMotionVectorsTexture, sampler_CameraMotionVectorsTexture, closest).xy;
        float2 PrevCoord = (i.texcoord - velocity);
        float4 MiddleCenter = _MainTex.SampleLevel(linearClampSampler, uv, 0);
        if (PrevCoord.x > 1 || PrevCoord.y > 1 || PrevCoord.x < 0 || PrevCoord.y < 0) {
            return float4(MiddleCenter.xyz, 1);
        }
        

        float4 TopLeft = SAMPLE_TEXTURE2D_OFFSET(_MainTex, linearClampSampler, uv, int2(-1, -1));
        float4 TopCenter = SAMPLE_TEXTURE2D_OFFSET(_MainTex, linearClampSampler, uv, int2(0, -1));
        float4 TopRight = SAMPLE_TEXTURE2D_OFFSET(_MainTex, linearClampSampler, uv, int2(1, -1));
        float4 MiddleLeft = SAMPLE_TEXTURE2D_OFFSET(_MainTex, linearClampSampler, uv, int2(-1,  0));
        float4 MiddleRight = SAMPLE_TEXTURE2D_OFFSET(_MainTex, linearClampSampler, uv, int2(1,  0));
        float4 BottomLeft = SAMPLE_TEXTURE2D_OFFSET(_MainTex, linearClampSampler, uv, int2(-1, 1));
        float4 BottomCenter = SAMPLE_TEXTURE2D_OFFSET(_MainTex, linearClampSampler, uv, int2(0, 1));
        float4 BottomRight = SAMPLE_TEXTURE2D_OFFSET(_MainTex, linearClampSampler, uv, int2(1, 1));
        
        float SampleWeights[9];
        SampleWeights[0] = HdrWeight4(TopLeft.rgb, ExposureScale);
        SampleWeights[1] = HdrWeight4(TopCenter.rgb, ExposureScale);
        SampleWeights[2] = HdrWeight4(TopRight.rgb, ExposureScale);
        SampleWeights[3] = HdrWeight4(MiddleLeft.rgb, ExposureScale);
        SampleWeights[4] = HdrWeight4(MiddleCenter.rgb, ExposureScale);
        SampleWeights[5] = HdrWeight4(MiddleRight.rgb, ExposureScale);
        SampleWeights[6] = HdrWeight4(BottomLeft.rgb, ExposureScale);
        SampleWeights[7] = HdrWeight4(BottomCenter.rgb, ExposureScale);
        SampleWeights[8] = HdrWeight4(BottomRight.rgb, ExposureScale);
        TopLeft = RGBToYCoCg(TopLeft);
        TopCenter = RGBToYCoCg(TopCenter);
        TopRight = RGBToYCoCg(TopRight);
        MiddleLeft = RGBToYCoCg(MiddleLeft);
        MiddleCenter = RGBToYCoCg(MiddleCenter);
        MiddleRight = RGBToYCoCg(MiddleRight);
        BottomLeft = RGBToYCoCg(BottomLeft);
        BottomCenter = RGBToYCoCg(BottomCenter);
        BottomRight = RGBToYCoCg(BottomRight);
        float TotalWeight = SampleWeights[0] + SampleWeights[1] + SampleWeights[2] + SampleWeights[3] + SampleWeights[4] + SampleWeights[5] + SampleWeights[6] + SampleWeights[7] + SampleWeights[8];                   
        float4 Filtered = (TopLeft * SampleWeights[0] + TopCenter * SampleWeights[1] + TopRight * SampleWeights[2] + MiddleLeft * SampleWeights[3] + MiddleCenter * SampleWeights[4] + MiddleRight * SampleWeights[5] + BottomLeft * SampleWeights[6] + BottomCenter * SampleWeights[7] + BottomRight * SampleWeights[8]) / TotalWeight;
            
        // Resolver Average
        float VelocityLength = length(velocity);
        float VelocityWeight = saturate(VelocityLength * _TemporalClipBounding.z);
        float AABBScale = lerp(_TemporalClipBounding.x, _TemporalClipBounding.y, VelocityWeight);

        float4 m1 = TopLeft + TopCenter + TopRight + MiddleLeft + MiddleCenter + MiddleRight + BottomLeft + BottomCenter + BottomRight;
        float4 m2 = TopLeft * TopLeft + TopCenter * TopCenter + TopRight * TopRight + MiddleLeft * MiddleLeft + MiddleCenter * MiddleCenter + MiddleRight * MiddleRight + BottomLeft * BottomLeft + BottomCenter * BottomCenter + BottomRight * BottomRight;
        
        float4 mean = m1 / 9;
        float4 stddev = sqrt(m2 / 9 - mean * mean);  

        float4 minColor = mean - AABBScale * stddev;
        float4 maxColor = mean + AABBScale * stddev;
        minColor = min(minColor, Filtered);
        maxColor = max(maxColor, Filtered);


//////////////////TemporalResolver
        float4 CurrColor = YCoCgToRGB(MiddleCenter);

        // Sharpen output
        float4 corners = ( YCoCgToRGB(TopLeft + BottomRight + TopRight + BottomLeft) - CurrColor ) * 2;
        CurrColor += ( CurrColor - (corners * 0.166667) ) * 2.718282 * _Sharpness;

        CurrColor = clamp(CurrColor, 0, HALF_MAX_MINUS1);
        
        // HistorySample
        float2 prevDepthUV = PrevCoord + _Jitter - _LastJitter;
        float lastFrameDepth = _LastDepthTex.SampleLevel(linearClampSampler, prevDepthUV, 0); //_LastFrameDepthTexture.Sample(sampler_LastFrameDepthTexture, prevDepthUV);
        float2 lastFrameMV = _LastMotionVectorTex.SampleLevel(linearClampSampler, prevDepthUV, 0);//_LastFrameMotionVectors.Sample(sampler_LastFrameMotionVectors, prevDepthUV);
        float lastFrameMVLen = dot(lastFrameMV, lastFrameMV);

        [unroll]
        for(uint ite = 0; ite < 8; ++ite)
        {
            float2 currentMV = _LastMotionVectorTex.SampleLevel(linearClampSampler, prevDepthUV, 0, _OffsetArray[ite]);// _LastFrameMotionVectors.Sample(sampler_LastFrameMotionVectors, prevDepthUV, _OffsetArray[ite]);
            float currentMVLen = dot(currentMV, currentMV);
            lastFrameMVLen = max(currentMVLen, lastFrameMVLen);
        }

        float LastVelocityWeight = saturate(sqrt(lastFrameMVLen) * _TemporalClipBounding.z);
        float4 worldPos = mul(_InvNonJitterVP, float4(i.texcoord, depth, 1));
        float4 lastWorldPos = mul(_InvLastVp, float4(prevDepthUV, lastFrameDepth, 1));
        worldPos /= worldPos.w; lastWorldPos /= lastWorldPos.w;
        worldPos -= lastWorldPos;
        float depthAdaptiveForce = 1 - saturate((dot(worldPos.xyz, worldPos.xyz) - 0.02) * 10);
        float4 PrevColor = _HistoryTex.SampleLevel(linearClampSampler, PrevCoord, 0);//SAMPLE_TEXTURE2D(_HistoryTex, sampler_HistoryTex, PrevCoord);
        float colDiff = depthAdaptiveForce - PrevColor.w;//Whether current Color is brighter than last
        float tWeight = lerp(0.7, 0.9, saturate(tanh(colDiff * 2) * 0.5 + 0.5));
        depthAdaptiveForce = lerp(depthAdaptiveForce, PrevColor.w, tWeight);
        depthAdaptiveForce = lerp(depthAdaptiveForce, 1, VelocityWeight);
        depthAdaptiveForce = lerp(depthAdaptiveForce, 1, LastVelocityWeight);
        float2 depth01 = Linear01Depth(float2(lastFrameDepth, depth));
        float finalDepthAdaptive = lerp(depthAdaptiveForce, 1, (depth01.x > 0.9999) || (depth01.y > 0.9999));
        PrevColor.xyz =  lerp(PrevColor.xyz, YCoCgToRGB( ClipToAABB( RGBToYCoCg(PrevColor.xyz), minColor.xyz, maxColor.xyz )), finalDepthAdaptive);
        float HistoryWeight = lerp(_FinalBlendParameters.x, _FinalBlendParameters.y, VelocityWeight);
        CurrColor.xyz = Tonemap(CurrColor.xyz);
        PrevColor.xyz = Tonemap(PrevColor.xyz);
        float4 TemporalColor = lerp(CurrColor, PrevColor, HistoryWeight);
        TemporalColor.xyz = TonemapInvert(TemporalColor.xyz);
        TemporalColor.w = depthAdaptiveForce;
        return max(0, TemporalColor);
}