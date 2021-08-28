#include "InputManager.h"
#include <windows.h>

namespace awesome {

	void InputManager::OnWindowMessage(unsigned int uMsg, unsigned int wParam) {
		bool isDown = (uMsg == WM_KEYDOWN);
		if (wParam == 'W')
			Keys[MoveCameraForward] = isDown;
		else if (wParam == 'S')
			Keys[MoveCameraBack] = isDown;
		else if (wParam == 'A')
			Keys[MoveCameraLeft] = isDown;
		else if (wParam == 'D')
			Keys[MoveCameraRight] = isDown;
		else if (wParam == 'Q')
			Keys[RaiseCamera] = isDown;
		else if (wParam == 'E')
			Keys[LowerCamera] = isDown;
		else if (wParam == VK_UP)
			Keys[LookCameraUp] = isDown;
		else if (wParam == VK_DOWN)
			Keys[LookCameraDown] = isDown;
		else if (wParam == VK_LEFT)
			Keys[TurnCameraLeft] = isDown;
		else if (wParam == VK_RIGHT)
			Keys[TurnCameraRight] = isDown;
	}

	void InputManager::SetKeyDown(InputAction a, bool value) {
		Keys[a] = value; 
	}

	void InputManager::ClearKeys() {
		ZeroMemory(Keys, sizeof(Keys));
	}

	bool InputManager::IsKeyDown(InputAction a) const { 
		return Keys[a]; 
	}

	float InputManager::GetPlayerSpeed(unsigned long long deltaMs) const {
		return static_cast<float>(PlayerSpeed / 1000 * deltaMs);
	}

} // awesome
