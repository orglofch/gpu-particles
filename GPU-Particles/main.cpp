/*
* Copyright (c) 2014 Owen Glofcheski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
*    1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
*
*    2. Altered source versions must be plainly marked as such, and must not
*    be misrepresented as being the original software.
*
*    3. This notice may not be removed or altered from any source
*    distribution.
*/

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <GL\glew.h>
#include <GL\freeglut.h>
#include <iostream>
#include <math.h>

#include "GLUtil.hpp"

using namespace std;

enum Mode
{
	MODE_NORMAL,
	MODE_FIRE,

	MODE_COUNT,
};

// Simulation state
Mode kMode = MODE_NORMAL;

size_t kNumParticles = 1900 * 1900;
size_t kTexWidth = (size_t)sqrt(kNumParticles);
size_t kTexHeight = kTexWidth;
bool kPaused = false;
float kRotationY = 0.0f;
float kTranslationZ = -2.0f;

// Shaders
GLuint kUpdateShaderProgram = 0;
GLuint kUpdateVertexShader = 0;
GLuint kUpdateFragmentShader = 0;

GLuint kRenderShaderProgram = 0;
GLuint kRenderVertexShader = 0;
GLuint kRenderFragmentShader = 0;

GLuint kNoiseShaderProgram = 0;
GLuint kNoiseVertexShader = 0;
GLuint kNoiseFragmentShader = 0;

// Uniforms
GLint kPositionUniformLocationUpdate = -1;
GLint kVelocityUniformLocationUpdate = -1;
GLint kCurlNoiseUniformLocationUpdate = -1;
GLint kMousePositionUniformLocationUpdate = -1;
GLint kMouseDownUniformLocationUpdate = -1;
GLint kTimeUniformLocationUpdate = -1;
GLint kModeUniformLocationUpdate = -1;

GLint kPositionUniformLocationRender = -1;
GLint kVelocityUniformLocationRender = -1;

// Textures
GLuint kTexturePosition[2] = { 0, 0 };
GLuint kTextureVelocity[2] = { 0, 0 };
GLuint kTextureCurlNoise = 0;
int kFrontTexture = 0;

// Buffers
GLuint kAttributeBuffer = 0;
GLuint kFBO[2] = { 0, 0 };
GLuint kNoiseFBO = 0;

// Input state
GLfloat kMouse[2] = { 0.0, 0.0 };
bool kLeftMouseDown = false;
bool kRotateLeft = false;
bool kRotateRight = false;
bool kZoomIn = false;
bool kZoomOut = false;
int kTime = 0;

// Window state
size_t kWindowSize[2] = { 0, 0 };
int kMainWindow = 0;

void Resize(int width, int height)
{
	kWindowSize[0] = width;
	kWindowSize[1] = height;
	SetPerspectiveProjection(60, kWindowSize[0], kWindowSize[1], 0.1, 1000.0);
}

void Update(void)
{
	glEnable(GL_TEXTURE_2D);

	glBindFramebuffer(GL_FRAMEBUFFER, kFBO[!kFrontTexture]);
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, (GLenum*)buffers);
	glViewport(0, 0, kTexWidth, kTexHeight);
	glDisable(GL_BLEND);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, kTexturePosition[kFrontTexture]);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, kTextureVelocity[kFrontTexture]);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, kTextureCurlNoise);

	glUseProgram(kUpdateShaderProgram);

	glUniform2fv(kMousePositionUniformLocationUpdate, 1, kMouse);
	glUniform1f(kMouseDownUniformLocationUpdate, kLeftMouseDown);
	glUniform1f(kTimeUniformLocationUpdate, kTime++);
	glUniform1f(kModeUniformLocationUpdate, kMode);

	// TODO(orglofch): This rebinds the active texture
	DrawTexturedQuad(kTextureCurlNoise);

	glUseProgram(0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Render(void) 
{
	if (!kPaused) {
		Update();
	}
	glEnable(GL_BLEND);
	
	if (kRotateLeft)
		kRotationY += 2.0f;
	if (kRotateRight)	
		kRotationY -= 2.0f;
	if (kZoomIn)
		kTranslationZ += 0.03f;
	if (kZoomOut)
		kTranslationZ -= 0.03f;

	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, kWindowSize[0], kWindowSize[1]);

	glLoadIdentity();
	glTranslatef(0.0, 0.0, kTranslationZ);
	glRotatef(kRotationY, 0.0f, 1.0f, 0.0f);

	glUseProgram(0);
	//DrawTexturedQuad(kTextureCurlNoise);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, kTexturePosition[!kFrontTexture]);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, kTextureVelocity[!kFrontTexture]);

	glBindBuffer(GL_ARRAY_BUFFER, kAttributeBuffer);
	glEnableVertexAttribArray(0);
	GL_CHECK(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat)* 3, (char *)0));
	glEnableVertexAttribArray(1);
	GL_CHECK(glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (char *)2));

	glUseProgram(kRenderShaderProgram);

	glDrawArrays(GL_LINES, 0, kNumParticles);

	glUseProgram(0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glutSwapBuffers();
	glutPostRedisplay();

	if (!kPaused) {
		kFrontTexture = !kFrontTexture;
	}
}

void UpdateMouseNormalized(int x, int y)
{
	kMouse[0] = 2.0f * x / kWindowSize[0] - 1.0f;
	kMouse[1] = -2.0f * y / kWindowSize[1] + 1.0f;
}

void HandleMouseMove(int x, int y)
{
	UpdateMouseNormalized(x, y);
}

void HandleMouseButton(int button, int state, int x, int y)
{
	UpdateMouseNormalized(x, y);
	switch (button) 
	{
		case GLUT_LEFT_BUTTON:
			kLeftMouseDown = (state == GLUT_DOWN);
			break;
		case GLUT_RIGHT_BUTTON:
			if (state == GLUT_DOWN) {
				kPaused = !kPaused;
				kFrontTexture = !kFrontTexture;
			}
			break;
	}
}

void HandlePressNormalKeys(unsigned char key, int x, int y) 
{
	switch (key) 
	{
		case 'a':
		case 'A':
			kRotateLeft = true;
			break;
		case 'd':
		case 'D':
			kRotateRight = true;
			break;
		case 'w':
		case 'W':
			kZoomIn = true;
			break;
		case 's':
		case 'S':
			kZoomOut = true;
			break;
		case 'q':
		case 'Q':
		case 27: 
			exit(EXIT_SUCCESS);
	}
}

void HandleReleaseNormalKeys(unsigned char key, int x, int y)
{
	switch (key) 
	{
		case 'a':
		case 'A':
			kRotateLeft = false;
			break;
		case 'd':
		case 'D':
			kRotateRight = false;
			break;
		case 'w':
		case 'W':
			kZoomIn = false;
			break;
		case 's':
		case 'S':
			kZoomOut = false;
			break;
		case 'm':
		case 'M':
			kMode = (Mode)((kMode + 1) % MODE_COUNT);
			break;
	}
}

void HandlePressSpecialKey(int key, int x, int y) 
{
	switch (key) 
	{
		case GLUT_KEY_UP: 
		case GLUT_KEY_DOWN: 
			break;
	}
}

void HandleReleaseSpecialKey(int key, int x, int y) 
{
	switch (key) 
	{
		case GLUT_KEY_UP:
		case GLUT_KEY_DOWN:
			break;
	}
}

void Init(void) 
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_NORMALIZE);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	SetPerspectiveProjection(60, kWindowSize[0], kWindowSize[1], 0.1, 1000.0);

	// Update shader
	LoadShader("update.vert", "update.frag",
		kUpdateShaderProgram, kUpdateVertexShader, kUpdateFragmentShader);
	kPositionUniformLocationUpdate = glGetUniformLocation(kUpdateShaderProgram, "uPositions");
	kVelocityUniformLocationUpdate = glGetUniformLocation(kUpdateShaderProgram, "uVelocities");
	kCurlNoiseUniformLocationUpdate = glGetUniformLocation(kUpdateShaderProgram, "uCurlNoise");
	kMousePositionUniformLocationUpdate = glGetUniformLocation(kUpdateShaderProgram, "uMousePosition");
	kMouseDownUniformLocationUpdate = glGetUniformLocation(kUpdateShaderProgram, "uMouseDown");
	kTimeUniformLocationUpdate = glGetUniformLocation(kUpdateShaderProgram, "uTime");
	kModeUniformLocationUpdate = glGetUniformLocation(kUpdateShaderProgram, "uMode");

	glUseProgram(kUpdateShaderProgram);
	glUniform1i(kPositionUniformLocationUpdate, 1);
	glUniform1i(kVelocityUniformLocationUpdate, 2);
	glUniform1i(kCurlNoiseUniformLocationUpdate, 3);

	// Render shader
	LoadShader("render.vert", "render.frag", 
		kRenderShaderProgram, kRenderVertexShader, kRenderFragmentShader);

	kPositionUniformLocationRender = glGetUniformLocation(kRenderShaderProgram, "uPositions");
	kVelocityUniformLocationRender = glGetUniformLocation(kRenderShaderProgram, "uVelocities");

	glUseProgram(kRenderShaderProgram);
	glUniform1i(kPositionUniformLocationRender, 1);
	glUniform1i(kVelocityUniformLocationRender, 2);

	// Noise shader
	LoadShader("noise.vert", "noise.frag",
		kNoiseShaderProgram, kNoiseVertexShader, kNoiseFragmentShader);

	glutDisplayFunc(Render);
	glutReshapeFunc(Resize);

	glutIgnoreKeyRepeat(1);
	glutMouseFunc(HandleMouseButton);
	glutMotionFunc(HandleMouseMove);
	glutPassiveMotionFunc(HandleMouseMove);
	glutKeyboardFunc(HandlePressNormalKeys);
	glutKeyboardUpFunc(HandleReleaseNormalKeys);
	glutSpecialFunc(HandlePressSpecialKey);
	glutSpecialUpFunc(HandleReleaseSpecialKey);
}

void GenerateNoise(void)
{
	GLfloat *noiseData = new GLfloat[kTexWidth * kTexHeight * 4];
	for (size_t i = 0; i < kTexWidth * kTexHeight * 4; ++i) {
		noiseData[i] = 0.0f;
	}
	CreateTexture2D(&kTextureCurlNoise, kTexWidth, kTexHeight, noiseData);
	delete[] noiseData;

	glGenFramebuffers(1, &kNoiseFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, kNoiseFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, kTextureCurlNoise, 0);

	glUseProgram(kNoiseShaderProgram);

	glBindFramebuffer(GL_FRAMEBUFFER, kNoiseFBO);
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, (GLenum*)buffers);
	glViewport(0, 0, kTexWidth, kTexHeight);
	glDisable(GL_BLEND);

	DrawTexturedQuad(kTextureCurlNoise);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GenerateParticles(void)
{
	GLfloat *pData = new GLfloat[kTexWidth * kTexHeight * 4];
	GLfloat *vData = new GLfloat[kTexWidth * kTexHeight * 4];
	for (size_t i = 0; i < kTexWidth * kTexHeight * 4; i += 4) {
		pData[i + 0] = 1.0f * rand() / RAND_MAX - 0.5f;
		pData[i + 1] = 1.0f * rand() / RAND_MAX - 0.5f;
		pData[i + 2] = 1.0f * rand() / RAND_MAX - 0.5f;
		pData[i + 3] = 1.0f;

		vData[i + 0] = 0.0f;
		vData[i + 1] = 0.0f;
		vData[i + 2] = 0.0f;
		vData[i + 3] = 0.0f;
	}
	CreateTexture2D(&kTexturePosition[0], kTexWidth, kTexHeight, pData);
	CreateTexture2D(&kTextureVelocity[0], kTexWidth, kTexHeight, vData);

	glGenFramebuffers(2, kFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, kFBO[0]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, kTexturePosition[0], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
		GL_TEXTURE_2D, kTextureVelocity[0], 0);

	for (size_t i = 0; i < kTexWidth * kTexHeight * 4; ++i) {
		pData[i] = 0.0f;
		vData[i] = 0.0f;
	}
	CreateTexture2D(&kTexturePosition[1], kTexWidth, kTexHeight, pData);
	CreateTexture2D(&kTextureVelocity[1], kTexWidth, kTexHeight, vData);

	glBindFramebuffer(GL_FRAMEBUFFER, kFBO[1]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, kTexturePosition[1], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
		GL_TEXTURE_2D, kTextureVelocity[1], 0);

	delete[] pData;
	delete[] vData;

	// Create dummy VBO
	GLfloat *attributeData = new GLfloat[kTexWidth * kTexHeight * 6];
	for (size_t x = 0; x < kTexWidth; ++x) {
		for (size_t y = 0; y < kTexHeight; ++y) {
			size_t i = (y * kTexWidth + x) * 6;
			attributeData[i + 0] = attributeData[i + 3] = 1.0f * (x + 0.5f) / kTexWidth; // s
			attributeData[i + 1] = attributeData[i + 4] = 1.0f * (y + 0.5f) / kTexHeight; // t
			attributeData[i + 2] = 1.0f; // head of line
			attributeData[i + 5] = 0.0f; // not head of line
		}
	}

	glGenBuffers(1, &kAttributeBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, kAttributeBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* kTexWidth * kTexHeight * 6, 
		attributeData, GL_STATIC_DRAW);
	delete[] attributeData;

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Cleanup(void)
{
	glDeleteBuffers(1, &kAttributeBuffer);

	glDeleteFramebuffers(2, kFBO);
	glDeleteTextures(2, kTexturePosition);
	glDeleteTextures(2, kTextureVelocity);

	glDeleteFramebuffers(1, &kNoiseFBO);
	glDeleteTextures(1, &kTextureCurlNoise);
}

int main(int argc, char **argv) 
{
	srand((unsigned int)time(NULL));

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1280, 720);
	kMainWindow = glutCreateWindow("GPU Particles");
	//glutFullScreen();
	glewInit();

	cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;

	Init();
	GenerateParticles();
	GenerateNoise();

	glutMainLoop();

	Cleanup();
}