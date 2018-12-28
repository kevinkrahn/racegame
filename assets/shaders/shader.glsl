layout (std140, binding = 0) uniform WorldInfo
{
    vec3 sunDirection;
    float time;
    vec3 sunColor;
};

#if defined VERT
layout (location = 0) in vec3 attrPosition;
layout (location = 1) in vec3 attrNormal;
layout (location = 2) in vec3 attrColor;
layout (location = 3) in vec2 attrTexCoord;

layout (location = 0) uniform mat4 worldMatrix;
layout (location = 1) uniform mat4 worldViewProjectionMatrix;
layout (location = 2) uniform mat3 normalMatrix;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;
layout(location = 3) out vec3 outWorldPosition;

void main()
{
    gl_Position = worldViewProjectionMatrix * vec4(attrPosition, 1.0);
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

void main()
{
    outColor = vec4(inColor, 1.0);
    //outColor.a = (sin(time + inWorldPosition.x + inWorldPosition.y + inWorldPosition.z) + 1.0) * 0.5;

    float light = max(dot(normalize(inNormal), sunDirection), 0.1);
    outColor.rgb *= light;
}

#elif defined GEOM

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

void main()
{
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
    EndPrimitive();
}

#endif
