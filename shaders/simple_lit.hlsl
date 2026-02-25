// Simple HLSL shaders: provide a minimal vertex shader entrypoint named "main"
// so older project build steps that compile the file as a vertex shader succeed,
// and provide a pixel shader entrypoint named "ps_main" for runtime use.

cbuffer FrameData : register(b0) {
    float3 LightDir;
    float3 LightColor;
    float LightIntensity;
};

struct VSOutput {
    float4 pos : POSITION;    // position output for older vs profiles
    float3 normal : TEXCOORD0;
    float2 uv : TEXCOORD1;
};

// Minimal vertex shader named 'main' to satisfy tools that compile as a vertex shader.
VSOutput main(float4 inPos : POSITION, float3 inNormal : NORMAL0, float2 inUV : TEXCOORD0) {
    VSOutput o;
    o.pos = inPos;
    o.normal = inNormal;
    o.uv = inUV;
    return o;
}

// Pixel shader entrypoint used at runtime. Use COLOR semantic for compatibility with
// older compilation steps; the build script compiles this with ps_5_0 explicitly.
float4 ps_main(VSOutput IN) : COLOR {
    float3 N = normalize(IN.normal);
    float ndotl = max(dot(N, -LightDir), 0.0);
    float3 baseColor = float3(1.0, 0.8, 0.8);
    float3 lit = lerp(baseColor, baseColor * LightColor, ndotl * LightIntensity);
    return float4(lit, 1.0);
}
