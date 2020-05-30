layout(binding = 1) uniform samplerCube cubemapSampler;
layout(binding = 2) uniform sampler2DShadow shadowDepthSampler;
layout(binding = 3) uniform sampler2D cloudShadowTexture;
layout(binding = 4) uniform sampler2D ssaoTexture;

float getShadow(sampler2DShadow tex, vec3 shadowCoord)
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
        vec3 texCoord;
        texCoord.xy = shadowCoord.xy + offsets[i] / 800.0;
        texCoord.z = shadowCoord.z;
        shadow += texture(tex, texCoord) / count;
    }

    return shadow;
}

float getFresnel(vec3 normal, vec3 worldPosition, float bias, float scale, float power)
{
    return max(bias + scale * pow(1.0 + dot(normalize(worldPosition - cameraPosition), normal), power), 0.0);
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

    // directional light shadow
#if SHADOWS_ENABLED
    float shadow = getShadow(shadowDepthSampler, shadowCoord);
#else
    float shadow = 1.0;
#endif
    float cloudShadow = min(texture(cloudShadowTexture,
            vec2(worldPosition.xy * 0.002) + vec2(time * 0.02, 0.0)).r, 1.0);
    shadow *= cloudShadow;

    // directional light
    const float sunPower = 1.0;
    const vec3 ambientDirection = normalize(vec3(0.3, 0.1, 0.8));
    float directLight = max(dot(normal, sunDirection) * sunPower * shadow, 0.055)
        + max(dot(normal, ambientDirection) * 0.07, 0.0);
    vec3 halfDir = normalize(sunDirection + camDir);
    vec3 specularLight = specularColor * (pow(max(dot(normal, halfDir), 0.0), specularPower) * specularStrength) * sunPower;

    lightOut += color.rgb * directLight;
    lightOut += specularLight * shadow;

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

        //float falloff = pow(smoothstep(light.radius, 0, distance), light.falloff);
        // NOTE: Hardcoded light falloff; uncomment line above to use uniform value
        float falloff = pow(smoothstep(light.radius, 0, distance), 2.f);
        float directLight = max(dot(normal, lightDirection), 0.0) * falloff;
        vec3 halfDir = normalize(lightDirection + camDir);
        vec3 specularLight = specularColor * light.color * (pow(max(dot(normal, halfDir), 0.0), specularPower) * specularStrength) * falloff;

        lightOut += color.rgb * light.color * directLight;
        lightOut += specularLight;
    }
#endif

    // fresnel
    float fresnel = getFresnel(normal, worldPosition, fresnelBias, fresnelScale, fresnelPower);
    lightOut += fresnel * max(shadow, 0.25);

    // ssao
#if SSAO_ENABLED
#ifndef NO_SSAO
    float ssaoAmount = texelFetch(ssaoTexture, ivec2(gl_FragCoord.xy), 0).r;
    lightOut *= clamp(ssaoAmount + directLight * 0.6, 0.0, 1.0);
#endif
#endif

    // environment reflections
    vec3 I = normalize(worldPosition - cameraPosition);
    vec3 R = reflect(I, normal);
    lightOut += textureLod(cubemapSampler, R, reflectionLod).rgb
        * reflectionStrength
        * clamp(shadow * getFresnel(normal, worldPosition, reflectionBias, 1.0, 1.3), 0.1, 1.0);

    // fog
#if FOG_ENABLED
    const vec3 fogColor = vec3(0.5, 0.6, 1);
    const float density = 0.0015;
    const float LOG2 = -1.442695;
    //float fogIntensity = 1.0 - clamp(exp(-density * (distanceToCamera - 90)), 0.0, 1.0);
    //float fogIntensity = 1.0 - clamp(exp(-density * distanceToCamera), 0.0, 1.0);
    float d = density * distanceToCamera;
    float fogIntensity = 1.0 - clamp(exp2(d * d * LOG2), 0.0, 1.0);
    lightOut = mix(lightOut, fogColor, fogIntensity);
#endif

    return vec4(lightOut, color.a);
}
