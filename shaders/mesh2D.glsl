#if defined VERT

layout(location = 0) in vec3 attrPosition;
layout(location = 1) in vec3 attrNormal;
layout(location = 4) in vec3 attrColor;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outPosition;

layout(location = 0) uniform mat4 viewProjection;
layout(location = 1) uniform mat4 worldTransform;

void main()
{
    outColor = attrColor;
    outNormal = attrNormal;
    outPosition = (worldTransform * vec4(attrPosition, 1.0)).xyz;
    gl_Position = viewProjection * vec4(outPosition, 1.0);
}

#elif defined FRAG

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inPosition;

layout(location = 0) out vec4 outColor;

layout(location = 2) uniform vec3 color;
layout(location = 3) uniform int overwriteColor;
layout(location = 4) uniform int shadeMode;
layout(location = 5) uniform vec3 camPosition;

void main()
{
    //outColor = vec4(vec3(1.0 - gl_FragCoord.z), 1.0);
    outColor = overwriteColor > 0 ? vec4(color, 1.0) : vec4(color * inColor, 1.0);
    if (shadeMode == 1)
    {
        outColor.rgb *= pow(inNormal.z, 7.0);
    }
    else if (shadeMode == 2)
    {
        //outColor.rgb *= dot(inNormal, normalize(vec3(1.0))) * 2.f;
        //outColor.rgb *= clamp(pow(1.0 + dot(inNormal, normalize(vec3(-1.0))), 1.f), 0.0, 1.0);
        float bias = 0.0;
        float scale = 3.0;
        float power = 3.0;
        outColor.rgb *= (1.0 - max(bias + scale * pow(1.0 + dot(normalize(inPosition - camPosition), inNormal), power), 0.0));
    }
}

#endif
