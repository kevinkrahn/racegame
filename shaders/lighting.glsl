layout(binding = 2) uniform sampler2DArrayShadow shadowDepthSampler;
layout(binding = 3) uniform sampler2D cloudShadowTexture;
layout(binding = 4) uniform sampler2DArray ssaoTexture;

float getShadow(sampler2DArrayShadow tex, vec3 shadowCoord)
{
#if 0
    const uint count = 4;
    vec2 offsets[count] = vec2[](
        vec2(-0.9420162, -0.39906216),
        vec2( 0.9455860, -0.76890725),
        vec2(-0.0941841, -0.92938870),
        vec2( 0.3449593,  0.29387760)
    );
#else
    const uint count = 9;
    vec2 offsets[count] = vec2[](
        vec2( 0.9558100, -0.18159000),
        vec2( 0.5014700, -0.35807000),
        vec2( 0.6960700,  0.35559000),
        vec2(-0.0036825, -0.59150000),
        vec2( 0.1593000,  0.08975000),
        vec2(-0.6503100,  0.05818900),
        vec2( 0.1191500,  0.78449000),
        vec2(-0.3429600,  0.51575000),
        vec2(-0.6038000, -0.41527000)
    );
#endif

    float shadow = 0.0;
    for (uint i=0; i<count; ++i)
    {
        vec4 texCoord;
        texCoord.xy = shadowCoord.xy + offsets[i] / 800.0;
        texCoord.w = shadowCoord.z;
        texCoord.z = gl_Layer;
        shadow += texture(tex, texCoord) / count;
    }

    return shadow;
}

float getFresnel(vec3 normal, vec3 worldPosition, float bias, float scale, float power)
{
    return max(bias + scale * pow(1.0 + dot(normalize(worldPosition - cameraPosition[gl_Layer]), normal), power), 0.0);
}

vec4 lighting(vec4 color, vec3 normal, vec3 shadowCoord, vec3 worldPosition,
        float specularPower, float specularStrength, vec3 specularColor,
        float fresnelBias, float fresnelScale, float fresnelPower, vec3 emit)
{
    const vec3 ambientDirection = normalize(vec3(0.2, 0.0, 0.8));
    float fresnel = getFresnel(normal, worldPosition, fresnelBias, fresnelScale, fresnelPower);

#if SHADOWS_ENABLED
    float shadow = getShadow(shadowDepthSampler, shadowCoord);
#else
    float shadow = 1.f;
#endif
    float cloudShadow = min(texture(cloudShadowTexture,
            vec2(worldPosition.xy * 0.002) + vec2(time * 0.02, 0.0)).r, 1.0);
    shadow *= cloudShadow;

    vec3 toCamera = cameraPosition[gl_Layer] - worldPosition;

    float sunPower = 1.0;
    float directLight = max(dot(normal, sunDirection) * sunPower * shadow, 0.0)
        + max(dot(normal, ambientDirection) * 0.12, 0.0);
    vec3 camDir = normalize(toCamera);
    vec3 halfDir = normalize(sunDirection + camDir);
    vec3 specularLight = specularColor * (pow(max(dot(normal, halfDir), 0.0), specularPower) * specularStrength) * sunPower;

    color.rgb *= max(directLight, 0.1);
    color.rgb += specularLight * shadow;
    color.rgb += fresnel * max(shadow, 0.5);

#if SSAO_ENABLED
#ifndef NO_SSAO
    float ssaoAmount = texelFetch(ssaoTexture, ivec3(gl_FragCoord.xy, gl_Layer), 0).r;
    color.rgb *= clamp(ssaoAmount + directLight * 0.5, 0.0, 1.0);
#endif
#endif

#ifndef NO_FOG
    const vec3 fogColor = vec3(0.5, 0.6, 1);
    float dist = length(toCamera);
#if 0
    const float fogStart = 90.0;
    const float fogEnd = 1200.0;
    float fogIntensity = max(dist - fogStart, 0.0) / (fogEnd - fogStart);
#else
    const float density = 0.0015;
    const float LOG2 = -1.442695;
    //float fogIntensity = 1.0 - clamp(exp(-density * (dist - 90)), 0.0, 1.0);
    //float fogIntensity = 1.0 - clamp(exp(-density * dist), 0.0, 1.0);
    float d = density * dist;
    float fogIntensity = 1.0 - clamp(exp2(d * d * LOG2), 0.0, 1.0);
#endif
    color.rgb = mix(color.rgb, fogColor, fogIntensity);
#endif

    color.rgb += emit;

    return color;
}
