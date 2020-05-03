#if defined VERT

#include "worldinfo.glsl"

layout(location = 0) in vec3 attrPosition;

layout(location = 0) uniform mat4 worldMatrix;

void main()
{
    gl_Position = (cameraViewProjection * worldMatrix) * vec4(attrPosition, 1.0);
}

#elif defined FRAG

layout(location = 1) uniform uint highlightID;

layout(location = 0) out uint outID;

void main()
{
    outID = highlightID;
}

#endif
