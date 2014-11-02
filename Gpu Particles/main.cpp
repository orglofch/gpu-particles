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

size_t kNumParticles = 900 * 900;
size_t kTexWidth = sqrt(kNumParticles);
size_t kTexHeight = kTexWidth;

// Shaders
GLuint kUpdateShaderProgram = 0;
GLuint kUpdateVertexShader = 0;
GLuint kUpdateFragmentShader = 0;

GLuint kRenderShaderProgram = 0;
GLuint kRenderVertexShader = 0;
GLuint kRenderFragmentShader = 0;

// Uniforms
GLint kPositionUniformLocationUpdate = -1;
GLint kVelocityUniformLocationUpdate = -1;
GLint kMousePositionUniformLocationUpdate = -1;
GLint kMouseDownUniformLocationUpdate = -1;

GLint kPositionUniformLocationRender = -1;
GLint kVelocityUniformLocationRender = -1;

// Attributes
GLint kIndexAttributeLocationUpdate = -1;
GLint kIndexAttributeLocationRender = -1;

// Textures
GLuint kTexturePosition[2] = { 0, 0 };
GLuint kTextureVelocity[2] = { 0, 0 };
int kFrontTexture = 0;

// Buffers
GLuint kAttributeBuffer = 0;
GLuint kFBO[2] = { 0, 0 };

// Button state
GLfloat kMouse[2] = { 0.0, 0.0 };
bool kLeftMouseDown = false;
bool kRightMouseDown = false;

size_t kWindowSize[2] = { 0.0, 0.0 };
int kMainWindow = 0;

void Resize(int width, int height)
{
	kWindowSize[0] = width;
	kWindowSize[1] = height;
	SetPerspectiveProjection(60, kWindowSize[0], kWindowSize[1], 0.1, 1000.0);
}

void Update(void)
{
	glUseProgram(kUpdateShaderProgram);

	glBindFramebuffer(GL_FRAMEBUFFER, kFBO[!kFrontTexture]);
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, (GLenum*)buffers);
	glViewport(0, 0, kTexWidth, kTexHeight);
	glDisable(GL_BLEND);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, kTexturePosition[kFrontTexture]);
	glUniform1i(kPositionUniformLocationUpdate, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, kTextureVelocity[kFrontTexture]);
	glUniform1i(kVelocityUniformLocationUpdate, 1);

	glUniform2fv(kMousePositionUniformLocationUpdate, 1, kMouse);
	glUniform1f(kMouseDownUniformLocationUpdate, kLeftMouseDown);

	DrawTexturedQuad(kTextureVelocity[kFrontTexture]);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Render(void) 
{
	// Paused
	if (!kRightMouseDown) {
		Update();
	}
	glEnable(GL_BLEND);

	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, kWindowSize[0], kWindowSize[1]);

	glUseProgram(kRenderShaderProgram);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, kTexturePosition[!kFrontTexture]);
	glUniform1i(kPositionUniformLocationRender, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, kTextureVelocity[!kFrontTexture]);
	glUniform1i(kVelocityUniformLocationRender, 1);

	glBindBuffer(GL_ARRAY_BUFFER, kAttributeBuffer);
	glEnableVertexAttribArray(0);
	GL_CHECK(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat)* 3, (char *)0));
	glEnableVertexAttribArray(1);
	GL_CHECK(glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (char *)2));

	glDrawArrays(GL_LINES, 0, kNumParticles);

	glutSwapBuffers();
	glutPostRedisplay();

	// Paused
	if (!kRightMouseDown) {
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
	switch (button) {
		case GLUT_LEFT_BUTTON:
			kLeftMouseDown = (state == GLUT_DOWN);
			break;
		case GLUT_RIGHT_BUTTON:
			kRightMouseDown = (state == GLUT_DOWN);
			kFrontTexture = !kFrontTexture;
			break;
	}
}

void HandleNormalKeys(unsigned char key, int x, int y) 
{
	switch (key) 
	{
		case 'q':
		case 'Q':
		case 27: 
			exit(EXIT_SUCCESS);
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
	glBlendFunc(GL_ONE, GL_ONE);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_NORMALIZE);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	SetPerspectiveProjection(60, kWindowSize[0], kWindowSize[1], 0.1, 1000.0);

	LoadShader("update.vert", "update.frag",
		kUpdateShaderProgram, kUpdateVertexShader, kUpdateFragmentShader);
	kPositionUniformLocationUpdate = glGetUniformLocation(kUpdateShaderProgram, "uPositions");
	kVelocityUniformLocationUpdate = glGetUniformLocation(kUpdateShaderProgram, "uVelocities");
	kMousePositionUniformLocationUpdate = glGetUniformLocation(kUpdateShaderProgram, "uMousePosition");
	kMouseDownUniformLocationUpdate = glGetUniformLocation(kUpdateShaderProgram, "uMouseDown");
	kIndexAttributeLocationUpdate = glGetAttribLocation(kUpdateShaderProgram, "aIndex");

	LoadShader("render.vert", "render.frag", 
		kRenderShaderProgram, kRenderVertexShader, kRenderFragmentShader);

	kPositionUniformLocationRender = glGetUniformLocation(kRenderShaderProgram, "uPositions");
	kVelocityUniformLocationRender = glGetUniformLocation(kRenderShaderProgram, "uVelocities");
	kIndexAttributeLocationRender = glGetAttribLocation(kRenderShaderProgram, "aIndex");

	glutDisplayFunc(Render);
	glutReshapeFunc(Resize);

	glutIgnoreKeyRepeat(1);
	glutMouseFunc(HandleMouseButton);
	glutMotionFunc(HandleMouseMove);
	glutKeyboardFunc(HandleNormalKeys);
	glutSpecialFunc(HandlePressSpecialKey);
	glutSpecialUpFunc(HandleReleaseSpecialKey);
}

void GenerateParticles(void)
{
	GLfloat *pData = new GLfloat[kTexWidth * kTexHeight * 4];
	GLfloat *vData = new GLfloat[kTexWidth * kTexHeight * 4];
	for (size_t x = 0; x < kTexWidth; ++x) {
		for (size_t y = 0; y < kTexHeight; ++y) {
			size_t i = (y * kTexWidth + x) * 4;

			pData[i + 0] = 1.0f * rand() / RAND_MAX * 2.0f - 1.0f;
			pData[i + 1] = 1.0f * rand() / RAND_MAX * 2.0f - 1.0f;
			pData[i + 2] = 1.0f * rand() / RAND_MAX * 2.0f - 1.0f;
			pData[i + 3] = 1.0f;

			vData[i + 0] = 0.0f;
			vData[i + 1] = 0.0f;
			vData[i + 2] = 0.0f;
			vData[i + 3] = 0.0f;
		}
	}
	CreateTexture(&kTexturePosition[0], kTexWidth, kTexHeight, pData);
	CreateTexture(&kTextureVelocity[0], kTexWidth, kTexHeight, vData);

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
	CreateTexture(&kTexturePosition[1], kTexWidth, kTexHeight, pData);
	CreateTexture(&kTextureVelocity[1], kTexWidth, kTexHeight, vData);

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
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* kTexWidth * kTexHeight * 6, attributeData, GL_STATIC_DRAW);
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
}

int main(int argc, char **argv) 
{
	srand((unsigned int)time(NULL));

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1200, 800);
	kMainWindow = glutCreateWindow("GPU Particles");
	glewInit();

	std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

	Init();
	GenerateParticles();

	glutMainLoop();

	Cleanup();
}