struct PointLight
{
    vec3 position;
    float radius;
    vec3 color;
    float falloff;
};

struct LightPartition
{
    PointLight pointLights[MAX_POINT_LIGHTS];
    uint pointLightCount;
};

layout (std140, binding = 0) uniform WorldInfo
{
    mat4 orthoProjection;
    vec3 sunDirection;
    float time;
    vec3 sunColor;
    float pad1;
    mat4 cameraViewProjection;
    mat4 cameraProjection;
    mat4 cameraView;
    vec3 cameraPosition;
    mat4 shadowViewProjectionBias;
    LightPartition lightPartitions[LIGHT_SPLITS][LIGHT_SPLITS];
    vec3 fogColor;
    float fogDensity;
    vec2 invResolution;
    float fogBeginDistance;
    float cloudShadowStrength;
    vec3 ambientColor;
    float ambientStrength;
};

