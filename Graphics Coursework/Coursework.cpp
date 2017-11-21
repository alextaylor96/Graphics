#pragma comment(lib, "nclgl.lib")

#include "../../nclgl/window.h"
#include "Renderer.h"

int main() {
	Window w("Cube Mapping! sky textures courtesy of http://www.hazelwhorley.com", 800, 600, false);
	if (!w.HasInitialised()) {
		return -1;
	}

	Renderer renderer(w);
	if (!renderer.HasInitialised()) {
		return -1;
	}

	w.LockMouseToWindow(false);
	w.ShowOSPointer(true);


	while (w.UpdateWindow() && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE)) {
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_1)) {
			renderer.changeScene(1);
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_2)) {
			renderer.changeScene(2);
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_3)) {
			renderer.changeScene(3);
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_SPACE)) {
			renderer.setPaused(true);
		}
		renderer.UpdateScene(w.GetTimer()->GetTimedMS());
		renderer.RenderScene();
	}

	return 0;
}