vec3 trilinearRgb(sampler2D colorSampler, vec3 inP, vec3 blend)
{
    vec2 uvX = inP.zy;
    vec2 uvY = inP.xz;
    vec2 uvZ = inP.xy;

    vec3 xColor = texture(colorSampler, uvX).rgb;
    vec3 yColor = texture(colorSampler, uvY).rgb;
    vec3 zColor = texture(colorSampler, uvZ).rgb;
    vec3 color = xColor * blend.x + yColor * blend.y + zColor * blend.z;

    return color;
}

float trilinearR(sampler2D colorSampler, vec3 inP, vec3 blend)
{
    vec2 uvX = inP.zy;
    vec2 uvY = inP.xz;
    vec2 uvZ = inP.xy;

    float xColor = texture(colorSampler, uvX).r;
    float yColor = texture(colorSampler, uvY).r;
    float zColor = texture(colorSampler, uvZ).r;
    float color = xColor * blend.x + yColor * blend.y + zColor * blend.z;

    return color;
}

vec3 trilinearNormal(sampler2D normalSampler, vec3 inN, vec3 inP, vec3 blend)
{
    vec2 uvX = inP.zy;
    vec2 uvY = inP.xz;
    vec2 uvZ = inP.xy;

#if 0
    // TODO: compare visual difference when normalization is enabled
    vec3 xNormal = normalize(texture(normalSampler, uvX).rgb * 2.0 - 1.0);
    vec3 yNormal = normalize(texture(normalSampler, uvY).rgb * 2.0 - 1.0);
    vec3 zNormal = normalize(texture(normalSampler, uvZ).rgb * 2.0 - 1.0);
#else
    vec3 xNormal = texture(normalSampler, uvX).rgb * 2.0 - 1.0;
    vec3 yNormal = texture(normalSampler, uvY).rgb * 2.0 - 1.0;
    vec3 zNormal = texture(normalSampler, uvZ).rgb * 2.0 - 1.0;
#endif
    xNormal = vec3(xNormal.xy + inN.zy, abs(xNormal.z) * inN.x);
    yNormal = vec3(yNormal.xy + inN.xz, abs(yNormal.z) * inN.y);
    zNormal = vec3(zNormal.xy + inN.xy, abs(zNormal.z) * inN.z);
    vec3 worldNormal = normalize(
            xNormal.zyx * blend.x +
            yNormal.xzy * blend.y +
            zNormal.xyz * blend.z);
    return worldNormal;
}

vec3 trilinearBlend(vec3 n)
{
    vec3 blend = abs(n);
    blend = max(blend - 0.2, 0.0);
    blend /= dot(blend, vec3(1));
    return blend;
}
