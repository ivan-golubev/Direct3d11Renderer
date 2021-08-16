#include "InputManager.h"
#include <windows.h>

namespace awesome {

	void InputManager::OnWindowMessage(unsigned int uMsg, unsigned int wParam) {
		bool isDown = (uMsg == WM_KEYDOWN);
		if (wParam == 'W' || wParam == VK_UP)
			Keys[MoveUp] = isDown;
		if (wParam == 'S' || wParam == VK_DOWN)
			Keys[MoveDown] = isDown;
		if (wParam == 'A' || wParam == VK_LEFT)
			Keys[MoveLeft] = isDown;
		if (wParam == 'D' || wParam == VK_RIGHT)
			Keys[MoveRight] = isDown;
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
