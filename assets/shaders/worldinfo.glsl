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
    vec4 projInfo[MAX_VIEWPORTS];
    vec4 projScale;
};

