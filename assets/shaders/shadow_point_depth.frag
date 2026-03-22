#version 460 core

in vec4 FragPos;

uniform vec3 uLightPos;
uniform float uFarPlane;

void main()
{
    float lightDistance = length(FragPos.xyz - uLightPos);
    gl_FragDepth = lightDistance / uFarPlane;
}
