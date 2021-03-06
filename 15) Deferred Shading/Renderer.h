#pragma once

#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/Camera.h"
#include "../../nclgl/OBJmesh.h"
#include "../../nclgl/heightmap.h"

#define LIGHTNUM 2

class Renderer : public OGLRenderer {
public:
	Renderer(Window &parent);
	virtual ~Renderer(void);

	virtual void RenderScene();
	virtual void UpdateScene(float msec);

protected:
	void FillBuffers();
	void DrawPointLights();
	void CombineBuffers();
	void GenerateScreenTexture(GLuint &into, bool depth = false);

	Shader* sceneShader;
	Shader* pointlightShader;
	Shader* combineShader;

	Light* pointLights;
	Mesh* heightMap;
	OBJMesh* sphere;
	Mesh* quad;
	Camera* camera;
	float rotation;
	GLuint bufferFBO;
	GLuint scene1Colour;
	GLuint bufferNormalTex;
	GLuint scene1Depth;
	GLuint pointLightFBO;
	GLuint lightEmissiveTex;
	GLuint lightSpecularTex;
};