#include "Renderer.h"

//Fbo to show 3 scenes at once
//shader to use fbos as blending/bluring transition
//make md5 mesh walk in scene 2
//bloom on gem and destroy with lazer in shader

Renderer::Renderer(Window &parent) : OGLRenderer(parent) {
	camera = new Camera();
	heightMap = new HeightMap(TEXTUREDIR"terrain.raw");
	quad = Mesh::GenerateQuad();

	mainscene = Mesh::GenerateQuad();
	subscene = Mesh::GenerateQuad();
	
	camera->SetPitch(0.0f);
	camera->SetYaw(350.0f);
	camera->SetPosition(Vector3(1800.0f, 280.0f, 2900.0f));

	light = new Light(Vector3((RAW_HEIGHT*HEIGHTMAP_X / 2.0f) - 500.0f, 1000.0F, (RAW_HEIGHT*HEIGHTMAP_Z / 2.0f)),
		Vector4(0.9f, 0.9f, 1.0f, 1), (RAW_WIDTH*HEIGHTMAP_X / 2.0f));

	reflectShader = new Shader(SHADERDIR"PerPixelVertex.glsl", SHADERDIR"reflectFragment.glsl");
	skyboxShader = new Shader(SHADERDIR"skyboxVertex.glsl", SHADERDIR"skyboxFragment.glsl");
	lightShader = new Shader(SHADERDIR"BumpVertex.glsl", SHADERDIR"BumpFragment.glsl");
	textShader = new Shader(SHADERDIR"TexturedVertex.glsl", SHADERDIR"TexturedFragment.glsl");
	sceneShader = new Shader(SHADERDIR"shadowscenevert.glsl", SHADERDIR"shadowscenefrag.glsl");
	shadowShader = new Shader(SHADERDIR"shadowVert.glsl", SHADERDIR"shadowFrag.glsl");
	
	if (!sceneShader->LinkProgram() || !shadowShader->LinkProgram()) {
		return;
	}


	hellData = new MD5FileData(MESHDIR"hellKnight.md5mesh");
	hellNode = new MD5Node(*hellData);

	hellData->AddAnim(MESHDIR"idle2.md5anim");
	hellNode->PlayAnim(MESHDIR"idle2.md5anim");

	if (!reflectShader->LinkProgram()) {
		return;
	}
	if (!skyboxShader->LinkProgram()) {
		return;
	}
	if (!lightShader->LinkProgram()) {
		return;
	}
	if (!textShader->LinkProgram()) {
		return;
	}

	quad->SetTexture(SOIL_load_OGL_texture(TEXTUREDIR"water.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));
	heightMap->SetTexture(SOIL_load_OGL_texture(TEXTUREDIR"sand.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));
	heightMap->SetBumpMap(SOIL_load_OGL_texture(TEXTUREDIR"sandbump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));


	cubeMap = SOIL_load_OGL_cubemap(TEXTUREDIR"rusted_west.jpg", TEXTUREDIR"rusted_east.jpg", TEXTUREDIR"rusted_up.jpg",
		TEXTUREDIR"rusted_down.jpg", TEXTUREDIR"rusted_south.jpg", TEXTUREDIR"rusted_north.jpg",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	basicFont = new Font(SOIL_load_OGL_texture(TEXTUREDIR"tahoma.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_COMPRESS_TO_DXT), 16, 16);

	if (!cubeMap) {
		return;
	}
	if (!quad->GetTexture()) {
		return;
	}
	if (!heightMap->GetTexture()) {
		return;
	}
	if (!heightMap->GetBumpMap()) {
		return;
	}


	SetTextureRepeating(quad->GetTexture(), true);
	SetTextureRepeating(heightMap->GetTexture(), true);
	SetTextureRepeating(heightMap->GetBumpMap(), true);

	//create fbo to render scene 1 into
	glGenTextures(1, &scene1Depth);
	glBindTexture(GL_TEXTURE_2D, scene1Depth);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height,
		0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	glGenTextures(1, &scene1Colour);
	glBindTexture(GL_TEXTURE_2D, scene1Colour);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	
	glGenFramebuffers(1, &scene1FBO);
	

	glBindFramebuffer(GL_FRAMEBUFFER, scene1FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_2D, scene1Depth, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
		GL_TEXTURE_2D, scene1Depth, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, scene1Colour, 0);
	
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=	GL_FRAMEBUFFER_COMPLETE || !scene1Depth || !scene1Colour) {
		return;
	}

	//create fbo to render scene 2 into
	glGenTextures(1, &scene2Depth);
	glBindTexture(GL_TEXTURE_2D, scene2Depth);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height,
		0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	glGenTextures(1, &scene2Colour);
	glBindTexture(GL_TEXTURE_2D, scene2Colour);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, NULL);


	glGenFramebuffers(1, &scene2FBO);


	glBindFramebuffer(GL_FRAMEBUFFER, scene2FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_2D, scene2Depth, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
		GL_TEXTURE_2D, scene2Depth, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, scene2Colour, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !scene2Depth || !scene2Colour) {
		return;
	}


	//fbo used in shadow mapping
	glGenTextures(1, &shadowMap);
	glBindTexture(GL_TEXTURE_2D, shadowMap);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &shadowFBO);

	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
	glDrawBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	floor = Mesh::GenerateQuad();
	floor->SetTexture(SOIL_load_OGL_texture(TEXTUREDIR"brick.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));
	floor->SetBumpMap(SOIL_load_OGL_texture(TEXTUREDIR"brickDOT3.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));

	waterRotate = 0.0f;
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);


	init = true;
}

Renderer::~Renderer(void) {
	delete camera;
	delete heightMap;
	delete quad;
	delete reflectShader;
	delete skyboxShader;
	delete lightShader;
	delete light;
	delete hellData;
	delete hellNode;
	delete sceneShader;
	delete shadowShader;
	delete subscene;
	glDeleteFramebuffers(1, &scene1FBO);
	glDeleteFramebuffers(1, &scene2FBO);
	currentShader = 0;
}

void Renderer::UpdateScene(float msec) {
	camera->UpdateCamera(msec);
	hellNode->Update(msec);
	viewMatrix = camera->BuildViewMatrix();
	waterRotate += msec / 1000.0f;
	fps = (1000/msec);
	framesLookup = (framesLookup + 1) % 100;
	recentFps[framesLookup] = fps;
}

void Renderer::changeScene(int changeTo)
{
	if (changeTo == 1) {
		currentMainScene = 1;
	}
	if (changeTo == 2) {
		currentMainScene = 2;
	}
}

void Renderer::RenderScene() {
	DrawScene1();
	DrawScene2();

	DisplayMain();
	DisplaySub();
	
	SwapBuffers();
}

void Renderer::DisplayMain()
{
	//problem here
	glDisable(GL_DEPTH_TEST);

	//doesnt bind texture properly in fbo, doesnt draw scene 1 correctly
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	SetCurrentShader(textShader);
	projMatrix = Matrix4::Orthographic(-1, 1, 1, -1, -1, 1) *
		Matrix4::Translation(Vector3(-0.75f, -0.75f, 0))
		* Matrix4::Scale(Vector3(0.25f, 0.25f, 0.25f));
	viewMatrix.ToIdentity();
	UpdateShaderMatrices();

	switch (currentMainScene) {
	case 1:
		mainscene->SetTexture(scene1FBO);
		break;
	case 2:
		mainscene->SetTexture(scene2FBO);
		break;
	}
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mainscene->GetTexture());
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mainscene->GetTexture());


	mainscene->Draw();

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
	glUseProgram(0);
	DrawFPS("FPS: ", Vector3(0.0f, 0.0f, 0.0f), 16.0f);

	glEnable(GL_DEPTH_TEST);
}

void Renderer::DisplaySub()
{
	glDisable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	SetCurrentShader(textShader);
	projMatrix = Matrix4::Orthographic(-1, 1, 1, -1, -1, 1);
	//	Matrix4::Translation(Vector3(0, 0, 0))*
	//	Matrix4::Scale(Vector3(1.0f, 0.25f, 0.25f));
		
	viewMatrix.ToIdentity();
	UpdateShaderMatrices();

	switch (currentMainScene) {
	case 2:
		subscene->SetTexture(scene1FBO);
		break;
	case 1:
		subscene->SetTexture(scene2FBO);
		break;
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, subscene->GetTexture());
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, subscene->GetTexture());


	subscene->Draw();

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
	glUseProgram(0);


	glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawScene1() {
	glBindFramebuffer(GL_FRAMEBUFFER, scene1FBO);

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	camera->SetPitch(0.0f);
	camera->SetYaw(350.0f);
	camera->SetPosition(Vector3(1800.0f, 280.0f, 2900.0f));

	light = new Light(Vector3((RAW_HEIGHT*HEIGHTMAP_X / 2.0f) - 500.0f, 1000.0F, (RAW_HEIGHT*HEIGHTMAP_Z / 2.0f)),
		Vector4(0.9f, 0.9f, 1.0f, 1), (RAW_WIDTH*HEIGHTMAP_X / 2.0f));

	DrawSkybox();
	DrawHeightmap();
	DrawWater();
	DrawHellKnight();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);
}

void Renderer::DrawScene2() {

	glBindFramebuffer(GL_FRAMEBUFFER, scene2FBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	camera->SetPitch(-8.0f);
	camera->SetYaw(40.0f);
	camera->SetPosition(Vector3(350.0f, 200.0f, 450.0f));
	light = new Light(Vector3(-450.f, 200.0f, 280.f), Vector4(1, 1, 1, 1), 5500.0f);
	DrawShadowScene();
	DrawCombinedScene();
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glUseProgram(0);
}


void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);
	SetCurrentShader(skyboxShader);
	UpdateShaderMatrices();
	quad->Draw();
	glUseProgram(0);
	glDepthMask(GL_TRUE);
}

void Renderer::DrawHellKnight()
{
	SetCurrentShader(textShader);
	modelMatrix.ToIdentity();
	modelMatrix = Matrix4::Translation(Vector3(2400, 280, 2000));
	textureMatrix.ToIdentity();

	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);

	UpdateShaderMatrices();

	hellNode->Draw(*this);
	
	glUseProgram(0);
}

void Renderer::DrawHeightmap() {
	SetCurrentShader(lightShader);
	SetShaderLight(*light);
	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "bumpTex"), 1);

	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();

	UpdateShaderMatrices();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, heightMap->GetTexture());
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, heightMap->GetBumpMap());

	heightMap->Draw();

	glUseProgram(0);
}

void Renderer::DrawWater() {
	SetCurrentShader(reflectShader);
	SetShaderLight(*light);
	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "cubeTex"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
	float heightX = (RAW_WIDTH*HEIGHTMAP_X / 2.0f);
	float heightY = 256 * HEIGHTMAP_Y / 3.0f;
	float heightZ = (RAW_HEIGHT * HEIGHTMAP_Z / 2.0f);

	modelMatrix = Matrix4::Translation(Vector3(heightX, heightY, heightZ)) *
		Matrix4::Scale(Vector3(heightX, 1, heightZ)) * Matrix4::Rotation(90, Vector3(1.0f, 0.0f, 0.0f));

	textureMatrix =  Matrix4::Scale(Vector3(10.0f, 10.0f, 10.0f)) * Matrix4::Rotation(waterRotate, Vector3(0.0f, 0.0f, 1.0f));

	UpdateShaderMatrices();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, quad->GetTexture());
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, quad->GetBumpMap());

	quad->Draw();
	glActiveTexture(GL_TEXTURE0);
	glUseProgram(0);
}

void Renderer::DrawFPS(const std::string &text, const Vector3 &position, const float size) {
	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	SetCurrentShader(textShader);

	float temp = 0;
	for (int i = 0; i < 100; ++i) {
		temp += recentFps[i];
	}
	
	TextMesh* mesh = new TextMesh(text + std::to_string((int)(temp/100)), *basicFont);

	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
	
	modelMatrix = Matrix4::Translation(Vector3(position.x, height - position.y, position.z)) * Matrix4::Scale(Vector3(size, size, 1));
	viewMatrix.ToIdentity();
	projMatrix = Matrix4::Orthographic(-1.0f, 1.0f, (float)width, 0.0f, (float)height, 0.0f);

	
	UpdateShaderMatrices();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mesh->GetTexture());
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, mesh->GetBumpMap());

	mesh->Draw();

	delete mesh; 

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glUseProgram(0);
}


void Renderer::DrawShadowScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

	glClear(GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	SetCurrentShader(shadowShader);

	viewMatrix = Matrix4::BuildViewMatrix(light->GetPosition(), Vector3(0, 0, 0));
	textureMatrix = biasMatrix*(projMatrix*viewMatrix);

	UpdateShaderMatrices();

	DrawFloor();
	DrawMesh();

	glUseProgram(0);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawCombinedScene() {
	SetCurrentShader(sceneShader);

	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "bumpTex"), 1);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "shadowTex"), 2);

	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	SetShaderLight(*light);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowMap);

	viewMatrix = camera->BuildViewMatrix();

	UpdateShaderMatrices();

	DrawFloor();
	DrawMesh();

	glUseProgram(0);
}

void Renderer::DrawMesh() {
	modelMatrix.ToIdentity();
	Matrix4 tempMatrix = textureMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "textureMatrix"), 1, false, *&tempMatrix.values);
	glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "modelMatrix"), 1, false, *&modelMatrix.values);
	hellNode->Draw(*this);
}

void Renderer::DrawFloor() {
	modelMatrix = Matrix4::Rotation(90, Vector3(1, 0, 0)) * Matrix4::Scale(Vector3(450, 450, 1));
	Matrix4 tempMatrix = textureMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "textureMatrix"), 1, false, *&tempMatrix.values);
	glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "modelMatrix"), 1, false, *&modelMatrix.values);


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, floor->GetTexture());
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, floor->GetBumpMap());

	floor->Draw();
}