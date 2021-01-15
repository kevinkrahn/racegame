layout(binding = 1) uniform samplerCube cubemapSampler;
layout(binding = 2) uniform sampler2DShadow shadowDepthSampler;
layout(binding = 3) uniform sampler2D cloudShadowTexture;
layout(binding = 4) uniform sampler2D ssaoTexture;

float getSunShadow(sampler2DShadow tex, vec3 shadowCoord)
{
#if SHADOWS_ENABLED
#if SHADOW_FILTERING_QUALITY == 0
    const uint count = 1;
    vec2 offsets[count] = vec2[](vec2(0.0, 0.0));
#elif SHADOW_FILTERING_QUALITY == 1
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
    //const float offsetDivider = 800.0;
    const float offsetDivider = 1200.0;

    float shadow = 0.0;
    for (uint i=0; i<count; ++i)
    {
        vec3 texCoord;
        texCoord.xy = shadowCoord.xy + offsets[i] / offsetDivider;
        texCoord.z = shadowCoord.z;
        shadow += texture(tex, texCoord) / count;
    }

    return shadow;
#else
    return 1.0;
#endif
}

float getFresnel(vec3 normal, vec3 dirToCam, float bias, float scale, float power)
{
    return max(bias + scale * pow(1.0 + dot(-dirToCam, normal), power), 0.0);
}

float luminance(vec3 color)
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

// TODO: investigate other BRDFs (diffuse-oren-nayar)
vec4 lighting(vec4 color, vec3 normal, vec3 shadowCoord, vec3 worldPosition,
        float specularPower, float specularStrength, vec3 specularColor,
        float fresnelBias, float fresnelScale, float fresnelPower, vec3 emit,
        float reflectionStrength, float reflectionLod, float reflectionBias)
{
    vec3 lightOut = emit;
    vec3 toCamera = cameraPosition - worldPosition;
    float distanceToCamera = length(toCamera);
    vec3 camDir = toCamera / distanceToCamera;

    float shadow = getSunShadow(shadowDepthSampler, shadowCoord);
    float cloudShadow = 1.0 - (texture(cloudShadowTexture,
            vec2(worldPosition.xy * 0.002) + vec2(time * 0.02, 0.0)).r * cloudShadowStrength);
    shadow *= cloudShadow;

    // directional light
    vec3 directSunLight = max(dot(normal, sunDirection) * shadow, 0.0) * color.rgb * sunColor;
    vec3 halfDir = normalize(sunDirection + camDir);
    vec3 specularSunLight = pow(max(dot(normal, halfDir), 0.0), specularPower)
                                * specularStrength * shadow * specularColor * sunColor;
    lightOut += directSunLight;
    lightOut += specularSunLight;

    // point lights
#if POINT_LIGHTS_ENABLED
    uint partitionX = uint((float(gl_FragCoord.x) * invResolution.x) * LIGHT_SPLITS);
    uint partitionY = uint((1.f - (float(gl_FragCoord.y) * invResolution.y)) * LIGHT_SPLITS);
    uint pointLightCount = lightPartitions[partitionX][partitionY].pointLightCount;
    for (uint i=0; i<pointLightCount; ++i)
    {
        PointLight light = lightPartitions[partitionX][partitionY].pointLights[i];
        vec3 lightDiff = light.position - worldPosition;
        float distance = length(lightDiff);
        vec3 lightDirection = lightDiff / distance;

#if 0
        float falloff = pow(smoothstep(light.radius, 0, distance), light.falloff);
#else
        // NOTE: Hardcoded light falloff; uncomment line above to use uniform value
        float falloff = pow(smoothstep(light.radius, 0, distance), 2.f);
#endif

        vec3 directLight = max(dot(normal, lightDirection), 0.0) * falloff * color.rgb * light.color;
        vec3 halfDir = normalize(lightDirection + camDir);
        vec3 specularLight = pow(max(dot(normal, halfDir), 0.0), specularPower)
            * specularStrength * falloff * specularColor * light.color;
        lightOut += directLight;
        lightOut += specularLight;
    }
#endif

    // fresnel
    float fresnel = getFresnel(normal, camDir, fresnelBias, fresnelScale, fresnelPower);
    lightOut += fresnel * shadow * luminance(sunColor);

    // ssao
    float ssaoAmount = 1.0;
#if SSAO_QUALITY > 0
#ifndef NO_SSAO
    ssaoAmount = min(
        texelFetch(ssaoTexture, ivec2(gl_FragCoord.xy), 0).r + luminance(lightOut) * 0.7 + 0.05, 1.0);
    lightOut *= ssaoAmount;
#endif
#endif

    // ambient light
    float reflectionAmount = reflectionStrength *
        clamp(shadow * getFresnel(normal, camDir, reflectionBias, 1.0, 1.3), 0.1, 1.0);
    float ambientAmount = 1.0 - reflectionAmount;
#if 1
    // give the ambient light a bit of directionality
    const vec3 ambientDirection = normalize(vec3(0.3, 0.1, 0.8));
    vec3 ambientLight = max(dot(normal, ambientDirection), 0.65)
        * ambientAmount * ssaoAmount * ambientStrength * color.rgb * ambientColor;
#else
    vec3 ambientLight = ambientAmount * ssaoAmount * ambientStrength * color.rgb * ambientColor;
#endif
    lightOut += ambientLight;

    // environment reflections
    vec3 I = normalize(worldPosition - cameraPosition);
    vec3 R = reflect(I, normal);
    lightOut += textureLod(cubemapSampler, R, reflectionLod).rgb
        * reflectionAmount * ssaoAmount * ambientStrength;

    // fog
#if FOG_ENABLED
    // TODO: should fog be multiplied by sun color and ambient color?
    const float LOG2 = -1.442695;
    float d = fogDensity * max(distanceToCamera - fogBeginDistance, 0.0);
    float fogIntensity = 1.0 - clamp(exp2(d * d * LOG2), 0.0, 1.0);
    lightOut = mix(lightOut, fogColor, fogIntensity);
#endif

    //return vec4(vec3(texelFetch(ssaoTexture, ivec2(gl_FragCoord.xy), 0).r), 1.0);
    return vec4(lightOut, color.a);
}
