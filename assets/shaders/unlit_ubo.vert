#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

layout(std140, binding = 1) uniform CameraDataBlock {
    mat4 uView;
    mat4 uProjection;
    mat4 uViewProjection;
    mat4 uInvViewProjection;
    vec3 uCameraPosition;
    float uNearPlane;
    vec2 _cameraPad0;
    float uFarPlane;
};

layout(std140, binding = 2) uniform ObjectDataBlock {
    mat4 uModel;
    mat4 uNormalMatrix;
    uint uObjectId;
    uint uMaterialIndex;
    vec2 _objectPad0;
};

void main() {
    TexCoords = aTexCoords;
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}
