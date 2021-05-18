#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include<math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textfile.h"

#include "Vectors.h"
#include "Matrices.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

using namespace std;

// Default window size
int WINDOW_WIDTH = 800;
int WINDOW_HEIGHT = 800;

bool mouse_pressed = false;
int starting_press_x = -1;
int starting_press_y = -1;

enum TransMode
{
	GeoTranslation = 0,
	GeoRotation = 1,
	GeoScaling = 2,
	ViewCenter = 3,
	ViewEye = 4,
	ViewUp = 5,
	LightPositionChange = 6,
	ShineeShange = 7
};


GLint iLocModel;
GLint iLocView;
GLint iLocProjection;

GLint iLocLightPosition;
GLint iLocLightDiffuse;
GLint iLocCutoffAngle;
GLint iLocShininess;

GLint iLocKa;
GLint iLocKd;
GLint iLocKs;
GLint iLocShadingType;
GLint iLocLightMode;
GLint iLocViewPos;

GLint cur_LightMode;
GLfloat Shininess;

struct LightDefaultInfo {
	Vector3 LightPosition;
	Vector3 LightDiffuse;
	GLfloat  Angle;
	Vector3 Direction;
} ;
LightDefaultInfo directionalInfo;
LightDefaultInfo pointInfo;
LightDefaultInfo spotInfo;


vector<string> filenames; // .obj filename list

struct PhongMaterial
{
	Vector3 Ka;
	Vector3 Kd;
	Vector3 Ks;
};

typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	GLuint p_color;
	int vertex_count;
	GLuint p_normal;
	PhongMaterial material;
	int indexCount;
	GLuint m_texture;
} Shape;

struct model
{
	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);	// Euler form
	vector<Shape> shapes;
};
vector<model> models;

struct camera
{
	Vector3 position;
	Vector3 center;
	Vector3 up_vector;
};
camera main_camera;

struct project_setting
{
	GLfloat nearClip, farClip;
	GLfloat fovy;
	GLfloat aspect;
	GLfloat left, right, top, bottom;
};
project_setting proj;

enum ProjMode
{
	Orthogonal = 0,
	Perspective = 1,
};
ProjMode cur_proj_mode = Orthogonal;
TransMode cur_trans_mode = GeoTranslation;

Matrix4 view_matrix;
Matrix4 project_matrix;


Shape quad;
Shape m_shpae;
int cur_idx = 0; // represent which model should be rendered now


static GLvoid Normalize(GLfloat v[3])
{
	GLfloat l;

	l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] /= l;
	v[1] /= l;
	v[2] /= l;
}

static GLvoid Cross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{

	n[0] = u[1] * v[2] - u[2] * v[1];
	n[1] = u[2] * v[0] - u[0] * v[2];
	n[2] = u[0] * v[1] - u[1] * v[0];
}


// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
	Matrix4 mat;

	mat = Matrix4(
		1, 0, 0, vec[0],
		0, 1, 0, vec[1],
		0, 0, 1, vec[2],
		0, 0, 0, 1
	);

	return mat;
}

// [TODO] given a scaling vector then output a Matrix4 (Scaling Matrix)
Matrix4 scaling(Vector3 vec)
{
	Matrix4 mat;

	mat = Matrix4(
		vec[0], 0, 0, 0,
		0, vec[1], 0, 0,
		0, 0, vec[2], 0,
		0, 0, 0, 1
	);

	return mat;
}


// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
	Matrix4 mat;


	mat = Matrix4(
		1, 0, 0, 0,
		0, cos(val), -sin(val), 0,
		0, sin(val), cos(val), 0,
		0, 0, 0, 1
	);


	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(
		cos(val), 0, sin(val), 0,
		0, 1, 0, 0,
		-sin(val), 0, cos(val), 0,
		0, 0, 0, 1
	);


	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(
		cos(val), -sin(val), 0, 0,
		sin(val), cos(val), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);


	return mat;
}

Matrix4 rotate(Vector3 vec)
{
	return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}

// [TODO] compute viewing matrix accroding to the setting of main_camera
void setViewingMatrix()
{
	Vector3 p1p2, p1p3;
	p1p2 = (main_camera.center - main_camera.position);
	p1p3 = (main_camera.up_vector - main_camera.position);

	Vector3 Rz = -p1p2 / p1p2.length();
	Vector3 Rx = p1p2.cross(p1p3) / p1p2.cross(p1p3).length();
	Vector3 Ry = Rz.cross(Rx);

	view_matrix = Matrix4(
		Rx[0], Rx[1], Rx[2], 0,
		Ry[0], Ry[1], Ry[2], 0,
		Rz[0], Rz[1], Rz[2], 0,
		0, 0, 0, 1
	);


	Matrix4 T = Matrix4(
		1, 0, 0, -main_camera.position.x,
		0, 1, 0, -main_camera.position.y,
		0, 0, 1, -main_camera.position.z,
		0, 0, 0, 1
	);


	view_matrix = view_matrix * T ;

}

// [TODO] compute orthogonal projection matrix
void setOrthogonal()
{
	cur_proj_mode = Orthogonal;
	// project_matrix [...] = ...
	project_matrix = Matrix4(
		2 / (proj.right - proj.left), 0, 0, -(proj.right + proj.left) / (proj.right - proj.left),
		0, 2 / (proj.top - proj.bottom), 0, -(proj.top + proj.bottom) / (proj.top - proj.bottom),
		0, 0, -2 / (proj.farClip - proj.nearClip), -(proj.farClip + proj.nearClip) / (proj.farClip - proj.nearClip),
		0, 0, 0, 1
	);
}

float cot(float x)
{
	return 1 / tan(x * 3.14159265358979323846 / 180.0);
}

// [TODO] compute persepective projection matrix
void setPerspective()
{
	cur_proj_mode = Perspective;
	// project_matrix [...] = ...
	GLfloat f = cot((float)proj.fovy / 2);
	project_matrix = Matrix4(
		f/ proj.aspect , 0, 0, 0,	
		0, f, 0, 0,
		0, 0, (proj.farClip ) / (proj.nearClip - proj.farClip), 2*(proj.farClip * proj.nearClip) / (proj.nearClip - proj.farClip),
		0, 0, -1, 0
	);

}


// Vertex buffers
GLuint VAO, VBO;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{

	WINDOW_WIDTH = width;
	WINDOW_HEIGHT = height;


// [TODO] change your aspect ratio
	proj.aspect = (float)width/2 / height;



	setPerspective();
	setViewingMatrix();
}

// refresh input parameter
void refresh_memorydata( int shapesNum, int ShadingType) {

	glUniform3f(iLocViewPos, main_camera.position[0], main_camera.position[1], main_camera.position[2]);
	glUniform3f(iLocKa, models[cur_idx].shapes[shapesNum].material.Ka[0], models[cur_idx].shapes[shapesNum].material.Ka[1], models[cur_idx].shapes[shapesNum].material.Ka[2]);
	glUniform3f(iLocKd, models[cur_idx].shapes[shapesNum].material.Kd[0], models[cur_idx].shapes[shapesNum].material.Kd[1], models[cur_idx].shapes[shapesNum].material.Kd[2]);
	glUniform3f(iLocKs, models[cur_idx].shapes[shapesNum].material.Ks[0], models[cur_idx].shapes[shapesNum].material.Ks[1], models[cur_idx].shapes[shapesNum].material.Ks[2]);
	glUniform1i(iLocShadingType, ShadingType);
	glUniform1i(iLocLightMode, cur_LightMode);
	glUniform1f(iLocShininess, Shininess);

	if (cur_LightMode == 0) {
		glUniform3f(iLocLightPosition, directionalInfo.LightPosition[0], directionalInfo.LightPosition[1], directionalInfo.LightPosition[2]);
		glUniform3f(iLocLightDiffuse, directionalInfo.LightDiffuse[0], directionalInfo.LightDiffuse[1], directionalInfo.LightDiffuse[2]);
	}
	if (cur_LightMode == 1) {
		glUniform3f(iLocLightPosition, pointInfo.LightPosition[0], pointInfo.LightPosition[1], pointInfo.LightPosition[2]);
		glUniform3f(iLocLightDiffuse, pointInfo.LightDiffuse[0], pointInfo.LightDiffuse[1], pointInfo.LightDiffuse[2]);
	}
	if (cur_LightMode == 2) {
		glUniform3f(iLocLightPosition, spotInfo.LightPosition[0], spotInfo.LightPosition[1], spotInfo.LightPosition[2]);
		glUniform3f(iLocLightDiffuse, spotInfo.LightDiffuse[0], spotInfo.LightDiffuse[1], spotInfo.LightDiffuse[2]);
		glUniform1f(iLocCutoffAngle, spotInfo.Angle);

	}
}


// Render function for display rendering
void RenderScene(void) {	
	// clear canvas
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	Matrix4 T, R, S;
	// [TODO] update translation, rotation and scaling
	T = translate(models[cur_idx].position);
	R = rotate(models[cur_idx].rotation);
	S = scaling(models[cur_idx].scale);

	Matrix4 MVP;
	GLfloat mvp[16];

	// [TODO] multiply all the matrix

	Matrix4 M;
	GLfloat m[16];
	M = (S * R * T);
	m[0] = M[0];  m[4] = M[1];   m[8] = M[2];    m[12] = M[3];
	m[1] = M[4];  m[5] = M[5];   m[9] = M[6];    m[13] = M[7];
	m[2] = M[8];  m[6] = M[9];   m[10] = M[10];  m[14] = M[11];
	m[3] = M[12]; m[7] = M[13];  m[11] = M[14];  m[15] = M[15];

	Matrix4 V;
	GLfloat v[16];

	V =  view_matrix ;
	v[0] = V[0];  v[4] = V[1];   v[8] = V[2];    v[12] = V[3];
	v[1] = V[4];  v[5] = V[5];   v[9] = V[6];    v[13] = V[7];
	v[2] = V[8];  v[6] = V[9];   v[10] = V[10];  v[14] = V[11];
	v[3] = V[12]; v[7] = V[13];  v[11] = V[14];  v[15] = V[15];

	Matrix4 P;
	GLfloat p[16];

	P = project_matrix;
	p[0] = P[0];  p[4] = P[1];   p[8] = P[2];    p[12] = P[3];
	p[1] = P[4];  p[5] = P[5];   p[9] = P[6];    p[13] = P[7];
	p[2] = P[8];  p[6] = P[9];   p[10] = P[10];  p[14] = P[11];
	p[3] = P[12]; p[7] = P[13];  p[11] = P[14];  p[15] = P[15];

	// use uniform to send mvp to vertex shader
	glUniformMatrix4fv(iLocModel, 1, GL_FALSE, m);
	glUniformMatrix4fv(iLocView, 1, GL_FALSE, v);
	glUniformMatrix4fv(iLocProjection, 1, GL_FALSE, p);

	for (int i = 0; i < models[cur_idx].shapes.size(); i++) 
	{
		glViewport(0,0 ,WINDOW_WIDTH / 2, WINDOW_HEIGHT);
		refresh_memorydata(i, 0);
		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);


		glViewport(WINDOW_WIDTH / 2, 0, WINDOW_WIDTH / 2, WINDOW_HEIGHT);
		refresh_memorydata(i, 1);
		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
	}	

}


void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// [TODO] Call back function for keyboard
	if (key == GLFW_KEY_X && action == GLFW_PRESS) {
		cur_idx == 0 ? cur_idx = 4 : cur_idx = cur_idx - 1;
	}
	else if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
		cur_idx == 4 ? cur_idx = 0 : cur_idx = cur_idx + 1;
	}
	else if (key == GLFW_KEY_R && action == GLFW_PRESS) {
		cur_trans_mode = GeoRotation;
	}
	else if (key == GLFW_KEY_S && action == GLFW_PRESS) {
		cur_trans_mode = GeoScaling;
	}
	else if (key == GLFW_KEY_T && action == GLFW_PRESS) {
		cur_trans_mode = GeoTranslation;
	}
	else if (key == GLFW_KEY_K && action == GLFW_PRESS) {
		cur_trans_mode = LightPositionChange;
	}
	else if (key == GLFW_KEY_J && action == GLFW_PRESS) {
		cur_trans_mode = ShineeShange;
	}
	else if (key == GLFW_KEY_I && action == GLFW_PRESS) {
		cout << "Matrix Value :" << endl;
		cout << "Viewing Matrix :" << endl;
		cout << view_matrix << endl;
		cout << "Projection Matrix :" << endl;
		cout << project_matrix << endl;
		cout << "Translation Matrix :" << endl;
		cout << translate(models[cur_idx].position) << endl;
		cout << "Rotation Matrix :" << endl;
		cout << rotate(models[cur_idx].rotation) << endl;
		cout << "Scaling Matrix :" << endl;
		cout << scaling(models[cur_idx].scale) << endl;
	}
	else if (key == GLFW_KEY_L && action == GLFW_PRESS) {
		cur_LightMode == 2 ? cur_LightMode = 0 : cur_LightMode = cur_LightMode + 1;
	}

	/*else if (key == GLFW_KEY_E && action == GLFW_PRESS) {
		cur_trans_mode = ViewEye;
	}
	else if (key == GLFW_KEY_C && action == GLFW_PRESS) {
		cur_trans_mode = ViewCenter;
	}
	else if (key == GLFW_KEY_U && action == GLFW_PRESS) {
		cur_trans_mode = ViewUp;
	}*/
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	switch (cur_trans_mode) {
	case GeoTranslation:
		models[cur_idx].position = models[cur_idx].position + Vector3(0, 0, 0.01 * yoffset);
		break;

	case GeoScaling:
		models[cur_idx].scale = models[cur_idx].scale + Vector3(0, 0, 0.01 * yoffset);
		break;

	case GeoRotation:
		models[cur_idx].rotation = models[cur_idx].rotation + Vector3(0, 0, 0.01 * yoffset);
		break;

	case ViewEye:
		main_camera.position = main_camera.position - Vector3(0, 0, 0.01 * yoffset);
		cout << "main_camera.position:" << main_camera.position << endl;

		setViewingMatrix();
		break;
	case ViewCenter:
		main_camera.center = main_camera.center - Vector3(0, 0, 0.01 * yoffset);
		cout << "main_camera.center:" << main_camera.center << endl;

		setViewingMatrix();
		break;
	case ViewUp:
		main_camera.up_vector = main_camera.up_vector - Vector3(0, 0, 0.01 * yoffset);
		cout << "main_camera.up_vector:" << main_camera.up_vector << endl;

		setViewingMatrix();
		break;
	case LightPositionChange:
		if (cur_LightMode == 0) {
			directionalInfo.LightDiffuse = directionalInfo.LightDiffuse +Vector3(0.1 * yoffset, 0.1 * yoffset, 0.1 * yoffset);
		}
		if (cur_LightMode == 1) {
			pointInfo.LightDiffuse = pointInfo.LightDiffuse + Vector3(0.1 * yoffset, 0.1 * yoffset, 0.1 * yoffset);
		}
		if (cur_LightMode == 2) {
			spotInfo.Angle = spotInfo.Angle +  ( 0.3 * yoffset);
			cout << spotInfo.Angle << endl;
		}
		break;
	case ShineeShange:
		Shininess = Shininess + (0.5 * yoffset);
		cout <<"Shininess: " << Shininess << endl;

		break;
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// [TODO] Call back function for mouse
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		mouse_pressed = true;
	}
	else {
		mouse_pressed = false;
	}
		
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	// [TODO] cursor position callback function
	// [TODO] Call back function for cursor position


	if (mouse_pressed == true) {
		switch (cur_trans_mode) {
		case GeoTranslation:
			models[cur_idx].position = models[cur_idx].position + Vector3(0.005 * (xpos - starting_press_x), -0.005 * (ypos - starting_press_y), 0);
			break;
		case GeoScaling:
			models[cur_idx].scale = models[cur_idx].scale + Vector3(0.005 * (xpos - starting_press_x), 0.005 * (ypos - starting_press_y), 0);
			break;
		case GeoRotation:
			models[cur_idx].rotation = models[cur_idx].rotation + Vector3(0.005 * (ypos - starting_press_y), 0.005 * (xpos - starting_press_x), 0);
			break;
		case ViewEye:
			main_camera.position = main_camera.position - Vector3(0.005 * (xpos - starting_press_x), 0.005 * (ypos - starting_press_y), 0);
			cout << "main_camera.position:" << main_camera.position << endl;
			setViewingMatrix();
			break;
		case ViewCenter:
			main_camera.center = main_camera.center - Vector3(0.005 * (xpos - starting_press_x), 0.005 * (ypos - starting_press_y), 0);
			cout << "main_camera.center:" << main_camera.center << endl;
			setViewingMatrix();
			break;
		case ViewUp:
			main_camera.up_vector = main_camera.up_vector - Vector3(0.005 * (xpos - starting_press_x), 0.005 * (ypos - starting_press_y), 0);
			cout << "main_camera.up_vector:" << main_camera.up_vector << endl;
			setViewingMatrix();
			break;
		case LightPositionChange:
			if (cur_LightMode == 0) {
				directionalInfo.LightPosition= directionalInfo.LightPosition + Vector3(0.005 * (xpos - starting_press_x), -0.005 * (ypos - starting_press_y), 0);
			}
			if (cur_LightMode == 1) {
				pointInfo.LightPosition = pointInfo.LightPosition + Vector3(0.005 * (xpos - starting_press_x), -0.005 * (ypos - starting_press_y), 0);
			}
			if (cur_LightMode == 2) {
				spotInfo.LightPosition = spotInfo.LightPosition + Vector3(0.005 * (xpos - starting_press_x), -0.005 * (ypos - starting_press_y), 0);
			}
			break;
		case ShineeShange:
			break;
		}
	}
	starting_press_x = xpos;
	starting_press_y = ypos;
}

void setShaders()
{
	GLuint v, f, p;
	char *vs = NULL;
	char *fs = NULL;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead("shader.vs");
	fs = textFileRead("shader.fs");

	glShaderSource(v, 1, (const GLchar**)&vs, NULL);
	glShaderSource(f, 1, (const GLchar**)&fs, NULL);

	free(vs);
	free(fs);

	GLint success;
	char infoLog[1000];
	// compile vertex shader
	glCompileShader(v);
	// check for shader compile errors
	glGetShaderiv(v, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(v, 1000, NULL, infoLog);
		std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// compile fragment shader
	glCompileShader(f);
	// check for shader compile errors
	glGetShaderiv(f, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(f, 1000, NULL, infoLog);
		std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// create program object
	p = glCreateProgram();

	// attach shaders to program object
	glAttachShader(p,f);
	glAttachShader(p,v);

	// link program
	glLinkProgram(p);
	// check for linking errors
	glGetProgramiv(p, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(p, 1000, NULL, infoLog);
		std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
	}

	glDeleteShader(v);
	glDeleteShader(f);

	iLocModel = glGetUniformLocation(p, "model");
	iLocView = glGetUniformLocation(p, "view");
	iLocProjection = glGetUniformLocation(p, "projection");

	iLocLightPosition = glGetUniformLocation(p, "light.position");
	iLocLightDiffuse = glGetUniformLocation(p, "light.diffuse");
	iLocShadingType = glGetUniformLocation(p, "ShadingType");
	iLocLightMode = glGetUniformLocation(p, "LightMode");
	iLocKa = glGetUniformLocation(p, "material.Ka");
	iLocKd = glGetUniformLocation(p, "material.Kd");
	iLocKs = glGetUniformLocation(p, "material.Ks");
	iLocViewPos = glGetUniformLocation(p, "viewPos");
	iLocShininess = glGetUniformLocation(p, "material.shininess");
	iLocCutoffAngle = glGetUniformLocation(p, "light.angle");

	if (success)
		glUseProgram(p);
    else
    {
        system("pause");
        exit(123);
    }
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, tinyobj::shape_t* shape)
{
	vector<float> xVector, yVector, zVector;
	float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

	// find out min and max value of X, Y and Z axis
	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//maxs = max(maxs, attrib->vertices.at(i));
		if (i % 3 == 0)
		{

			xVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minX)
			{
				minX = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxX)
			{
				maxX = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 1)
		{
			yVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minY)
			{
				minY = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxY)
			{
				maxY = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 2)
		{
			zVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minZ)
			{
				minZ = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxZ)
			{
				maxZ = attrib->vertices.at(i);
			}
		}
	}

	float offsetX = (maxX + minX) / 2;
	float offsetY = (maxY + minY) / 2;
	float offsetZ = (maxZ + minZ) / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		if (offsetX != 0 && i % 3 == 0)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
		}
		else if (offsetY != 0 && i % 3 == 1)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
		}
		else if (offsetZ != 0 && i % 3 == 2)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
		}
	}

	float greatestAxis = maxX - minX;
	float distanceOfYAxis = maxY - minY;
	float distanceOfZAxis = maxZ - minZ;

	if (distanceOfYAxis > greatestAxis)
	{
		greatestAxis = distanceOfYAxis;
	}

	if (distanceOfZAxis > greatestAxis)
	{
		greatestAxis = distanceOfZAxis;
	}

	float scale = greatestAxis / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
		attrib->vertices.at(i) = attrib->vertices.at(i) / scale;
	}
	size_t index_offset = 0;
	for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
		int fv = shape->mesh.num_face_vertices[f];

		// Loop over vertices in the face.
		for (size_t v = 0; v < fv; v++) {
			// access to vertex
			tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
			// Optional: vertex colors
			colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
			// Optional: vertex normals
			if (idx.normal_index >= 0) {
				normals.push_back(attrib->normals[3 * idx.normal_index + 0]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 1]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 2]);
			}
		}
		index_offset += fv;
	}
}

string GetBaseDir(const string& filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos)
		return filepath.substr(0, filepath.find_last_of("/\\"));
	return "";
}

void LoadModels(string model_path)
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	tinyobj::attrib_t attrib;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;
	vector<GLfloat> normals;

	string err;
	string warn;

	string base_dir = GetBaseDir(model_path); // handle .mtl with relative path

#ifdef _WIN32
	base_dir += "\\";
#else
	base_dir += "/";
#endif

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str(), base_dir.c_str());

	if (!warn.empty()) {
		cout << warn << std::endl;
	}

	if (!err.empty()) {
		cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	printf("Load Models Success ! Shapes size %d Material size %d\n", shapes.size(), materials.size());
	model tmp_model;

	vector<PhongMaterial> allMaterial;
	for (int i = 0; i < materials.size(); i++)
	{
		PhongMaterial material;
		material.Ka = Vector3(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
		material.Kd = Vector3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
		material.Ks = Vector3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
		allMaterial.push_back(material);
	}

	for (int i = 0; i < shapes.size(); i++)
	{

		vertices.clear();
		colors.clear();
		normals.clear();
		normalization(&attrib, vertices, colors, normals, &shapes[i]);
		// printf("Vertices size: %d", vertices.size() / 3);

		Shape tmp_shape;
		glGenVertexArrays(1, &tmp_shape.vao);
		glBindVertexArray(tmp_shape.vao);

		glGenBuffers(1, &tmp_shape.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), &vertices.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		tmp_shape.vertex_count = vertices.size() / 3;

		glGenBuffers(1, &tmp_shape.p_color);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
		glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GL_FLOAT), &colors.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glGenBuffers(1, &tmp_shape.p_normal);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_normal);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GL_FLOAT), &normals.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		// not support per face material, use material of first face
		if (allMaterial.size() > 0)
			tmp_shape.material = allMaterial[shapes[i].mesh.material_ids[0]];
		tmp_model.shapes.push_back(tmp_shape);
	}
	shapes.clear();
	materials.clear();
	models.push_back(tmp_model);
}

void initParameter()
{
	proj.left = -1;
	proj.right = 1;
	proj.top = 1;
	proj.bottom = -1;
	proj.nearClip = 0.001;
	proj.farClip = 100.0;
	proj.fovy = 80;
	proj.aspect = (float)WINDOW_WIDTH/2/ WINDOW_HEIGHT;
	
	main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
	main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);

	directionalInfo.LightDiffuse = Vector3(1.0f, 1.0f, 1.0f);
	directionalInfo.LightPosition = Vector3(1.0f, 1.0f, 1.0f);
	Shininess = 64;

	pointInfo.LightPosition= Vector3(0.0f, 2.0f, 1.0f);
	pointInfo.LightDiffuse = Vector3(1.0f, 1.0f, 1.0f);
	pointInfo.Direction = Vector3(0.0f, 0.0f, 0.0f);

	spotInfo.LightPosition = Vector3(0.0f, 0.0f, 2.0f);
	spotInfo.LightDiffuse = Vector3(1.0f, 1.0f, 1.0f);
	spotInfo.Angle = 30;
	cur_LightMode =0;

	setViewingMatrix();
	setPerspective();	//set default projection matrix as perspective matrix
}

void setupRC()
{
	// setup shaders
	setShaders();
	initParameter();

	// OpenGL States and Values
	glClearColor(0.2, 0.2, 0.2, 1.0);
	vector<string> model_list{ "../NormalModels/bunny5KN.obj", "../NormalModels/dragon10KN.obj", "../NormalModels/lucy25KN.obj", "../NormalModels/teapot4KN.obj", "../NormalModels/dolphinN.obj"};
	// [TODO] Load five model at here
	for (int i = 0; i < 5; i++)
	{
		LoadModels(model_list[i]);
	}
}

void glPrintContextInfo(bool printExtension)
{
	cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
	cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
	cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
	cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	if (printExtension)
	{
		GLint numExt;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
		cout << "GL_EXTENSIONS =" << endl;
		for (GLint i = 0; i < numExt; i++)
		{
			cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
		}
	}
}


int main(int argc, char **argv)
{
    // initial glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
#endif

    
    // create window
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "109062530 HW2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    
    // load OpenGL function pointer
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
	// register glfw callback functions
    glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

    glfwSetFramebufferSizeCallback(window, ChangeSize);
	glEnable(GL_DEPTH_TEST);
	// Setup render context
	setupRC();
	int fps = 0;
	// main loop
    while (!glfwWindowShouldClose(window))
    {
        // render
        RenderScene();
        // swap buffer from back to front
        glfwSwapBuffers(window);

        // Poll input event
        glfwPollEvents();
    }
	
	// just for compatibiliy purposes
	return 0;
}
