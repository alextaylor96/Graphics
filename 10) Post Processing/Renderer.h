#pragma once

#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/Camera.h"
#include "../../nclgl/HeightMap.h"



class Renderer : public OGLRenderer {
public:
	Renderer(Window & parent);
	virtual ~Renderer(void);
	virtual void RenderScene();
	virtual void UpdateScene(float msec);

	int POST_PASSES = 10;
	bool increasing = true;

protected:



	void PresentScene();
	void DrawPostProcess();
	void DrawScene();

	Shader * sceneShader;
	Shader * combineShader;
	Shader * processShader;

	Camera * camera;
	Mesh * quad;
	HeightMap * heightMap;
	GLuint bufferFBO;
	GLuint processFBO;
	GLuint bufferColourTex[2];
	GLuint bufferDepthTex;
};
