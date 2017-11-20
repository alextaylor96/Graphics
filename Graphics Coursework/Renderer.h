#pragma once
#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/Camera.h"
#include "../../nclgl/HeightMap.h"
#include "../../nclgl/textmesh.h"

class Renderer : public OGLRenderer {
public:
	Renderer(Window & parent);
	virtual ~Renderer(void);

	virtual void RenderScene();
	virtual void UpdateScene(float msec);


protected:
	void DrawHeightmap();
	void DrawWater();
	void DrawSkybox();
	void DrawText(const std::string &text, const Vector3 &position, const float size = 10.0f);

	Shader * lightShader;
	Shader * reflectShader;
	Shader * skyboxShader;
	Shader* textShader;

	HeightMap * heightMap;
	Mesh * quad;

	Light * light;
	Camera * camera;

	GLuint cubeMap;

	//Font*	basicFont;
	float waterRotate;
};