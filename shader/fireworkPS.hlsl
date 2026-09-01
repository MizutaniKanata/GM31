#include "common.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    // UV中心(0.5, 0.5)からの距離を計算し、火花のグロー(減光)カーブを作る
    float2 centered = In.TexCoord - float2(0.5f, 0.5f);
    float dist = length(centered) * 2.0f; // 0(中心) 〜 1(外周)

    float glow = saturate(1.0f - dist);
    glow = pow(glow, 1.8f); // コアを明るく、外側をなだらかに減光

    if (Material.TextureEnable)
    {
        outDiffuse = g_Texture.Sample(g_SamplerState, In.TexCoord);
        outDiffuse *= In.Diffuse;
    }
    else
    {
        outDiffuse = In.Diffuse;
    }

    // グローを反映してコア以外を減光・アルファも滲ませる
    outDiffuse.rgb *= glow;
    outDiffuse.a *= glow;

    // 中心付近を白く発光させ、火花らしい眩しさを追加
    float core = saturate(1.0f - dist * 2.0f);
    outDiffuse.rgb += core * core * 0.4f;
}
