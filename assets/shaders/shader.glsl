layout (std140, binding = 0) uniform WorldInfo
{
    mat4 orthoProjection;
    vec3 sunDirection;
    float time;
    vec3 sunColor;
    mat4 cameras[MAX_VIEWPORTS];
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
    //gl_Position = worldViewProjectionMatrix * vec4(attrPosition, 1.0);
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

    float light = max(dot(normalize(inNormal), sunDirection), 0.1);
    outColor.rgb *= light;
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

void main()
{
    gl_Layer = gl_InvocationID;
    gl_Position = cameras[gl_InvocationID] * vec4(inWorldPosition[0], 1.0);
    outColor = inColor[0];
    outNormal = inNormal[0];
    outTexCoord = inTexCoord[0];
    outWorldPosition = inWorldPosition[0];
    EmitVertex();

    gl_Layer = gl_InvocationID;
    gl_Position = cameras[gl_InvocationID] * vec4(inWorldPosition[1], 1.0);
    outColor = inColor[1];
    outNormal = inNormal[1];
    outTexCoord = inTexCoord[1];
    outWorldPosition = inWorldPosition[1];
    EmitVertex();

    gl_Layer = gl_InvocationID;
    gl_Position = cameras[gl_InvocationID] * vec4(inWorldPosition[2], 1.0);
    outColor = inColor[2];
    outNormal = inNormal[2];
    outTexCoord = inTexCoord[2];
    outWorldPosition = inWorldPosition[2];
    EmitVertex();
    EndPrimitive();
}

#endif
