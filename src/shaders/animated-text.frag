#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D source;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec4 color1;
    vec4 color2;
};

void main()
{
    vec4 mask = texture(source, qt_TexCoord0);
    float t = clamp(qt_TexCoord0.x * 0.5 + qt_TexCoord0.y * 0.5, 0.0, 1.0);
    vec3 grad = mix(color1.rgb, color2.rgb, t);
    fragColor = vec4(grad * mask.a, mask.a) * qt_Opacity;
}
