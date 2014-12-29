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

// Simulation state
size_t kNumParticles = 800 * 800;
size_t kTexWidth = sqrt(kNumParticles);
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
GLint kMousePositionUniformLocationUpdate = -1;
GLint kMouseDownUniformLocationUpdate = -1;

GLint kPositionUniformLocationRender = -1;
GLint kVelocityUniformLocationRender = -1;

GLint kPermUniformLocationNoise = -1;

// Attributes
GLint kIndexAttributeLocationUpdate = -1;
GLint kIndexAttributeLocationRender = -1;

// Textures
GLuint kTexturePosition[2] = { 0, 0 };
GLuint kTextureVelocity[2] = { 0, 0 };
GLuint kPermTexture = 0;
GLuint kNoiseTexture = 0;
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

// Window state
size_t kWindowSize[2] = { 0.0, 0.0 };
int kMainWindow = 0;

int perm[256] = { 
	151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 
	103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 
	26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 
	87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166, 77, 
	146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
	102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196,
	135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
	5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
	223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
	129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
	251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107,
	49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
	138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180 
};

int grad3[16][3] = { 
	{ 0, 1, 1 }, { 0, 1, -1 }, { 0, -1, 1 }, { 0, -1, -1 },
	{ 1, 0, 1 }, { 1, 0, -1 }, { -1, 0, 1 }, { -1, 0, -1 },
	{ 1, 1, 0 }, { 1, -1, 0 }, { -1, 1, 0 }, { -1, -1, 0 }, 
	{ 1, 0, -1 }, { -1, 0, -1 }, { 0, -1, 1 }, { 0, 1, 1 } 
};

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

	//glUseProgram(0);
	//DrawTexturedQuad(kNoiseTexture);

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

	LoadShader("noise.vert", "noise.frag",
		kNoiseShaderProgram, kNoiseVertexShader, kNoiseFragmentShader);

	kPermUniformLocationNoise = glGetUniformLocation(kNoiseShaderProgram, "uPermTexture");

	glutDisplayFunc(Render);
	glutReshapeFunc(Resize);

	glutIgnoreKeyRepeat(1);
	glutMouseFunc(HandleMouseButton);
	glutMotionFunc(HandleMouseMove);
	glutKeyboardFunc(HandlePressNormalKeys);
	glutKeyboardUpFunc(HandleReleaseNormalKeys);
	glutSpecialFunc(HandlePressSpecialKey);
	glutSpecialUpFunc(HandleReleaseSpecialKey);
}

void GenerateNoise(void)
{
	glUseProgram(kNoiseShaderProgram);

	glBindFramebuffer(GL_FRAMEBUFFER, kNoiseFBO);
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, (GLenum*)buffers);
	glViewport(0, 0, kTexWidth, kTexHeight);
	glDisable(GL_BLEND);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, kPermTexture);
	glUniform1i(kPermUniformLocationNoise, 0);

	DrawTexturedQuad(kPermTexture);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* kTexWidth * kTexHeight * 6, 
		attributeData, GL_STATIC_DRAW);
	delete[] attributeData;

	// Create noise
	GLbyte *permData = new GLbyte[kTexWidth * kTexHeight * 4];
	for (int x = 0; x < 256; ++x) {
		for (int y = 0; y < 256; ++y) {
			int i = (x * 256 + y) * 4;
			char value = perm[(y + perm[x]) & 0xFF];
			permData[i + 0] = grad3[value & 0x0F][0] * 64 + 64;
			permData[i + 1] = grad3[value & 0x0F][1] * 64 + 64;
			permData[i + 2] = grad3[value & 0x0F][2] * 64 + 64;
			permData[i + 3] = value;
		}
	}

	glGenTextures(1, &kPermTexture);
	glBindTexture(GL_TEXTURE_2D, kPermTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kTexWidth, kTexHeight, 0, GL_RGBA, 
		GL_UNSIGNED_BYTE, permData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	delete[] permData;

	GLbyte *noiseData = new GLbyte[kTexWidth * kTexHeight * 4];
	for (int i = 0; i < kTexWidth * kTexHeight * 4; ++i) {
		noiseData[i] = 0;
	}
	glGenTextures(1, &kNoiseTexture);
	glBindTexture(GL_TEXTURE_2D, kNoiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kTexWidth, kTexHeight, 0, GL_RGBA,
		GL_UNSIGNED_BYTE, noiseData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	delete[] noiseData;

	glBindFramebuffer(GL_FRAMEBUFFER, kNoiseFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, kNoiseTexture, 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Cleanup(void)
{
	glDeleteBuffers(1, &kAttributeBuffer);
	glDeleteFramebuffers(2, kFBO);
	glDeleteTextures(2, kTexturePosition);
	glDeleteTextures(2, kTextureVelocity);
	glDeleteTextures(1, &kPermTexture);
	glDeleteTextures(1, &kNoiseTexture);
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
	GenerateNoise();
	GenerateParticles();

	glutMainLoop();

	Cleanup();
}