#include "../3d/Camera.h"
#include "../base/WinApp.h"

Camera::Camera()
    : transform({ { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } })
    , horizontalFov_(0.45f)
    , aspectRatio_(float(WinApp::KClientWidth) / float(WinApp::KClientHeight))
    , nearClip_(0.1f)
    , farClip_(100.0f)
    , worldMatrix(MakeAffineMatrix(transform.scale, transform.rotate, transform.translate))
    , viewMatrix(Inverse(worldMatrix))
    , projectionMatrix(MakePrespectiveFovMatrix(horizontalFov_, aspectRatio_, nearClip_, farClip_))
    , projectionInverseMatrix(Inverse(projectionMatrix))
    , viewProjectionMatrix(Multiply(viewMatrix, projectionMatrix))
{
}

void Camera::Update()
{
    worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
    viewMatrix = Inverse(worldMatrix);

    projectionMatrix = MakePrespectiveFovMatrix(horizontalFov_, aspectRatio_, nearClip_, farClip_);
    projectionInverseMatrix = Inverse(projectionMatrix);

    viewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);
}