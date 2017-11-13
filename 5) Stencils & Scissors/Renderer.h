#pragma once
#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/Camera.h"

class Renderer : public OGLRenderer {
public:
	Renderer(Window & parent);
	virtual ~Renderer(void);
	virtual void RenderScene();
	void ToggleScissor();
	void ToggleStencil();

protected:
	Mesh * triangle;
	Mesh * quad;
	bool usingScissor;
	bool usingStencil;

};