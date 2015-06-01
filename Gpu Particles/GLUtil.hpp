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

#ifndef _GL_UTIL_HPP_
#define _GL_UTIL_HPP_

#include <cassert>
#include <fstream>
#include <GL\glew.h>
#include <GL\freeglut.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

void CheckOpenGLError(const char* stmt, const char* fname, int line)
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		printf("OpenGL error %08x, at %s:%i - for %s\n", err, fname, line, stmt);
	}
}

#ifdef _DEBUG
#define GL_CHECK(stmt) do { \
	stmt; \
	CheckOpenGLError(#stmt, __FILE__, __LINE__); \
} while (0)
#else
#define GL_CHECK(stmt) stmt
#endif

void CreateTexture2D(GLuint *texture, size_t width, size_t height, GLfloat *data)
{
	assert(data);
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, *texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA,
		GL_FLOAT, data);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void CreateTexture3D(GLuint *texture, size_t width, size_t height, size_t depth, GLfloat *data) {
	assert(data);
	glEnable(GL_TEXTURE_3D);
	glGenTextures(1, texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, *texture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, width, height, depth, 0, 
		GL_RGBA, GL_FLOAT, data);
	glBindTexture(GL_TEXTURE_3D, 0);
}

void DrawTexturedQuad(GLuint texture)
{
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0); glVertex3f(-1.0, -1.0, -10.0);
		glTexCoord2f(0.0, 1.0); glVertex3f(1.0, -1.0, -10.0);
		glTexCoord2f(1.0, 1.0); glVertex3f(1.0, 1.0, -10.0);
		glTexCoord2f(1.0, 0.0); glVertex3f(-1.0, 1.0, -10.0);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
}

void SetPerspectiveProjection(GLdouble fov, size_t width, size_t height, GLdouble n, GLdouble f)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	float aspect = 1.0f * width / height;
	gluPerspective(fov, 1.0 * width / height, n, f);
	
	glMatrixMode(GL_MODELVIEW);
}

void SetOrthographicProjection(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble n, GLdouble f)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(left, right, bottom, top, n, f);

	glMatrixMode(GL_MODELVIEW);
}

std::string loadFile(const char *filename)
{
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cout << "Unable to open file " << filename << std::endl;
	}

	std::stringstream fileData;
	fileData << file.rdbuf();
	file.close();

	return fileData.str();
}

void printShaderInfo(GLint shader)
{
	int infoLogLen = 0;
	int charsWritten = 0;
	GLchar *infoLog;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

	if (infoLogLen > 0) {
		infoLog = new GLchar[infoLogLen];
		glGetShaderInfoLog(shader, infoLogLen, &charsWritten, infoLog);
		std::cout << "Info: " << std::endl << infoLog << std::endl;
		delete[] infoLog;
	}
}

bool LoadShader(const char *vsFilename, 
	const char *fsFilename, 
	GLuint &shaderProgram, 
	GLuint &vertexShader, 
	GLuint &fragmentShader)
{
	shaderProgram = 0;
	vertexShader = 0;
	fragmentShader = 0;

	std::string vertexShaderString = loadFile(vsFilename);
	std::string fragmentShaderString = loadFile(fsFilename);
	int vlen = vertexShaderString.length();
	int flen = fragmentShaderString.length();

	if (vertexShaderString.empty())
		return false;
	if (fragmentShaderString.empty())
		return false;

	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	const char *vertexShaderCStr = vertexShaderString.c_str();
	const char *fragmentShaderCStr = fragmentShaderString.c_str();
	glShaderSource(vertexShader, 1, (const GLchar **)&vertexShaderCStr, &vlen);
	glShaderSource(fragmentShader, 1, (const GLchar **)&fragmentShaderCStr, &flen);

	GLint compiled;

	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compiled);
	if (compiled == false) {
		std::cout << "Vertex shader not compiled." << std::endl;
		printShaderInfo(vertexShader);

		glDeleteShader(vertexShader);
		vertexShader = 0;
		glDeleteShader(fragmentShader);
		fragmentShader = 0;

		return false;
	}

	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compiled);
	if (compiled == false) {
		std::cout << "Fragment shader not compiled." << std::endl;
		printShaderInfo(fragmentShader);

		glDeleteShader(vertexShader);
		vertexShader = 0;
		glDeleteShader(fragmentShader);
		fragmentShader = 0;

		return false;
	}

	shaderProgram = glCreateProgram();

	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	glLinkProgram(shaderProgram);

	GLint IsLinked;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, (GLint *)&IsLinked);
	if (IsLinked == false) {
		std::cout << "Failed to link shader." << std::endl;

		GLint maxLength;
		glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &maxLength);
		if (maxLength>0) {
			char *pLinkInfoLog = new char[maxLength];
			glGetProgramInfoLog(shaderProgram, maxLength, &maxLength, pLinkInfoLog);
			std::cout << pLinkInfoLog << std::endl;
			delete[] pLinkInfoLog;
		}

		glDetachShader(shaderProgram, vertexShader);
		glDetachShader(shaderProgram, fragmentShader);
		glDeleteShader(vertexShader);
		vertexShader = 0;
		glDeleteShader(fragmentShader);
		fragmentShader = 0;
		glDeleteProgram(shaderProgram);
		shaderProgram = 0;

		return false;
	}

	return true;
}

struct Vertex {
	float x, y, z;
	float nx, ny, nz;
	float s0, t0;
};

/*void LoadObj(const std::string &filename) {
	GLuint vertexVBO;
	GLuint indexVBO;

	unsigned int vertex_count = m_verts.size();
	unsigned int index_count = m_faces.size();
	// Create array buffer, faces should be triangular
	Vertex *vertices = new Vertex[vertex_count];
	TriIndex *indices = new TriIndex[index_count];
	for (unsigned int i = 0; i < vertex_count; ++i) {
		Point3D vertex = m_verts.at(i);
		Vector3D normal = m_vert_norms.at(i);
		Point2D tex = m_texs.at(i);
		vertices[i].x = vertex[0];
		vertices[i].y = vertex[1];
		vertices[i].z = vertex[2];
		vertices[i].nx = normal[0];
		vertices[i].ny = normal[1];
		vertices[i].nz = normal[2];
		vertices[i].s0 = tex[0];
		vertices[i].t0 = tex[1];
	}
	for (unsigned int i = 0; i < index_count; ++i) {
		Face face = m_faces.at(i);
		indices[i].i1 = face.vertices.at(0) - 1;
		indices[i].i2 = face.vertices.at(1) - 1;
		indices[i].i3 = face.vertices.at(2) - 1;
	}

	glGenBuffers(1, &vertexVBO);
	glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
	glBufferData(GL_ARRAY_BUFFER, vertex_count*sizeof(CVert), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &indexVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count*sizeof(TriIndex), indices,
		GL_STATIC_DRAW);

	delete[] vertices;
	delete[] indices;

	std::vector<float[3]> vertices;
	std::vector<float[2]> textures;
	std::vector<float[3]> normals;

	std::vector<std::vector<int> > faces;

	std::ifstream ifs(filename);

	std::string lineHeader;
	while (ifs >> lineHeader) {
		if (lineHeader.compare("v") == 0) { // Vertex
			float vertex[3];
			ifs >> vertex[0] >> vertex[1] >> vertex[2];
			vertices.push_back(vertex);
		} else if (lineHeader.compare("vt") == 0) { // Texture
			float texture[2];
			ifs >> texture[1] >> texture[2];
			textures.push_back(texture);
		} else if (lineHeader.compare("vn") == 0) { // Normal
			float normal[3];
			ifs >> normal[0] >> normal[1] >> normal[2];
			normals.push_back(normal);
		} else if (lineHeader.compare("f") == 0) { // Face
			std::vector<int> face;
			char line[256];
			char delimeter;
			ifs.getline(line, 256);
			std::istringstream iss(line);
			int vertex, texture, normal;
			while (iss) {
				iss >> vertex >> delimeter;
				iss >> texture;
				if (!iss) { // There may be no texture
					iss.clear();
				}
				iss >> delimeter >> normal;

				face.vertices.push_back(vertex);
				face.textures.push_back(texture);
				face.normals.push_back(normal);
			}

			faces.push_back(face);
			face.vertices.clear();
			face.textures.clear();
			face.normals.clear();
		}
	}
	Mesh *mesh = new Mesh(vertices, normals, textures, faces);
	mMeshes.insert(std::pair<std::string, Mesh*>(filename, mesh));
	return mesh;

}*/

#endif