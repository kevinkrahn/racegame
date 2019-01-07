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
};

#if defined VERT

layout(location = 0) in vec3 attrPosition;
layout(location = 1) in vec3 attrNormal;
layout(location = 2) in vec4 attrColor;
layout(location = 3) in vec2 attrTexCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;
layout(location = 3) out vec3 outWorldPosition;

void main()
{
    outColor = attrColor;
    outNormal = attrNormal;
    outTexCoord = attrTexCoord;
    outWorldPosition = attrPosition;
}

#elif defined FRAG

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

void main()
{
    outColor = inColor;
    float light = max(dot(normalize(inNormal), sunDirection), 0.1);
    outColor.rgb *= light;
}

#elif defined GEOM

layout(triangles, invocations = VIEWPORT_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec4 inColor[];
layout(location = 1) in vec3 inNormal[];
layout(location = 2) in vec2 inTexCoord[];
layout(location = 3) in vec3 inWorldPosition[];

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;

void main()
{
    for (uint i=0; i<3; ++i)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = cameraViewProjection[gl_InvocationID] * vec4(inWorldPosition[i], 1.0);
        outColor = inColor[i];
        outNormal = inNormal[i];
        outTexCoord = inTexCoord[i];
        EmitVertex();
    }

    EndPrimitive();
}

#endif
