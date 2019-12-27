#if defined VERT

layout(location = 0) out vec2 outTexCoord;

void main()
{
    gl_Position = vec4(-1.0 + float((gl_VertexID & 1) << 2), -1.0 + float((gl_VertexID & 2) << 1), 0.0, 1.0);
    outTexCoord = vec2((gl_Position.x + 1.0) * 0.5, (gl_Position.y + 1.0) * 0.5);
}

#elif defined FRAG

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D tex;

layout(location = 2) uniform vec2 invResolution;

vec4 gaussianBlur5(sampler2D image, vec2 uv, vec2 dir)
{
    vec4 color = vec4(0.0);
    const vec2 off1 = vec2(1.33333333333333333) * dir;
    color += texture(image, uv) * 0.29411764705882354;
    color += texture(image, uv + off1) * 0.35294117647058826;
    color += texture(image, uv - off1) * 0.35294117647058826;
    return color;
}

vec4 gaussianBlur9(sampler2D image, vec2 uv, vec2 dir)
{
    vec4 color = vec4(0.0);
    const vec2 off1 = vec2(1.3846153846) * dir;
    const vec2 off2 = vec2(3.2307692308) * dir;
    color += texture(image, uv) * 0.2270270270;
    color += texture(image, uv + off1) * 0.3162162162;
    color += texture(image, uv - off1) * 0.3162162162;
    color += texture(image, uv + off2) * 0.0702702703;
    color += texture(image, uv - off2) * 0.0702702703;
    return color;
}

vec4 gaussianBlur13(sampler2D image, vec2 uv, vec2 dir) {
    vec4 color = vec4(0.0);
    const vec2 off1 = vec2(1.411764705882353) * dir;
    const vec2 off2 = vec2(3.2941176470588234) * dir;
    const vec2 off3 = vec2(5.176470588235294) * dir;
    color += texture(image, uv) * 0.1964825501511404;
    color += texture(image, uv + off1) * 0.2969069646728344;
    color += texture(image, uv - off1) * 0.2969069646728344;
    color += texture(image, uv + off2) * 0.09447039785044732;
    color += texture(image, uv - off2) * 0.09447039785044732;
    color += texture(image, uv + off3) * 0.010381362401148057;
    color += texture(image, uv - off3) * 0.010381362401148057;
    return color;
}

#ifdef VBLUR
const uint BLUR_DIRECTION = 1;
#endif

#ifdef HBLUR
const uint BLUR_DIRECTION = 0;
#endif

void main()
{
    const vec2 dir = vec2(BLUR_DIRECTION, BLUR_DIRECTION ^ 1) * invResolution;
    outColor = gaussianBlur9(tex, inTexCoord, dir);
}
#endif
