#include "Include/Sampler.cginc"
#include "Include/D3D12.hlsl"
#include "Include/StdLib.cginc"
Texture2D<float4> _MainTex : register(t0, space0);

Texture2D<float> _CameraDepthTexture : register(t1, space0);
Texture2D<float2> _CameraMotionVectorsTexture : register(t2, space0);
Texture2D<float4> _NeighborMaxTex : register(t3, space0);
Texture2D<float4> _VelocityTex : register(t4, space0);

cbuffer Params : register(b0)
{
    float4 _ZBufferParams;
    float4 _NeighborMaxTex_TexelSize;
    float4 _VelocityTex_TexelSize;
    float4 _MainTex_TexelSize;
    float2 _ScreenParams;
    float _VelocityScale;
    float _MaxBlurRadius;
    float _RcpMaxBlurRadius;
    float _TileMaxLoop;
    float _LoopCount;
    float _TileMaxOffs;
};

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

         float4 FragVelocitySetup(v2f i) : SV_Target
        {
            // Sample the motion vector.
            float2 v = SAMPLE_TEXTURE2D_LOD(_CameraMotionVectorsTexture, bilinearClampSampler, i.texcoord, 0).rg;
            // Apply the exposure time and convert to the pixel space.
            v *= (_VelocityScale * 0.5) * _MainTex_TexelSize.zw;

            // Clamp the vector with the maximum blur radius.
            v /= max(1.0, length(v) * _RcpMaxBlurRadius);

            // Sample the depth of the pixel.
            float d = Linear01Depth(_CameraDepthTexture.SampleLevel(bilinearClampSampler, i.texcoord, 0).r, _ZBufferParams);

            // Pack into 10/10/10/2 format.
            return float4((v * _RcpMaxBlurRadius + 1.0) * 0.5, d, 0.0);
        }

        float2 MaxV(float2 v1, float2 v2)
        {
            return dot(v1, v1) < dot(v2, v2) ? v2 : v1;
        }

        // TileMax filter (2 pixel width with normalization)
        float4 FragTileMax1(v2f i) : SV_Target
        {
            float4 d = _MainTex_TexelSize.xyxy * float4(-0.5, -0.5, 0.5, 0.5);

            float2 v1 = SAMPLE_TEXTURE2D_LOD(_MainTex, bilinearClampSampler, i.texcoord + d.xy, 0).rg;
            float2 v2 = SAMPLE_TEXTURE2D_LOD(_MainTex, bilinearClampSampler, i.texcoord + d.zy, 0).rg;
            float2 v3 = SAMPLE_TEXTURE2D_LOD(_MainTex, bilinearClampSampler, i.texcoord + d.xw, 0).rg;
            float2 v4 = SAMPLE_TEXTURE2D_LOD(_MainTex, bilinearClampSampler, i.texcoord + d.zw, 0).rg;

            v1 = (v1 * 2.0 - 1.0) * _MaxBlurRadius;
            v2 = (v2 * 2.0 - 1.0) * _MaxBlurRadius;
            v3 = (v3 * 2.0 - 1.0) * _MaxBlurRadius;
            v4 = (v4 * 2.0 - 1.0) * _MaxBlurRadius;

            return float4(MaxV(MaxV(MaxV(v1, v2), v3), v4), 0.0, 0.0);
        }

        // TileMax filter (2 pixel width)
        float4 FragTileMax2(v2f i) : SV_Target
        {
            float4 d = _MainTex_TexelSize.xyxy * float4(-0.5, -0.5, 0.5, 0.5);

            float2 v1 = SAMPLE_TEXTURE2D_LOD(_MainTex, bilinearClampSampler, i.texcoord + d.xy, 0).rg;
            float2 v2 = SAMPLE_TEXTURE2D_LOD(_MainTex, bilinearClampSampler, i.texcoord + d.zy, 0).rg;
            float2 v3 = SAMPLE_TEXTURE2D_LOD(_MainTex, bilinearClampSampler, i.texcoord + d.xw, 0).rg;
            float2 v4 = SAMPLE_TEXTURE2D_LOD(_MainTex, bilinearClampSampler, i.texcoord + d.zw, 0).rg;

            return float4(MaxV(MaxV(MaxV(v1, v2), v3), v4), 0.0, 0.0);
        }

        // TileMax filter (variable width)
        float4 FragTileMaxV(v2f i) : SV_Target
        {
            float2 uv0 = i.texcoord + _MainTex_TexelSize.xy * _TileMaxOffs;

            float2 du = float2(_MainTex_TexelSize.x, 0.0);
            float2 dv = float2(0.0, _MainTex_TexelSize.y);

            float2 vo = 0.0;

            [loop]
            for (int ix = 0; ix < _TileMaxLoop; ix++)
            {
                [loop]
                for (int iy = 0; iy < _TileMaxLoop; iy++)
                {
                    float2 uv = uv0 + du * ix + dv * iy;
                    vo = MaxV(vo, SAMPLE_TEXTURE2D_LOD(_MainTex, bilinearClampSampler, uv, 0).rg);
                }
            }

            return float4(vo, 0.0, 0.0);
        }

        // NeighborMax filter
        float4 FragNeighborMax(v2f i) : SV_Target
        {
            const float cw = 1.01; // Center weight tweak

            float4 d = _MainTex_TexelSize.xyxy * float4(1.0, 1.0, -1.0, 0.0);

            float2 v1 = SAMPLE_TEXTURE2D_LOD(_MainTex, bilinearClampSampler, i.texcoord - d.xy, 0).rg;
            float2 v2 = SAMPLE_TEXTURE2D_LOD(_MainTex, bilinearClampSampler, i.texcoord - d.wy, 0).rg;
            float2 v3 = SAMPLE_TEXTURE2D_LOD(_MainTex, bilinearClampSampler, i.texcoord - d.zy, 0).rg;

            float2 v4 = SAMPLE_TEXTURE2D_LOD(_MainTex, bilinearClampSampler, i.texcoord - d.xw, 0).rg;
            float2 v5 = SAMPLE_TEXTURE2D_LOD(_MainTex, bilinearClampSampler, i.texcoord, 0).rg * cw;
            float2 v6 = SAMPLE_TEXTURE2D_LOD(_MainTex, bilinearClampSampler, i.texcoord + d.xw, 0).rg;

            float2 v7 = SAMPLE_TEXTURE2D_LOD(_MainTex, bilinearClampSampler, i.texcoord + d.zy, 0).rg;
            float2 v8 = SAMPLE_TEXTURE2D_LOD(_MainTex, bilinearClampSampler, i.texcoord + d.wy, 0).rg;
            float2 v9 = SAMPLE_TEXTURE2D_LOD(_MainTex, bilinearClampSampler, i.texcoord + d.xy, 0).rg;

            float2 va = MaxV(v1, MaxV(v2, v3));
            float2 vb = MaxV(v4, MaxV(v5, v6));
            float2 vc = MaxV(v7, MaxV(v8, v9));
            return float4(MaxV(va, MaxV(vb, vc)) * (1.0 / cw), 0.0, 0.0);
        }

        // -----------------------------------------------------------------------------
        // Reconstruction

        // Returns true or false with a given interval.
        bool Interval(float phase, float interval)
        {
            return frac(phase / interval) > 0.499;
        }

        // Jitter function for tile lookup
        float2 JitterTile(float2 uv)
        {
            float rx, ry;
            sincos(GradientNoise(uv + float2(2.0, 0.0), _ScreenParams) * TWO_PI, ry, rx);
            return float2(rx, ry) * _NeighborMaxTex_TexelSize.xy * 0.25;
        }

        // Velocity sampling function
        float3 SampleVelocity(float2 uv)
        {
            float3 v = SAMPLE_TEXTURE2D_LOD(_VelocityTex, bilinearClampSampler, uv, 0.0).xyz;
            return float3((v.xy * 2.0 - 1.0) * _MaxBlurRadius, v.z);
        }

        // Reconstruction filter
        float4 FragReconstruction(v2f i) : SV_Target
        {
            //return float4(i.texcoord, 0, 0);
            // Color sample at the center point
            const float4 c_p = _MainTex[i.vertex.xy];

            // Velocity/Depth sample at the center point
            const float3 vd_p = SampleVelocity(i.texcoord);
            const float l_v_p = max(length(vd_p.xy), 0.5);
            const float rcp_d_p = 1.0 / vd_p.z;

            // NeighborMax vector sample at the center point
            const float2 v_max = SAMPLE_TEXTURE2D_LOD(_NeighborMaxTex, bilinearClampSampler, i.texcoord + JitterTile(i.texcoord), 0).xy;
            const float l_v_max = length(v_max);
            // Escape early if the NeighborMax vector is small enough.
            if (l_v_max < 2.0) return c_p;
            const float rcp_l_v_max = 1.0 / l_v_max;
            // Use V_p as a secondary sampling direction except when it's too small
            // compared to V_max. This vector is rescaled to be the length of V_max.
            const float2 v_alt = (l_v_p * 2.0 > l_v_max) ? vd_p.xy * (l_v_max / l_v_p) : v_max;

            // Determine the sample count.
            const float sc = floor(min(_LoopCount, l_v_max * 0.5));

            // Loop variables (starts from the outermost sample)
            const float dt = 1.0 / sc;
            const float t_offs = (GradientNoise(i.texcoord, _ScreenParams) - 0.5) * dt;
            float t = 1.0 - dt * 0.5;
            float count = 0.0;

            // Background velocity
            // This is used for tracking the maximum velocity in the background layer.
            float l_v_bg = max(l_v_p, 1.0);

            // Color accumlation
            float4 acc = 0.0;

            [loop]
            while (t > dt * 0.25)
            {
                // Sampling direction (switched per every two samples)
                const float2 v_s = Interval(count, 4.0) ? v_alt : v_max;

                // Sample position (inverted per every sample)
                const float t_s = (Interval(count, 2.0) ? -t : t) + t_offs;

                // Distance to the sample position
                const float l_t = l_v_max * abs(t_s);

                // UVs for the sample position
                const float2 uv0 = i.texcoord + v_s * t_s * _MainTex_TexelSize.xy;
                const float2 uv1 = i.texcoord + v_s * t_s * _VelocityTex_TexelSize.xy;

                // Color sample
                const float3 c = SAMPLE_TEXTURE2D_LOD(_MainTex, bilinearClampSampler, uv0, 0.0).rgb;
                // Velocity/Depth sample
                const float3 vd = SampleVelocity(uv1);

                // Background/Foreground separation
                const float fg = saturate((vd_p.z - vd.z) * 20.0 * rcp_d_p);

                // Length of the velocity vector
                const float l_v = lerp(l_v_bg, length(vd.xy), fg);

                // Sample weight
                // (Distance test) * (Spreading out by motion) * (Triangular window)
                const float w = saturate(l_v - l_t) / l_v * (1.2 - t);

                // Color accumulation
                acc += float4(c, 1.0) * w;

                // Update the background velocity.
                l_v_bg = max(l_v_bg, l_v);

                // Advance to the next sample.
                t = Interval(count, 2.0) ? t - dt : t;
                count += 1.0;
            }

            // Add the center sample.
            acc += float4(c_p.rgb, 1.0) * (1.2 / (l_v_bg * sc * 2.0));

            return float4(acc.rgb / acc.a, c_p.a);
        }
        
        