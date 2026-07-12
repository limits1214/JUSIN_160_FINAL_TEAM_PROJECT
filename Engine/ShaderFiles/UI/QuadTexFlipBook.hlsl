#include "../ShaderDefines.hlsl"

Texture2D tex : register(t0);
//SamplerState samp : register(s0);

struct VS_IN
{
    float3 posL : POSITION;
    float2 uv : TEXCOORD;
};

struct PS_IN
{
    float4 posH : SV_POSITION;
    float2 uv : TEXCOORD;
};

PS_IN VSMain(VS_IN vin)
{
    PS_IN output;
    output.posH = mul(float4(vin.posL, 1.f), g_matWVP);

    output.uv = vin.uv;

    return output;
}

// Pixel Shader
float4 PSMain(PS_IN input) : SV_Target
{
    float2 uv = g_ui_texCoord + input.uv * g_ui_uvSize;
    
    float4 texColor = tex.Sample(LinearWrap, uv);
    
    if (max(texColor.r, max(texColor.g, texColor.b)) < 0.01f)
    {
        discard;
    }
    
    float alpha = dot(texColor.rgb, float3(0.299, 0.587, 0.114));
    alpha = pow(alpha, 0.5f);
    
    if (alpha < 0.1f)
    {
        discard;
    }
    
    float brightness = dot(texColor.rgb, float3(0.299, 0.587, 0.114));
    brightness = pow(brightness, 1.f);
    
    if (max(g_ui_color.r, max(g_ui_color.g, g_ui_color.b)) > 0.0f)
    {
        texColor.rgb = g_ui_color.rgb * brightness;
    }
    
    return float4(texColor.rgb, brightness * g_ui_color.a);
}
