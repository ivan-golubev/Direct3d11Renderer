#pragma once

namespace awesome {

	enum InputAction {
		RaiseCamera,
		LowerCamera,
		MoveCameraLeft,
		MoveCameraRight,
		MoveCameraForward,
		MoveCameraBack,
		TurnCameraLeft,
		TurnCameraRight,
		LookCameraUp,
		LookCameraDown,
		Count
	};

	class InputManager {
	public:
		void OnWindowMessage(unsigned int uMsg, unsigned int wParam);
		void SetKeyDown(InputAction a, bool value);
		void ClearKeys();
		bool IsKeyDown(InputAction a) const;
		float GetPlayerSpeed(unsigned long long deltaMs) const;
	private:
		bool Keys[InputAction::Count];
		float PlayerSpeed{ 1.5f };
	};

} // namespace awesome