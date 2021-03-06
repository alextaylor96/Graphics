#include "Renderer.h"


//make md5 mesh walk in scene 2 problem with anim not loading
//add color correct to a scene?
//tesselation on cube then warp over time/destroy/use normal map

Renderer::Renderer(Window &parent) : OGLRenderer(parent) {
	camera = new Camera();
	heightMap = new HeightMap(TEXTUREDIR"terrain.raw");
	quad = Mesh::GenerateQuad();
	screen = Mesh::GenerateQuad();

	planet = new OBJMesh();
	if (!planet->LoadOBJMesh(MESHDIR"cube.obj")) {
		return;
	}
	planet->type = GL_PATCHES;
	planet->SetTexture(SOIL_load_OGL_texture(TEXTUREDIR"lava.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS)); //http://spiralgraphics.biz/packs/terrain_volcanic_gaseous/previews/Lava%20Cracks.jpg
	planet->SetBumpMap(SOIL_load_OGL_texture(TEXTUREDIR"lavaBump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));
	planet->GenerateNormals();
	
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
	textureShader = new Shader(SHADERDIR"TexturedVertex.glsl", SHADERDIR"TexturedFragment.glsl");
	sceneShader = new Shader(SHADERDIR"shadowscenevert.glsl", SHADERDIR"shadowscenefrag.glsl");
	shadowShader = new Shader(SHADERDIR"shadowVert.glsl", SHADERDIR"shadowFrag.glsl");
	transitionShader = new Shader(SHADERDIR"transitionVertex.glsl", SHADERDIR"transitionFragment.glsl");
	colorCorrectShader = new Shader(SHADERDIR"ccVertex.glsl", SHADERDIR"ccFragment.glsl");
	
	planetShader = new Shader(SHADERDIR"planetVertex.glsl", SHADERDIR"planetFragment.glsl", "", SHADERDIR"planetTcs.glsl", SHADERDIR"planetTes.glsl");
	if (!sceneShader->LinkProgram() || !shadowShader->LinkProgram()) {
		return;
	}

	if (!planetShader->LinkProgram() || !transitionShader->LinkProgram()) {
		return;
	}


	hellData = new MD5FileData(MESHDIR"hellKnight.md5mesh");
	hellNode = new MD5Node(*hellData);
	hellNode2 = new MD5Node(*hellData);

	hellData->AddAnim(MESHDIR"idle2.md5anim");
	hellData->AddAnim(MESHDIR"walk7.md5anim");

	hellNode->PlayAnim(MESHDIR"idle2.md5anim");
	hellNode2->PlayAnim(MESHDIR"walk7.md5anim");

	if (!reflectShader->LinkProgram()) {
		return;
	}
	if (!skyboxShader->LinkProgram()) {
		return;
	}
	if (!lightShader->LinkProgram()) {
		return;
	}
	if (!textureShader->LinkProgram()) {
		return;
	}
	if (!colorCorrectShader->LinkProgram()) {
		return;
	}
	
	quad->SetTexture(SOIL_load_OGL_texture(TEXTUREDIR"water.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));
	heightMap->SetTexture(SOIL_load_OGL_texture(TEXTUREDIR"sand.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));
	heightMap->SetBumpMap(SOIL_load_OGL_texture(TEXTUREDIR"sandbump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));


	skybox = SOIL_load_OGL_cubemap(TEXTUREDIR"rusted_west.jpg", TEXTUREDIR"rusted_east.jpg", TEXTUREDIR"rusted_up.jpg",
		TEXTUREDIR"rusted_down.jpg", TEXTUREDIR"rusted_south.jpg", TEXTUREDIR"rusted_north.jpg",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	//http://www.custommapmakers.org/skyboxes.php
	spacebox = SOIL_load_OGL_cubemap(TEXTUREDIR"purplenebula_lf.tga", TEXTUREDIR"purplenebula_rt.tga", TEXTUREDIR"purplenebula_up.tga",
		TEXTUREDIR"purplenebula_dn.tga", TEXTUREDIR"purplenebula_bk.tga", TEXTUREDIR"purplenebula_ft.tga",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	shadebox = SOIL_load_OGL_cubemap(TEXTUREDIR"pr_ft.tga", TEXTUREDIR"pr_bk.tga", TEXTUREDIR"pr_up.tga",
		TEXTUREDIR"pr_dn.tga", TEXTUREDIR"pr_rt.tga", TEXTUREDIR"pr_lf.tga",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	basicFont = new Font(SOIL_load_OGL_texture(TEXTUREDIR"tahoma.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_COMPRESS_TO_DXT), 16, 16);

	if (!skybox) {
		return;
	}
	if (!spacebox) {
		return;
	}
	if (!shadebox) {
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
	if (!planet->GetTexture()) {
		return;
	}
	if (!planet->GetBumpMap()) {
		return;
	}


	SetTextureRepeating(quad->GetTexture(), true);
	SetTextureRepeating(heightMap->GetTexture(), true);
	SetTextureRepeating(heightMap->GetBumpMap(), true);

	//create fbo to render image for the screen into
	glGenTextures(1, &screenDepth);
	glBindTexture(GL_TEXTURE_2D, screenDepth);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height,
		0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	glGenTextures(1, &screenColour);
	glBindTexture(GL_TEXTURE_2D, screenColour);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, NULL);


	glGenFramebuffers(1, &screenFBO);


	glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_2D, screenDepth, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
		GL_TEXTURE_2D, screenDepth, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, screenColour, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !screenDepth || !screenColour) {
		return;
	}

	//create fbo to render image for post processing into
	glGenTextures(1, &postDepth);
	glBindTexture(GL_TEXTURE_2D, postDepth);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height,
		0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	glGenTextures(1, &postColour);
	glBindTexture(GL_TEXTURE_2D, postColour);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, NULL);


	glGenFramebuffers(1, &postFBO);


	glBindFramebuffer(GL_FRAMEBUFFER, postFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_2D, postDepth, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
		GL_TEXTURE_2D, postDepth, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, postColour, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !postDepth || !postColour) {
		return;
	}
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

	//create fbo to render scene 3 into
	glGenTextures(1, &scene3Depth);
	glBindTexture(GL_TEXTURE_2D, scene3Depth);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height,
		0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	glGenTextures(1, &scene3Colour);
	glBindTexture(GL_TEXTURE_2D, scene3Colour);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, NULL);


	glGenFramebuffers(1, &scene3FBO);


	glBindFramebuffer(GL_FRAMEBUFFER, scene3FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_2D, scene3Depth, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
		GL_TEXTURE_2D, scene3Depth, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, scene3Colour, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !scene3Depth || !scene3Colour) {
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


	for (int i = 0; i < 100; ++i) {
		recentFps[i] = 60;
	}

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
	glDeleteFramebuffers(1, &scene3FBO);
	currentShader = 0;
}

void Renderer::UpdateScene(float msec) {
	camera->UpdateCamera(msec);
	hellNode->Update(msec);
	hellNode2->Update(msec);
	viewMatrix = camera->BuildViewMatrix();
	waterRotate += msec / 1000.0f;
	fps = (1000/msec);
	framesLookup = (framesLookup + 1) % 100;
	recentFps[framesLookup] = fps;
	if (transitioningOut) {
		fade -= 0.01;
		offset += (msec / 1000.0f * 2.0f * 3.14159f * 0.75f);
		if (fade <= 0.0f) {
			tcsLevel = 1;
			transitioningOut = false;
			transitioningIn = true;
			switch (changingTo) {
			case 1:
				currentMainScene = 1;
				currentsubScene = 2;
				break;
			case 2:
				currentMainScene = 2;
				currentsubScene = 3;
				break;
			case 3:
				currentMainScene = 3;
				currentsubScene = 1;
				break;
			}
		}
	}
	if (transitioningIn) {
		fade += 0.01;
		offset += (msec / 1000.0f * 2.0f * 3.14159f * 0.75f);
		if (fade >= 1.0f) {
			transitioningIn = false;
			fade = 1.0f;
		}
	}

	if (!paused) {
		sceneTime += msec;
	}

	if (sceneTime > 10000.0f) {
		NextScene();
	}
	updateTcsVal();

}

void Renderer::changeScene(int changeTo)
{
	paused = false;
	sceneTime = 0;
	hellNode2->setAnimCycles(0);
	if (changeTo == 1) {
		transitioningOut = true;
		changingTo = 1;
	}
	if (changeTo == 2) {
		transitioningOut = true;
		changingTo = 2;
	}
	if (changeTo == 3) {
		transitioningOut = true;
		changingTo = 3;
	}
}

void Renderer::RenderScene() {

	glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, screenColour, 0);


	if (currentMainScene == 1 || currentsubScene == 1) {
		DrawScene1();
	}
	if (currentMainScene == 2 || currentsubScene == 2) {
		DrawScene2();
	}
	if (currentMainScene == 3 || currentsubScene == 3) {
		DrawScene3();
	}

	//draws the main and sub scenes to correct place on screen buffer
	DrawMainScene();
	DrawSubScene();
	
	//if in a transition add a post process blur effect
	if (transitioningOut || transitioningIn) {
		postProcessTransition();
	}
	//if scene 2 add color correction problem here get help
	/*if (currentMainScene == 2) {
		colorCorrection();
	}*/
	//displays the screen buffer
	DisplayScreen();

	DrawFPS("FPS: ", Vector3(0.0f, 0.0f, 0.0f), 16.0f);

	
	SwapBuffers();
}


void Renderer::DrawMainScene()
{
	glDisable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	SetCurrentShader(textureShader);
	projMatrix = Matrix4::Orthographic(-1, 1, 1, -1, -1, 1);

	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	textureMatrix.ToIdentity();
	UpdateShaderMatrices();

	switch (currentMainScene) {
	case 1:
		mainscene->SetTexture(scene1Colour);
		break;
	case 2:
		mainscene->SetTexture(scene2Colour);
		break;
	case 3:
		mainscene->SetTexture(scene3Colour);
		break;
	}
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mainscene->GetTexture());

	mainscene->Draw();

	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawSubScene()
{
	glDisable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);
	
	SetCurrentShader(textureShader);

	projMatrix = Matrix4::Orthographic(-1, 1, 1, -1, -1, 1)*
		Matrix4::Translation(Vector3(0.75f, 0.75f, 0))
		* Matrix4::Scale(Vector3(0.25f, 0.25f, 0.25f));
	
	viewMatrix.ToIdentity();
	textureMatrix.ToIdentity();
	modelMatrix.ToIdentity();
	UpdateShaderMatrices();
	
	switch (currentsubScene) {
	case 1:
		subscene->SetTexture(scene1Colour);
		break;
	case 2:
		subscene->SetTexture(scene2Colour);
		break;
	case 3:
		subscene->SetTexture(scene3Colour);
		break;
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, subscene->GetTexture());
	
	subscene->Draw();

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
	glUseProgram(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glEnable(GL_DEPTH_TEST);
}


void Renderer::DisplayScreen()
{
	glDisable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	SetCurrentShader(textureShader);
	projMatrix = Matrix4::Orthographic(-1, 1, 1, -1, -1, 1);

	viewMatrix.ToIdentity();
	textureMatrix.ToIdentity();
	UpdateShaderMatrices();

	screen->SetTexture(screenColour);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, screen->GetTexture());


	screen->Draw();

	glUseProgram(0);

	glEnable(GL_DEPTH_TEST);
}


void Renderer::postProcessTransition()
{
	glDisable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, postFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, postColour, 0);

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	SetCurrentShader(transitionShader);
	projMatrix = Matrix4::Orthographic(-1, 1, 1, -1, -1, 1);

	viewMatrix.ToIdentity();
	textureMatrix.ToIdentity();
	UpdateShaderMatrices();

	glActiveTexture(GL_TEXTURE13);
	glBindTexture(GL_TEXTURE_2D, screenColour);

	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 13);

	glUniform1f(glGetUniformLocation(currentShader->GetProgram(), "offset"), offset);

	glUniform2f(glGetUniformLocation(currentShader->GetProgram(), "pixelSize"), 1.0f/width, 1.0f/height);

	glUniform1f(glGetUniformLocation(currentShader->GetProgram(), "fade"),  fade);

	//draw screen with post processing effect shader
	screen->Draw();

	glUseProgram(0);

	//set the screen color to be the result of the post process
	std::swap(screenColour, postColour);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::colorCorrection()
{
	glDisable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, postFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, postColour, 0);

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	SetCurrentShader(colorCorrectShader);
	projMatrix = Matrix4::Orthographic(-1, 1, 1, -1, -1, 1);

	viewMatrix.ToIdentity();
	textureMatrix.ToIdentity();
	UpdateShaderMatrices();

	glActiveTexture(GL_TEXTURE13);
	glBindTexture(GL_TEXTURE_2D, screenColour);

	//draw screen with post processing effect shader
	screen->Draw();

	glUseProgram(0);

	//set the screen color to be the result of the post process
	std::swap(screenColour, postColour);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}


void Renderer::DrawScene1() {
	pitchHold = camera->GetPitch();
	posHold = camera->GetPosition();
	yawHold = camera->GetYaw();

	glBindFramebuffer(GL_FRAMEBUFFER, scene1FBO);

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	if (!paused || (currentMainScene != 1)) {
		camera->SetPitch(0.0f);
		camera->SetYaw(350.0f);
		camera->SetPosition(Vector3(1800.0f, 280.0f, 2900.0f));
	}
	
	light = new Light(Vector3((RAW_HEIGHT*HEIGHTMAP_X / 2.0f) - 500.0f, 1000.0F, (RAW_HEIGHT*HEIGHTMAP_Z / 2.0f)),
		Vector4(0.9f, 0.9f, 1.0f, 1), (RAW_WIDTH*HEIGHTMAP_X / 2.0f));

	
	viewMatrix = camera->BuildViewMatrix();

	DrawSkybox();
	DrawHeightmap();
	DrawWater();
	DrawHellKnight();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);

	camera->SetPitch(pitchHold);
	camera->SetYaw(yawHold);
	camera->SetPosition(posHold);
}

void Renderer::DrawScene2() {

	pitchHold = camera->GetPitch();
	posHold = camera->GetPosition();
	yawHold = camera->GetYaw();

	if (!paused || (currentMainScene != 2)) {
		camera->SetPitch(-8.0f);
		camera->SetYaw(40.0f);
		camera->SetPosition(Vector3(350.0f, 200.0f, 450.0f));
	}

	light = new Light(Vector3(-450.f, 200.0f, 280.f), Vector4(1, 1, 1, 1), 5500.0f);
	
	DrawShadowScene();

	glBindFramebuffer(GL_FRAMEBUFFER, scene2FBO);


	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	DrawShadeBox();
	DrawCombinedScene();
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glUseProgram(0);

	camera->SetPitch(pitchHold);
	camera->SetYaw(yawHold);
	camera->SetPosition(posHold);
}

void Renderer::DrawScene3()
{


	pitchHold = camera->GetPitch();
	posHold = camera->GetPosition();
	yawHold = camera->GetYaw();

	if (!paused || (currentMainScene != 3)) {
		camera->SetPitch(-8.0f);
		camera->SetYaw(40.0f);
		camera->SetPosition(Vector3(350.0f, 200.0f, 450.0f));
	}
	viewMatrix = camera->BuildViewMatrix();

	light = new Light(Vector3(-450.f, 200.0f, 280.f), Vector4(1, 1, 1, 1), 5500.0f);

	glBindFramebuffer(GL_FRAMEBUFFER, scene3FBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	DrawSpaceBox();
	DrawPlanet();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glUseProgram(0);

	camera->SetPitch(pitchHold);
	camera->SetYaw(yawHold);
	camera->SetPosition(posHold);

}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);
	SetCurrentShader(skyboxShader);
	UpdateShaderMatrices();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox);
	quad->Draw();
	glUseProgram(0);
	glDepthMask(GL_TRUE);
}

void Renderer::DrawSpaceBox()
{
	glDepthMask(GL_FALSE);
	SetCurrentShader(skyboxShader);
	UpdateShaderMatrices();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, spacebox);

	quad->Draw();
	glUseProgram(0);
	glDepthMask(GL_TRUE);
}

void Renderer::DrawShadeBox()
{
	glDepthMask(GL_FALSE);
	SetCurrentShader(skyboxShader);
	viewMatrix = camera->BuildViewMatrix();
	UpdateShaderMatrices();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, shadebox);

	quad->Draw();
	glUseProgram(0);
	glDepthMask(GL_TRUE);
}

void Renderer::DrawHellKnight()
{
	SetCurrentShader(textureShader);
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
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox);
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

	SetCurrentShader(textureShader);

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

void Renderer::NextScene()
{
	if (!transitioningOut && !transitioningIn) {
		if (currentMainScene == 3) {
			changeScene(1);
		}
		else {
			changeScene(currentMainScene + 1);
		}
	}
}

void Renderer::PrevScene()
{
	if (!transitioningOut && !transitioningIn) {
		if (currentMainScene == 1) {
			changeScene(3);
		}
		else {
			changeScene(currentMainScene - 1);
		}
	}
}

void Renderer::DrawMesh() {
	modelMatrix.ToIdentity();
	modelMatrix = modelMatrix * Matrix4::Translation(Vector3(750 - (130 * hellNode2->getAnimCycles()), 0, 0));
	Matrix4 tempMatrix = textureMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "textureMatrix"), 1, false, *&tempMatrix.values);
	glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "modelMatrix"), 1, false, *&modelMatrix.values);
	hellNode2->Draw(*this);
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

int rotation=1;
int tcsLevel = 1;

void Renderer::DrawPlanet()
{
	SetCurrentShader(planetShader);

	modelMatrix.ToIdentity();
	modelMatrix = Matrix4::Translation(Vector3(0,100,0)) * Matrix4::Scale(Vector3(100,100,100)) * Matrix4::Rotation(rotation,Vector3(0,100,0));
	textureMatrix.ToIdentity();
	rotation++;
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "bumpTex"), 1);
	glUniform1f(glGetUniformLocation(currentShader->GetProgram(), "tcsLevel"), tcsLevel);

	UpdateShaderMatrices();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, planet->GetTexture());
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, planet->GetBumpMap());
	
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_N))
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	planet->Draw();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glUseProgram(0);
}


void Renderer::updateTcsVal()
{
	
	if (sceneTime > 6000.0f) {
		tcsLevel = 2;
	}
	if (sceneTime > 7000.0f) {
		tcsLevel = 5;
	}
	if (sceneTime > 8000.0f) {
		tcsLevel = 10;
	}
	if (sceneTime > 9000.0f) {
		tcsLevel = 25;
	}
}