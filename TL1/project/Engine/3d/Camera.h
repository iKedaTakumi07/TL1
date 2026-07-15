#pragma once
#include "../base/Math.h"

class WinApp;
class Camera {
public:
    Camera();

    // 更新
    void Update();

    // Setter
    void SetRotate(const Vector3& rotate) { transform.rotate = rotate; }
    void SetTranslate(const Vector3& translate) { transform.translate = translate; }
    void SetFovY(const float fovY) { horizontalFov_ = fovY; }
    void SetAspectRatio(const float aspectRatio) { aspectRatio_ = aspectRatio; };
    void SetNearClip(const float nearClip) { nearClip_ = nearClip; }
    void SetFarClip(const float farClip) { farClip_ = farClip; }

    // Getter
    const Matrix4x4& GetWorldMatrix() const { return worldMatrix; }
    const Matrix4x4& GetViewMatrix() const { return viewMatrix; }
    const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix; }
    const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix; }
    const Matrix4x4& GetProjectionInverse() const { return projectionInverseMatrix; }
    Vector3 GetRotate() const { return transform.rotate; }
    Vector3 GetTranslate() const { return transform.translate; }

private:
    Transform transform;
    Matrix4x4 worldMatrix;
    Matrix4x4 viewMatrix;

    Matrix4x4 projectionMatrix;
    float horizontalFov_;
    float aspectRatio_;
    float nearClip_;
    float farClip_;

    Matrix4x4 viewProjectionMatrix;
    Matrix4x4 projectionInverseMatrix;
};
