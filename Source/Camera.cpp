#include "Camera.h"
#include "InputManager.h"

namespace awesome {

    Camera::Camera(InputManager* im): inputManager(im) {}

    void Camera::UpdateCamera(unsigned long long  deltaTimeMs) {
        float3 camFwdXZ = normalise({ cameraFwd.x, 0, cameraFwd.z });
        float3 cameraRightXZ = cross(camFwdXZ, { 0, 1, 0 });

        const float CAM_MOVE_AMOUNT = CAM_MOVE_SPEED * deltaTimeMs / 1000.0f;
        if (inputManager->IsKeyDown(MoveCameraForward))
            cameraPos += camFwdXZ * CAM_MOVE_AMOUNT;
        if (inputManager->IsKeyDown(MoveCameraBack))
            cameraPos -= camFwdXZ * CAM_MOVE_AMOUNT;
        if (inputManager->IsKeyDown(MoveCameraLeft))
            cameraPos -= cameraRightXZ * CAM_MOVE_AMOUNT;
        if (inputManager->IsKeyDown(MoveCameraRight))
            cameraPos += cameraRightXZ * CAM_MOVE_AMOUNT;
        if (inputManager->IsKeyDown(RaiseCamera))
            cameraPos.y += CAM_MOVE_AMOUNT;
        if (inputManager->IsKeyDown(LowerCamera))
            cameraPos.y -= CAM_MOVE_AMOUNT;

        const float CAM_TURN_AMOUNT = CAM_TURN_SPEED * deltaTimeMs / 1000.0f;
        if (inputManager->IsKeyDown(TurnCameraLeft))
            cameraYaw += CAM_TURN_AMOUNT;
        if (inputManager->IsKeyDown(TurnCameraRight))
            cameraYaw -= CAM_TURN_AMOUNT;
        if (inputManager->IsKeyDown(LookCameraUp))
            cameraPitch += CAM_TURN_AMOUNT;
        if (inputManager->IsKeyDown(LookCameraDown))
            cameraPitch -= CAM_TURN_AMOUNT;

        // Wrap yaw to avoid floating-point errors if we turn too far
        while (cameraYaw >= 2 * static_cast<float>(M_PI))
            cameraYaw -= 2 * static_cast<float>(M_PI);
        while (cameraYaw <= -2 * static_cast<float>(M_PI))
            cameraYaw += 2 * static_cast<float>(M_PI);

        // Clamp pitch to stop camera flipping upside down
        if (cameraPitch > degreesToRadians(85))
            cameraPitch = degreesToRadians(85);
        if (cameraPitch < -degreesToRadians(85))
            cameraPitch = -degreesToRadians(85);

        viewMatrix = translationMat(-cameraPos) * rotateYMat(-cameraYaw) * rotateXMat(-cameraPitch);
    }

    void Camera::UpdatePerspectiveMatrix(float windowAspectRatio) {
        perspectiveMatrix = makePerspectiveMat(windowAspectRatio, degreesToRadians(84), 0.1f, 1000.f);
    }

    float4x4& Camera::GetViewMatrix() { return viewMatrix; }
    float4x4& Camera::GetPerspectiveMatrix() { return perspectiveMatrix; }

}
