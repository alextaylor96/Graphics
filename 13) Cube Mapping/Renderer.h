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

	void setPaused(bool setTo) {
		paused = setTo;
	}

	void NextScene();
	void PrevScene();


protected:
	void changeScene(int changeTo);
	bool paused = false;
	bool transitioningOut = false;
	bool transitioningIn = false;
	float offset = 0;
	float fade = 1.0f;

	float sceneTime = 0.0f;

	int currentMainScene = 1;
	int currentsubScene = 2;
	int changingTo;
	
	GLuint screenFBO;
	GLuint screenDepth;
	GLuint screenColour;

	GLuint postFBO;
	GLuint postDepth;
	GLuint postColour;

	GLuint scene1FBO;
	GLuint scene1Depth;
	GLuint scene1Colour;

	GLuint scene2FBO;
	GLuint scene2Depth;
	GLuint scene2Colour;

	GLuint scene3FBO;
	GLuint scene3Depth;
	GLuint scene3Colour;

	GLuint scenePostFBO;
	GLuint scenePostDepth;
	GLuint scenePostColour;

    Mesh * screen;
	Mesh * mainscene;
	Mesh * subscene;
	
	void DrawMainScene();
    void DrawSubScene();

	void DrawBloom();

	void DisplayScreen();
	
	void postProcessTransition();
	void colorCorrection();

	void DrawHeightmap();
	void DrawWater();
	void DrawSkybox();
	void DrawHellKnight();
	void DrawFPS(const std::string &text, const Vector3 &position, const float size = 10.0f);

	void DrawScene1();
	void DrawScene2();
	void DrawScene3();

	Shader * lightShader;
	Shader * reflectShader;
	Shader * skyboxShader;
	Shader* textureShader;
	Shader* transitionShader;



	HeightMap * heightMap;
	Mesh * quad;

	OBJMesh* planet;

	MD5FileData* hellData;
	MD5Node* hellNode;
	MD5Node* hellNode2;

	Light * light;
	Camera * camera;

	GLuint skybox;
	GLuint spacebox;
	GLuint shadebox;

	float fps = 0;
	float recentFps[100];
	int framesLookup = 0;

	Font*	basicFont;
	float waterRotate;

	void DrawMesh();
	void DrawFloor();
	void DrawPlanet();
	void DrawSpaceBox();
	void DrawShadeBox();
	void DrawShadowScene();
	void DrawCombinedScene();
	
	Shader* sceneShader;
	Shader* shadowShader;
	Shader* colorCorrectShader;

	Shader* planetShader;
	Shader* bloomShader;

	GLuint shadowMap;
	GLuint shadowFBO;
	
	Mesh* floor;
	int tcsLevel;
	void updateTcsVal();

	float pitchHold;
	Vector3 posHold;
	float yawHold;
};