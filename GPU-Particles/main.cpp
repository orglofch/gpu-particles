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

#include "Utility\algebra.hpp"
#include "Utility\colour.hpp"
#include "Utility\gl.hpp"
#include "Utility\quaternion.hpp"

#include "flip_buffer.hpp"

using namespace std;

struct UpdateShader : public Shader
{
	Uniform position_uniform = -1;
	Uniform velocity_uniform = -1;
	Uniform normal_uniform = -1;

	Uniform curl_noise_uniform = -1;

	Uniform mouse_position_uniform = -1;
	Uniform mouse_down_uniform = -1;

	Uniform time_uniform = -1;

	Uniform particle_decay_uniform = -1;
	Uniform particle_lift_uniform = -1;
	Uniform particle_drag_uniform = -1;
};

struct RenderShader : public Shader
{
	Uniform position_uniform = -1;
	Uniform velocity_uniform = -1;
	Uniform normal_uniform = -1;
	Uniform shadow_map_uniform = -1;

	Uniform light_uniform = -1;

	Uniform life_fade = -1;

	Uniform global_ambient = -1;

	Uniform light_mvp_uniform = -1;
	Uniform light_bias_uniform = -1;
};

struct DepthShader : public Shader
{
	Uniform position_uniform = -1;
	Uniform velocity_uniform = -1;
};

struct InputState
{
	Vector2 mouse_position;
	bool left_mouse_down = false;

	bool rotate_left = false;
	bool rotate_right = false;

	bool zoom_in = false;
	bool zoom_out = false;
};

struct WindowState
{
	int main_window = 0;

	Vector2 window_size;
};

struct Light
{
	Vector3 position; // TODO(orglofch): Rename to translation
	Quaternion rotation;

	Colour colour;

	Matrix4x4 perpective;
};

struct SimulationState
{
	int time = 0;

	size_t particle_count = 2000 * 2000;

	float particle_decay = 0.000;
	float particle_lift = 0.0000;
	float particle_drag = 0.99;

	float rotation_y = 0.0f;
	float translation_z = -2.0f;

	bool curl_noise = false;
	bool paused = false;
	bool life_fade = true;
	bool shadow_map = true;

	std::vector<Light> lights;

	InputState input_state;

	WindowState window_state;

	Colour global_ambient;

	UpdateShader update_shader;
	RenderShader render_shader;
	Shader noise_shader;
	DepthShader depth_shader;

	FlipBuffer position_texture;
	FlipBuffer velocity_texture;
	FlipBuffer normal_texture;

	FlipBuffer frame_buffer;
};

SimulationState state;

size_t kTexWidth = (size_t)sqrt(state.particle_count);
size_t kTexHeight = kTexWidth;

// Textures
Texture kTextureColor = 0;
Texture kTextureCurlNoise = 0;
Texture kDepthTexture = 0;

// Buffers
GLuint kAttributeBuffer = 0;
GLuint kColorFBO = 0;
GLuint kNoiseFBO = 0;
GLuint kDepthFBO = 0;

void update() {
	if (state.input_state.rotate_left)
		state.rotation_y += 2.0f;
	if (state.input_state.rotate_right)
		state.rotation_y -= 2.0f;

	if (state.input_state.zoom_in)
		state.translation_z += 0.03f;
	if (state.input_state.zoom_out)
		state.translation_z -= 0.03f;

	//state.lights[0].rotation *= Quaternion(0.005, 0, 0, 1);

	if (state.paused) {
		return;
	}

	GL_CHECK();

	glBindFramebuffer(GL_FRAMEBUFFER, *state.frame_buffer.getInactiveBuffer());
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, (GLenum*)buffers);

	glViewport(0, 0, kTexWidth, kTexHeight);
	
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	GL_CHECK();

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, *state.position_texture.getActiveBuffer());
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, *state.velocity_texture.getActiveBuffer());
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, *state.normal_texture.getActiveBuffer());
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, kTextureCurlNoise);
	GL_CHECK();

	glUseShader(state.update_shader);
	GL_CHECK();

	GLfloat mouse_pos[2] = { state.input_state.mouse_position.x, state.input_state.mouse_position.y };
	glUniform2fv(state.update_shader.mouse_position_uniform, 1, mouse_pos);
	glUniform1f(state.update_shader.mouse_down_uniform, state.input_state.left_mouse_down);
	glUniform1f(state.update_shader.time_uniform, state.time++);
	glUniform1f(state.update_shader.curl_noise_uniform, state.curl_noise);
	glUniform1f(state.update_shader.particle_decay_uniform, state.particle_decay);
	glUniform1f(state.update_shader.particle_lift_uniform, state.particle_lift);
	glUniform1f(state.update_shader.particle_drag_uniform, state.particle_drag);
	GL_CHECK();

	// TODO(orglofch): This rebinds the active texture
	glDrawTexturedQuad(*state.position_texture.getActiveBuffer());
	GL_CHECK();

	glUseProgram(0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GL_CHECK();
}

void renderShadowMaps() {
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, kDepthFBO);

	// TODO(orglofch): Make the viewport the light resolution eventually
	glViewport(0, 0, 512, 512);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, *state.position_texture.getInactiveBuffer());
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, *state.velocity_texture.getInactiveBuffer());

	glBindBuffer(GL_ARRAY_BUFFER, kAttributeBuffer);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (char *)0);
	//glEnableVertexAttribArray(1);
	//glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (char *)2);
	GL_CHECK();

	glUseShader(state.depth_shader);

	Matrix4x4 lightMVPMat =
		state.lights[0].perpective
		* state.lights[0].rotation.unit().matrix()
		* Matrix4x4::translation(state.lights[0].position);

	// TODO(orglofch): Make this work for multiple light sources.
	for (Light &light : state.lights) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMultMatrixd(light.perpective.transpose().d);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixd((light.rotation.unit().matrix() * Matrix4x4::translation(light.position)).transpose().d);

		glPointSize(3);
		glDrawArrays(GL_POINTS, 0, state.particle_count);
	}

	glUseProgram(0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void render() {
	if (state.shadow_map) {
		renderShadowMaps();
	}

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	glViewport(0, 0, state.window_state.window_size.x, state.window_state.window_size.y);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glSetOrthographicProjection(0, state.window_state.window_size.x, 0, state.window_state.window_size.y, 0, 1);
	glUseProgram(0);
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, kDepthTexture);
	glDrawRect(0, 200, 0, 200, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	GL_CHECK();

	glSetPerspectiveProjection(80, state.window_state.window_size[0], state.window_state.window_size[1], 0.1, 10.0);
	glTranslatef(0.0, 0.0, state.translation_z);
	glRotatef(state.rotation_y, 0.0f, 1.0f, 0.0f);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, *state.position_texture.getInactiveBuffer());
	glActiveTexture(GL_TEXTURE2);
	// Make this state.velocity_texture with GL_LINES for cool effects!
	glBindTexture(GL_TEXTURE_2D, *state.velocity_texture.getInactiveBuffer());
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, *state.normal_texture.getInactiveBuffer());
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, kDepthTexture);
	GL_CHECK();

	glBindBuffer(GL_ARRAY_BUFFER, kAttributeBuffer);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat)* 3, (char *)0);
	//glEnableVertexAttribArray(1);
	//glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (char *)2);
	GL_CHECK();

	glUseShader(state.render_shader);

	Vector3 realLightPosition = state.lights[0].rotation.unit().matrix() * state.lights[0].position;

	GLfloat light[16] = { 
		255.0f, 255.0f, 255.0f, 1.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		realLightPosition.x, realLightPosition.y, realLightPosition.z, 0.0f,
	};

	Matrix4x4 lightMVPMat = 
		state.lights[0].perpective
		* state.lights[0].rotation.unit().matrix()
		* Matrix4x4::translation(state.lights[0].position);

	glUniform4fv(state.render_shader.light_uniform, 4, light);
	glUniform4fv(state.render_shader.global_ambient, 1, state.global_ambient.d);
	glUniform1f(state.render_shader.life_fade, state.life_fade);

	Matrix4x4 transposeMVP = lightMVPMat.transpose();
	GLfloat floatMVPMat[16];
	for (int i = 0; i < 16; ++i) {
		floatMVPMat[i] = transposeMVP.d[i];
	}
	glUniformMatrix4fv(state.render_shader.light_mvp_uniform, 1, GL_FALSE, floatMVPMat);

	GLfloat biasMatrix[] = {
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 0.5, 0.0,
		0.5, 0.5, 0.5, 1.0
	};
	glUniformMatrix4fv(state.render_shader.light_bias_uniform, 1, GL_FALSE, biasMatrix);

	glPointSize(3);
	glDrawArrays(GL_POINTS, 0, state.particle_count);

	glUseProgram(0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// TODO Render UI

	glutSwapBuffers();
	glutPostRedisplay();
	GL_CHECK();
}

void tick() {
	GL_CHECK();
	update();
	render();

	if (!state.paused) {
		state.position_texture.flip();
		state.velocity_texture.flip();
		state.normal_texture.flip();
		state.frame_buffer.flip();
	}
}

void updateMouseNormalized(int x, int y) {
	state.input_state.mouse_position[0] = 2.0f * x / state.window_state.window_size[0] - 1.0f;
	state.input_state.mouse_position[1] = -2.0f * y / state.window_state.window_size[1] + 1.0f;
}

void handleResize(int width, int height) {
	state.input_state.mouse_position[0] = width;
	state.input_state.mouse_position[1] = height;
}

void handleMouseMove(int x, int y) {
	updateMouseNormalized(x, y);
}

void handleMouseButton(int button, int button_state, int x, int y) {
	updateMouseNormalized(x, y);
	switch (button) {
		case GLUT_LEFT_BUTTON:
			state.input_state.left_mouse_down = (button_state == GLUT_DOWN);
			break;
		case GLUT_RIGHT_BUTTON:
			if (button_state == GLUT_DOWN) {
				state.paused = !state.paused;
			}
			break;
	}
}

void handlePressNormalKeys(unsigned char key, int x, int y) {
	switch (key) {
		case 'a':
		case 'A':
			state.input_state.rotate_left = true;
			break;
		case 'd':
		case 'D':
			state.input_state.rotate_right = true;
			break;
		case 'w':
		case 'W':
			state.input_state.zoom_in = true;
			break;
		case 's':
		case 'S':
			state.input_state.zoom_out = true;
			break;
		case 'q':
		case 'Q':
		case 27: 
			exit(EXIT_SUCCESS);
	}
}

void handleReleaseNormalKeys(unsigned char key, int x, int y) {
	switch (key) {
		case 'a':
		case 'A':
			state.input_state.rotate_left = false;
			break;
		case 'd':
		case 'D':
			state.input_state.rotate_right = false;
			break;
		case 'w':
		case 'W':
			state.input_state.zoom_in = false;
			break;
		case 's':
		case 'S':
			state.input_state.zoom_out = false;
			break;
	}
}

void handlePressSpecialKey(int key, int x, int y) {
	switch (key) {
		case GLUT_KEY_UP: 
		case GLUT_KEY_DOWN: 
			break;
	}
}

void handleReleaseSpecialKey(int key, int x, int y) {
	switch (key) {
		case GLUT_KEY_UP:
		case GLUT_KEY_DOWN:
			break;
	}
}

void init() {
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_NORMALIZE);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glSetPerspectiveProjection(60, state.window_state.window_size[0], state.window_state.window_size[1], 0.1, 1000.0);

	// Update shader.
	state.update_shader.program = glLoadShader("update.vert", "update.frag");
	state.update_shader.position_uniform = glGetUniform(state.update_shader, "positions");
	state.update_shader.velocity_uniform = glGetUniform(state.update_shader, "velocities");
	state.update_shader.normal_uniform = glGetUniform(state.update_shader, "normals");
	state.update_shader.curl_noise_uniform = glGetUniform(state.update_shader, "curl_noise");
	state.update_shader.mouse_position_uniform = glGetUniform(state.update_shader, "mouse_position");
	state.update_shader.mouse_down_uniform = glGetUniform(state.update_shader, "mouse_down");
	state.update_shader.time_uniform = glGetUniform(state.update_shader, "time");
	state.update_shader.particle_decay_uniform = glGetUniform(state.update_shader, "decay");
	state.update_shader.particle_lift_uniform = glGetUniform(state.update_shader, "lift");
	state.update_shader.particle_drag_uniform = glGetUniform(state.update_shader, "drag");
	GL_CHECK();

	glUseShader(state.update_shader);
	glUniform1i(state.update_shader.position_uniform, 1);
	glUniform1i(state.update_shader.velocity_uniform, 2);
	glUniform1i(state.update_shader.normal_uniform, 3);
	GL_CHECK();

	// Render shader.
	state.render_shader.program = glLoadShader("render.vert", "render.frag");

	state.render_shader.position_uniform = glGetUniform(state.render_shader, "positions");
	state.render_shader.velocity_uniform = glGetUniform(state.render_shader, "velocities");
	state.render_shader.normal_uniform = glGetUniform(state.render_shader, "normals");
	state.render_shader.light_uniform = glGetUniform(state.render_shader, "light");
	state.render_shader.life_fade = glGetUniform(state.render_shader, "life_fade");
	state.render_shader.global_ambient = glGetUniform(state.render_shader, "global_ambient");
	state.render_shader.shadow_map_uniform = glGetUniform(state.render_shader, "shadowMap");
	state.render_shader.light_mvp_uniform = glGetUniform(state.render_shader, "lightMVP");
	state.render_shader.light_bias_uniform = glGetUniform(state.render_shader, "lightBias");
	GL_CHECK();

	glUseProgram(state.render_shader.program);
	glUniform1i(state.render_shader.position_uniform, 1);
	glUniform1i(state.render_shader.velocity_uniform, 2);
	glUniform1i(state.render_shader.normal_uniform, 3);
	glUniform1i(state.render_shader.shadow_map_uniform, 4);
	GL_CHECK();

	// Noise shader.
	state.noise_shader.program = glLoadShader("noise.vert", "noise.frag");
	GL_CHECK();

	// Depth shader.
	state.depth_shader.program = glLoadShader("depth.vert", "depth.frag");

	state.depth_shader.position_uniform = glGetUniform(state.depth_shader, "positions");
	state.depth_shader.velocity_uniform = glGetUniform(state.depth_shader, "velocities");
	GL_CHECK();

	glUseProgram(state.depth_shader.program);
	glUniform1i(state.depth_shader.position_uniform, 1);
	glUniform1i(state.depth_shader.velocity_uniform, 2);
	GL_CHECK();

	glutDisplayFunc(tick);

	glutIgnoreKeyRepeat(1);
	glutMouseFunc(handleMouseButton);
	glutMotionFunc(handleMouseMove);
	glutPassiveMotionFunc(handleMouseMove);
	glutKeyboardFunc(handlePressNormalKeys);
	glutKeyboardUpFunc(handleReleaseNormalKeys);
	glutSpecialFunc(handlePressSpecialKey);
	glutSpecialUpFunc(handleReleaseSpecialKey);
	glutReshapeFunc(handleResize);

	GL_CHECK();
}

void generateNoise() {
	GLfloat *noiseData = new GLfloat[kTexWidth * kTexHeight * 4];
	for (size_t i = 0; i < kTexWidth * kTexHeight * 4; ++i) {
		noiseData[i] = 0.0f;
	}
	glCreateTexture2D(&kTextureCurlNoise, kTexWidth, kTexHeight, 4, noiseData);
	delete[] noiseData;

	glGenFramebuffers(1, &kNoiseFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, kNoiseFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, kTextureCurlNoise, 0);

	glUseShader(state.noise_shader);

	glBindFramebuffer(GL_FRAMEBUFFER, kNoiseFBO);
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, (GLenum*)buffers);
	glViewport(0, 0, kTexWidth, kTexHeight);
	glDisable(GL_BLEND);

	glDrawTexturedQuad(kTextureCurlNoise);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GL_CHECK();
}

void generateParticles() {
	int texture_bytes = kTexWidth * kTexHeight * 4;
	GLfloat *pData = new GLfloat[texture_bytes];
	GLfloat *vData = new GLfloat[texture_bytes];
	GLfloat *nData = new GLfloat[texture_bytes];
	for (size_t i = 0; i < texture_bytes; i += 4) {
		pData[i + 0] = 1.0f * rand() / RAND_MAX - 0.5f;
		pData[i + 1] = 1.0f * rand() / RAND_MAX - 0.5f;
		pData[i + 2] = 1.0f * rand() / RAND_MAX - 0.5f;
		pData[i + 3] = 1.0f;

		vData[i + 0] = 0.0f;
		vData[i + 1] = 0.0f;
		vData[i + 2] = 0.0f;
		vData[i + 3] = 0.0f;

		nData[i + 0] = pData[i + 0];
		nData[i + 1] = pData[i + 1];
		nData[i + 2] = pData[i + 2];
		nData[i + 3] = 0.0f;
	}
	glCreateTexture2D(state.position_texture.getActiveBuffer(), kTexWidth, kTexHeight, 4, pData);
	glCreateTexture2D(state.velocity_texture.getActiveBuffer(), kTexWidth, kTexHeight, 4, vData);
	glCreateTexture2D(state.normal_texture.getActiveBuffer(), kTexWidth, kTexHeight, 4, nData);

	glGenFramebuffers(2, state.frame_buffer.getBuffers());

	// TODO(orglofch): Move this into flip buffer
	glBindFramebuffer(GL_FRAMEBUFFER, *state.frame_buffer.getActiveBuffer());
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, *state.position_texture.getActiveBuffer(), 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
		GL_TEXTURE_2D, *state.velocity_texture.getActiveBuffer(), 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
		GL_TEXTURE_2D, *state.normal_texture.getActiveBuffer(), 0);
	GL_CHECK();

	for (size_t i = 0; i < texture_bytes; ++i) {
		pData[i] = 0.0f;
		vData[i] = 0.0f;
		nData[i] = 0.0f;
	}
	glCreateTexture2D(state.position_texture.getInactiveBuffer(), kTexWidth, kTexHeight, 4, pData);
	glCreateTexture2D(state.velocity_texture.getInactiveBuffer(), kTexWidth, kTexHeight, 4, vData);
	glCreateTexture2D(state.normal_texture.getInactiveBuffer(), kTexWidth, kTexHeight, 4, nData);

	glBindFramebuffer(GL_FRAMEBUFFER, *state.frame_buffer.getInactiveBuffer());
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, *state.position_texture.getInactiveBuffer(), 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
		GL_TEXTURE_2D, *state.velocity_texture.getInactiveBuffer(), 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
		GL_TEXTURE_2D, *state.normal_texture.getInactiveBuffer(), 0);
	GL_CHECK();

	delete[] pData;
	delete[] vData;

	// Create dummy VBO
	GLfloat *attributeData = new GLfloat[kTexWidth * kTexHeight * 2]; // *6 and elsewhere
	for (size_t x = 0; x < kTexWidth; ++x) {
		for (size_t y = 0; y < kTexHeight; ++y) {
			size_t i = (y * kTexWidth + x) * 2;
			attributeData[i + 0] = 1.0f * (x + 0.5f) / kTexWidth; // s
			attributeData[i + 1] = 1.0f * (y + 0.5f) / kTexHeight; // t
			//attributeData[i + 3] = 1.0f * (x + 0.5f) / kTexWidth; // s
			//attributeData[i + 4] = 1.0f * (y + 0.5f) / kTexHeight; // t
			//attributeData[i + 2] = 1.0f; // head of line
			//attributeData[i + 5] = 0.0f; // not head of line
		}
	}
	GL_CHECK();

	glGenBuffers(1, &kAttributeBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, kAttributeBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* kTexWidth * kTexHeight * 2, 
		attributeData, GL_STATIC_DRAW);
	delete[] attributeData;
	GL_CHECK();
}

void generateColorBuffers() {
	int buffer_bytes = state.window_state.window_size[0] * state.window_state.window_size[1] * 4;
	GLfloat *colorData = new GLfloat[buffer_bytes];
	for (size_t i = 0; i < buffer_bytes; ++i) {
		colorData[i] = 255;
	}

	glCreateTexture2D(&kTextureCurlNoise, state.window_state.window_size[0],
		state.window_state.window_size[1], 4, colorData);
	delete[] colorData;

	glGenFramebuffers(1, &kColorFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, kColorFBO);
	GL_CHECK();
}

void generateDepthBuffer() {
	// TODO(orglofch): Decrease this resolution
	int buffer_bytes = 512 * 512;
	GLfloat *depthData = new GLfloat[buffer_bytes];
	for (size_t i = 0; i < buffer_bytes; ++i) {
		depthData[i] = 0.0f;
	}
	glGenTextures(1, &kDepthTexture);
	glBindTexture(GL_TEXTURE_2D, kDepthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16,
		512, 512, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);

	glGenFramebuffers(1, &kDepthFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, kDepthFBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, kDepthTexture, 0);
	glDrawBuffer(GL_NONE);
	GL_CHECK();
}

void cleanup() {
	glDeleteBuffers(1, &kAttributeBuffer);

	glDeleteFramebuffers(2, state.frame_buffer.getBuffers());
	glDeleteFramebuffers(1, &kDepthFBO);

	glDeleteTextures(2, state.position_texture.getBuffers());
	glDeleteTextures(2, state.velocity_texture.getBuffers());
	glDeleteTextures(2, state.normal_texture.getBuffers());
	glDeleteTextures(1, &kDepthTexture);

	glDeleteFramebuffers(1, &kNoiseFBO);
	glDeleteTextures(1, &kTextureCurlNoise);
}

int main(int argc, char **argv) 
{
	srand((unsigned int)time(NULL));

	state.window_state.window_size[0] = 1080;
	state.window_state.window_size[1] = 680;

	Light light;
	light.position.z = -5;
	light.perpective = Matrix4x4::orthographic(-3, 3, -3, 3, -6, 6);
	state.lights.push_back(light);

	state.global_ambient = { 1.0f, 0.6f, 0.3f, 0.05f };

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(state.window_state.window_size[0], state.window_state.window_size[1]);
	state.window_state.main_window = glutCreateWindow("GPU Particles");
	//glutFullScreen();
	glewInit();

	cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;

	init();
	generateDepthBuffer();
	generateParticles();
	generateNoise();
	generateColorBuffers();

	glutMainLoop();

	cleanup();
}