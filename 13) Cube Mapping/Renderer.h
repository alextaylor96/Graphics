#pragma once
#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/Camera.h"
#include "../../nclgl/HeightMap.h"
#include "../../nclgl/textmesh.h"
#include "../../nclgl/MD5Mesh.h"
#include "../../nclgl/MD5Node.h"
#include "../../nclgl/OBJmesh.h"

#define SHADOWSIZE 2048

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
	void DrawHellKnight();
	void DrawFPS(const std::string &text, const Vector3 &position, const float size = 10.0f);

	Shader * lightShader;
	Shader * reflectShader;
	Shader * skyboxShader;
	Shader* textShader;
	Shader* shadowShader;

	HeightMap * heightMap;
	Mesh * quad;


	MD5FileData* hellData;
	MD5Node* hellNode;
	Light * light;
	Camera * camera;

	GLuint cubeMap;
	GLuint shadowTex;

	float fps = 0;
	float recentFps[100];
	int framesLookup = 0;

	Font*	basicFont;
	float waterRotate;
};