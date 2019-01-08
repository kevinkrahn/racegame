layout (std140, binding = 0) uniform WorldInfo
{
    mat4 orthoProjection;
    vec3 sunDirection;
    float time;
    vec3 sunColor;
    mat4 cameraViewProjection[MAX_VIEWPORTS];
    mat4 cameraProjection[MAX_VIEWPORTS];
    mat4 cameraView[MAX_VIEWPORTS];
    vec3 cameraPosition[MAX_VIEWPORTS];
    mat4 shadowViewProjection[MAX_VIEWPORTS];
    mat4 shadowViewProjectionBias[MAX_VIEWPORTS];
};

#if defined VERT

layout(location = 0) in vec3 attrPosition;
layout(location = 1) in vec3 attrNormal;
layout(location = 2) in vec3 attrColor;
layout(location = 3) in vec2 attrTexCoord;

layout(location = 0) uniform mat4 worldMatrix;
layout(location = 1) uniform mat3 normalMatrix;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;
layout(location = 3) out vec3 outWorldPosition;

void main()
{
    outColor = attrColor;
    outNormal = normalize(normalMatrix * attrNormal);
    outTexCoord = attrTexCoord;
    outWorldPosition = (worldMatrix * vec4(attrPosition, 1.0)).xyz;
}

#elif defined FRAG

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inWorldPosition;
layout(location = 4) in vec3 inShadowCoord;

layout(binding = 2) uniform sampler2DArrayShadow shadowDepthSampler;
layout(binding = 4) uniform sampler2DArray ssaoTexture;

float getShadow()
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
        texCoord.xy = inShadowCoord.xy + offsets[i] / 800.0;
        texCoord.w = inShadowCoord.z;
        texCoord.z = gl_Layer;
        shadow += texture(shadowDepthSampler, texCoord) / count;
    }

    return shadow;
}

void main()
{
    outColor = vec4(inColor, 1.0);

    float directLight = max(dot(normalize(inNormal), sunDirection) * getShadow(), 0.0);
    outColor.rgb *= max(directLight, 0.4);
    float ssaoAmount = texture(ssaoTexture, vec3(gl_FragCoord.xy / textureSize(ssaoTexture, 0).xy, gl_Layer)).r;
    outColor.rgb *= clamp(ssaoAmount + directLight * 0.5, 0.0, 1.0);
}

#elif defined GEOM

layout(triangles, invocations = VIEWPORT_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 inColor[];
layout(location = 1) in vec3 inNormal[];
layout(location = 2) in vec2 inTexCoord[];
layout(location = 3) in vec3 inWorldPosition[];

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;
layout(location = 3) out vec3 outWorldPosition;
layout(location = 4) out vec3 outShadowCoord;

void main()
{
    for (uint i=0; i<3; ++i)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = cameraViewProjection[gl_InvocationID] * vec4(inWorldPosition[i], 1.0);
        outColor = inColor[i];
        outNormal = inNormal[i];
        outTexCoord = inTexCoord[i];
        outWorldPosition = inWorldPosition[i];
        outShadowCoord = (shadowViewProjectionBias[gl_InvocationID] * vec4(inWorldPosition[i], 1.0)).xyz;
        EmitVertex();
    }

    EndPrimitive();
}

#endif
