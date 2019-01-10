#if defined VERT

void main()
{
    gl_Position = vec4(-1.0 + float((gl_VertexID & 1) << 2), -1.0 + float((gl_VertexID & 2) << 1), 0.0, 1.0);
}

#elif defined FRAG

layout(location = 0) out vec4 outColor;

layout(location = 0) uniform vec4 projInfo[VIEWPORT_COUNT];
layout(location = 10) uniform float projScale[VIEWPORT_COUNT];

layout(binding = 3) uniform sampler2DArray depthTexture;

vec3 reconstructCSPosition(vec2 S, float z)
{
    return vec3((S.xy * projInfo[gl_Layer].xy + projInfo[gl_Layer].zw) * z, z);
}

vec3 reconstructCSFaceNormal(vec3 C)
{
    return normalize(cross(dFdy(C), dFdx(C)));
}

vec3 reconstructNonUnitCSFaceNormal(vec3 C)
{
    return cross(dFdy(C), dFdx(C));
}

#define NUM_SAMPLES 15
#define LOG_MAX_OFFSET 3
#define MAX_MIP_LEVEL 4
#define NUM_SPIRAL_TURNS 7
#define FAR_PLANE_Z -200.0
#define TWO_PI 6.283185307179586

const float intensity = 1.0;
const float radius = 12.0;
const float bias = 0.01;
const float intensityDivR6 = intensity / pow(radius, 6.0);
const float radius2 = radius * radius;

vec2 tapLocation(int sampleNumber, float spinAngle, out float ssR)
{
    float alpha = float(sampleNumber + 0.5) * (1.0 / NUM_SAMPLES);
    float angle = alpha * (NUM_SPIRAL_TURNS * 6.28) + spinAngle;
    ssR = alpha;
    return vec2(cos(angle), sin(angle));
}

float CSZToKey(float z)
{
    return clamp(z * (1.0 / FAR_PLANE_Z), 0.0, 1.0);
}

void packKey(float key, out vec2 p)
{
    float temp = floor(key * 256.0);
    p.x = temp * (1.0 / 256.0);
    p.y = key * 256.0 - temp;
}

vec3 getPosition(ivec2 ssP)
{
    vec3 P;
    P.z = texelFetch(depthTexture, ivec3(ssP, gl_Layer), 0).r;
    P = reconstructCSPosition(vec2(ssP) + vec2(0.5), P.z);
    return P;
}

vec3 getOffsetPosition(ivec2 ssC, vec2 unitOffset, float ssR)
{
    int mipLevel = clamp(findMSB(int(ssR)) - LOG_MAX_OFFSET, 0, MAX_MIP_LEVEL);
    ivec2 ssP = ivec2(ssR * unitOffset) + ssC;
    vec3 P;
    ivec2 mipP = clamp(ssP >> mipLevel, ivec2(0), textureSize(depthTexture, mipLevel).xy - ivec2(1));
    P.z = texelFetch(depthTexture, ivec3(mipP, gl_Layer), mipLevel).r;
    P = reconstructCSPosition(vec2(ssP) + vec2(0.5), P.z);
    return P;
}

float sampleAO(in ivec2 ssC, in vec3 C, in vec3 n_C, in float ssDiskRadius, in int tapIndex, in float randomPatternRotationAngle) {
    float ssR;
    vec2 unitOffset = tapLocation(tapIndex, randomPatternRotationAngle, ssR);
    ssR *= ssDiskRadius;

    vec3 Q = getOffsetPosition(ssC, unitOffset, ssR);

    vec3 v = Q - C;

    float vv = dot(v, v);
    float vn = dot(v, n_C);

    const float epsilon = 0.01;
    float f = max(radius2 - vv, 0.0); return f * f * f * max((vn - bias) / (epsilon + vv), 0.0);
}

void main() {
    ivec2 ssC = ivec2(gl_FragCoord.xy);

    vec3 C = getPosition(ssC);

    packKey(CSZToKey(C.z), outColor.gb);

    //float randomPatternRotationAngle = (3 * ssC.x ^ ssC.y + ssC.x * ssC.y) * 10;
    float randomPatternRotationAngle = mod((3 * ssC.x ^ ssC.y + ssC.x * ssC.y) * 10, TWO_PI);
    vec3 n_C = reconstructCSFaceNormal(C);
    float diskRadius = -projScale[gl_Layer] * radius / C.z;

    float sum = 0.0;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        sum += sampleAO(ssC, C, n_C, diskRadius, i, randomPatternRotationAngle);
    }

    float A = max(0.0, 1.0 - sum * intensityDivR6 * (5.0 / NUM_SAMPLES));

    if (abs(dFdx(C.z)) < 0.02) {
        A -= dFdx(A) * ((ssC.x & 1) - 0.5);
    }
    if (abs(dFdy(C.z)) < 0.02) {
        A -= dFdy(A) * ((ssC.y & 1) - 0.5);
    }

    outColor.r = A;
    outColor.a = 1.0;
}

#elif defined GEOM

layout(triangles, invocations = VIEWPORT_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

void main()
{
    for (uint i=0; i<3; ++i)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }

    EndPrimitive();
}

#endif
