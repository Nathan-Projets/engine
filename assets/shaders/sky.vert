#version 460 core

layout(location = 0) in vec3 aPos;

out vec3 TexCoords;

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
    vec3 sampleDirection = mat3(uModel) * aPos;
    TexCoords = sampleDirection;

    // remove translation from view matrix to keep skybox centered on camera
    mat4 viewNoTranslation = uView;
    viewNoTranslation[3] = vec4(0.0, 0.0, 0.0, 1.0);
    
    vec4 pos = uProjection * viewNoTranslation * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
