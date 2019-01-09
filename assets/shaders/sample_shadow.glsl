float getShadow(sampler2DArrayShadow tex, vec3 shadowCoord)
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
        texCoord.xy = shadowCoord.xy + offsets[i] / 800.0;
        texCoord.w = shadowCoord.z;
        texCoord.z = gl_Layer;
        shadow += texture(tex, texCoord) / count;
    }

    return shadow;
}