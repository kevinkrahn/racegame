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

vec3 _textureNormal(sampler2D normalSampler, vec2 texCoord)
{
#if 0
    vec3 normal = texture(normalSampler, texCoord).rgb * 2.0 - 1.0;
#else
    vec3 normal;
    // TODO: Is this right?
    //normal.xy = texture(normalSampler, texCoord).rg * (255.0 / 128.0) - 1.0;
    normal.xy = texture(normalSampler, texCoord).rg * 2.0 - 1.0;
    normal.z = sqrt(1.0 - normal.x*normal.x - normal.y*normal.y);
#endif
    return normal;
}

vec3 trilinearNormal(sampler2D normalSampler, vec3 inN, vec3 inP, vec3 blend)
{
    vec2 uvX = inP.zy;
    vec2 uvY = inP.xz;
    vec2 uvZ = inP.xy;

    vec3 xNormal = _textureNormal(normalSampler, uvX);
    vec3 yNormal = _textureNormal(normalSampler, uvY);
    vec3 zNormal = _textureNormal(normalSampler, uvZ);
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
