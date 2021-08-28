#pragma once
#include "3DMaths.h"

namespace awesome {
	class InputManager;

	class Camera
	{
	public:
		Camera(InputManager*);
		void UpdateCamera(unsigned long long deltaTimeMs);
		void UpdatePerspectiveMatrix(float windowAspectRatio);
		float4x4& GetViewMatrix();
		float4x4& GetPerspectiveMatrix();

	private:
		InputManager* inputManager;

		float3 cameraPos = { 0, 0, 2 };
		float3 cameraFwd = { 0, 0, -1 };
		float cameraPitch{ 0.f };
		float cameraYaw{ 0.f };

		float4x4 perspectiveMatrix{};
		float4x4 viewMatrix{};

		const float CAM_MOVE_SPEED{ 5.f }; // in metres per second
		const float CAM_TURN_SPEED{ static_cast<float>(M_PI) }; // in radians per second
	};
}
