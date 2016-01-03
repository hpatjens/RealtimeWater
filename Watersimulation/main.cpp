// ------------------------------------------------------------------------------------------------------
//
// Description: Watersiumulation using a compute shader
//
// Author: Henrik Patjens
// Date: 5.1.2015
//
// ------------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------------
// Default Libraries
// ------------------------------------------------------------------------------------------------------
#include <cstdlib>
#include <cstdio>
#include <cstdarg>

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#define _USE_MATH_DEFINES
#include <math.h>
#include <numeric>
#include <set>
#include <streambuf>
#include <string>
#include <tuple>
#include <vector>

// ------------------------------------------------------------------------------------------------------
// External Libraries
// ------------------------------------------------------------------------------------------------------
#include "glew.h"
#include "glfw3.h"

#define GLM_FORCE_RADIANS
#include "glm.hpp"
#include "gtc/noise.hpp"
#include "gtc/type_ptr.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtx/transform.hpp"

#include "corona.h"

// ------------------------------------------------------------------------------------------------------
// Defines
// ------------------------------------------------------------------------------------------------------
#define for0(VAR, MAX) for (std::remove_const<decltype(MAX)>::type VAR = 0; VAR < (MAX); VAR++)
#define breakcase break; case
#define breakdefault break; default
#define intern static

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Vec2i = glm::ivec2;
using Mat2 = glm::mat2;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;

const auto IDENTITY4 = Mat4(1.0f);
const auto IDENTITY3 = Mat3(1.0f);

Mat3 makeMat3(const Mat4 m) {	
	auto mptr = glm::value_ptr(m);
	float a[] {
		mptr[0], mptr[1], mptr[2],
		mptr[4], mptr[5], mptr[6],
		mptr[8], mptr[9], mptr[10]
	};
	return glm::make_mat3(a);
}

Mat3 makeNormalMatrix(const Mat4& worldMatrix) {
	return makeMat3(glm::transpose(glm::inverse(worldMatrix)));
}

struct Attributes {
	enum {
		Position = 0,
		Normal = 1,
		TexCoord = 2,
		VertexPosition = 3
	};
};

struct Geometry {
	GLuint arrayBuffer;
	GLuint vertexArrayObject;
};

intern Geometry c_unitQuad;

struct Vertex {
	Vec3 position;
	Vec3 normal;
	Vec2 texCoord;
};

void setAttribPointer(GLuint vertexArrayObject, GLuint location, GLuint buffer, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLuint offset) {
	assert((glIsBuffer(buffer) != GL_FALSE));

	GLint previousBuffer;
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousBuffer);
	{
		GLint previousVAO;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVAO);
		{				
			glBindVertexArray(vertexArrayObject);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glEnableVertexAttribArray(location);
			glVertexAttribPointer(location, size, type, normalized, stride, reinterpret_cast<void*>(offset));
		}
		glBindVertexArray(previousVAO);
	}
	glBindBuffer(GL_ARRAY_BUFFER, previousBuffer);
}

void initialize() {
	// Unit Quad
	{
		Vertex unitQuadVertices[] {
			{ { -1, -1, 0 }, { 0, 1, 0 }, { 0, 0 } },
			{ {  1, -1, 0 }, { 0, 1, 0 }, { 1, 0 } },
			{ {  1,  1, 0 }, { 0, 1, 0 }, { 1, 1 } },

			{ { -1, -1, 0 }, { 0, 1, 0 }, { 0, 0 } },
			{ {  1,  1, 0 }, { 0, 1, 0 }, { 1, 1 } },
			{ { -1,  1, 0 }, { 0, 1, 0 }, { 0, 1 } },
		};

		// Array Buffer
		glGenBuffers(1, &c_unitQuad.arrayBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, c_unitQuad.arrayBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(unitQuadVertices), unitQuadVertices, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Vertex Array Object
		glGenVertexArrays(1, &c_unitQuad.vertexArrayObject);
		setAttribPointer(c_unitQuad.vertexArrayObject, Attributes::Position, c_unitQuad.arrayBuffer, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
		setAttribPointer(c_unitQuad.vertexArrayObject, Attributes::TexCoord, c_unitQuad.arrayBuffer, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, texCoord));
	}
}

void renderUnitQuad()  {
	glBindVertexArray(c_unitQuad.vertexArrayObject);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

struct Errors {
	enum { EverythingFine = 0, Fatal = 1 };
};

void quit(const char *format, ...) {
	va_list arg;
	int done;

	va_start(arg, format);
	done = fprintf(stderr, format, arg);
	va_end(arg);

#if defined(_DEBUG) && defined(_MSC_VER)
	__debugbreak();
#endif

	exit(Errors::Fatal);
}

enum class RenderMode { 
	Normal, 
	Background, 
	Water, 
	WaterMap, 
	TopView 
};

struct Direction { 
	enum { 
		Front, 
		Back, 
		Left, 
		Right, 
		Up, 
		Down, 
		DirectionCount 
	}; 
};

struct FramebufferData {
	GLuint colorTexture;
	GLuint depthTexture;
	GLuint id;
};

struct ProgramData {
	GLuint id;
	std::map<std::string, GLint> uniforms;
};

std::string loadFile(const std::string& filename) {
	std::ifstream t(filename);
	std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
	return str;
}

void checkShaderLog(GLuint shaderId) {
	GLint length;
	glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &length);

	if (length > 1) {
		std::vector<GLchar> logBuffer;
		logBuffer.resize(length);
		glGetShaderInfoLog(shaderId, length, &length, logBuffer.data());

		quit("Shaderlog: %s\n", logBuffer.data());
	}
}

void checkProgramLog(GLuint programId) {
	GLint length;
	glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &length);

	if (length > 1) {
		std::vector<GLchar> logBuffer;
		logBuffer.resize(length);
		glGetProgramInfoLog(programId, length, &length, logBuffer.data());

		quit("Programlog: %s\n", logBuffer.data());
	}
}

GLuint createProgram(std::set<std::tuple<std::string, GLenum>> shaderConfiguration) {
	auto programId = glCreateProgram();

	std::vector<GLuint> shaders;

	for (auto& shader : shaderConfiguration) {
		auto shaderFileName = std::get<0>(shader);
		auto shaderType = std::get<1>(shader);
		
		auto shaderSource = loadFile(shaderFileName);
		const char * shaderSourcePtr = shaderSource.c_str();

		auto shaderId = glCreateShader(shaderType);
		shaders.push_back(shaderId);

		glShaderSource(shaderId, 1, &shaderSourcePtr, nullptr);
		glCompileShader(shaderId);
		checkShaderLog(shaderId);

		glAttachShader(programId, shaderId);
	}

	glLinkProgram(programId);
	checkProgramLog(programId);

	for (auto& shader : shaders) {
		glDeleteShader(shader);
	}

	return programId;
}

struct Program {
	GLuint id;
	static void use(Program& program) {
		glUseProgram(program.id);
	}
	static void unuse() {
#if _DEBUG
		glUseProgram(0);
#endif
	}
};

struct WaterProgram : public Program {
	struct Uniforms {
		enum {
			WorldMatrix = 0,
			ViewMatrix = 1,
			ProjectionMatrix = 2,
			NormalMatrix = 3,
			FramebufferSize = 4,
			TopViewMatrix = 5,
			TopProjectionMatrix = 6,
			DeltaTime = 7,
			Time = 8,
		};
	};

	void initialize() {
		id = createProgram({
			std::make_tuple("content/Watersimulation/water.vert", GL_VERTEX_SHADER),
			std::make_tuple("content/Watersimulation/water.frag", GL_FRAGMENT_SHADER) 
		});
	}

	void worldMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::WorldMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void viewMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::ViewMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void projectionMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::ProjectionMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void normalMatrix(const Mat3& m) { glUniformMatrix3fv(Uniforms::NormalMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void framebufferSize(const Vec2& v) { glUniform2f(Uniforms::FramebufferSize, v.x, v.y); }
	void topViewMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::TopViewMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void topProjectionMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::TopProjectionMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void deltaTime(float f) { glUniform1f(Uniforms::DeltaTime, f); }
	void time(float f) { glUniform1f(Uniforms::Time, f); }

	void backgroundColorTexture(GLuint id) { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, id); }
	void backgroundDepthTexture(GLuint id) { glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, id); }
	void topViewDepthTexture(GLuint id) { glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, id); }
	void noiseTexture(GLuint id) { glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, id); }
	void noiseNormalTexture(GLuint id) { glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, id); }
	void subSurfaceScatteringTexture(GLuint id) { glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_1D, id); }
	void skyCubemapTexture(GLuint id) { glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_CUBE_MAP, id); }
};

struct CombineProgram : public Program {
	void initialize() {
		id = createProgram({
			std::make_tuple("content/Watersimulation/combine.vert", GL_VERTEX_SHADER),
			std::make_tuple("content/Watersimulation/combine.frag", GL_FRAGMENT_SHADER) 
		});
	}

	void backgroundColorTexture(GLuint id) { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, id); }
	void backgroundDepthTexture(GLuint id) { glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, id); }
	void waterColorTexture(GLuint id) { glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, id); }
	void waterDepthTexture(GLuint id) { glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, id); }
};

struct GroundProgram : public Program {
	struct Uniforms {
		enum {
			WorldMatrix = 0,
			ViewMatrix = 1,
			ProjectionMatrix = 2,
			NormalMatrix = 3,
			WaterMapProjectionMatrix = 4,
			WaterMapViewMatrix = 5,
			LightPosition = 6,
			GrassTextureScale = 7,
			SandTextureScale = 8,
			Time = 9,
		};
	};

	void initialize() {
		id = createProgram({
			std::make_tuple("content/Watersimulation/ground.vert", GL_VERTEX_SHADER),
			std::make_tuple("content/Watersimulation/ground.frag", GL_FRAGMENT_SHADER) 
		});
	}

	void worldMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::WorldMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void viewMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::ViewMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void projectionMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::ProjectionMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void normalMatrix(const Mat3& m) { glUniformMatrix3fv(Uniforms::NormalMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void waterMapProjectionMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::WaterMapProjectionMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void waterMapViewMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::WaterMapViewMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void lightPosition(const Vec3& v) { glUniform3f(Uniforms::LightPosition, v.x, v.y, v.z); }
	void grassTextureScale(float f) { glUniform1f(Uniforms::GrassTextureScale, f); }
	void sandTextureScale(float f) { glUniform1f(Uniforms::SandTextureScale, f); }
	void time(float f) { glUniform1f(Uniforms::Time, f); }

	void waterMapDepth(GLuint id) { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, id); }
	void waterMapNormals(GLuint id) { glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, id); }
	void grassTexture(GLuint id) { glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, id); }
	void sandTexture(GLuint id) { glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, id); }
	void noiseNormalTexture(GLuint id) { glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, id); }
	void causticTexture(GLuint id) { glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, id); }
	void subSurfaceScatteringTexture(GLuint id) { glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_1D, id); }
};

struct SkyProgram : public Program {
	struct Uniforms {
		enum {
			WorldMatrix = 0,
			ViewMatrix = 1,
			ProjectionMatrix = 2,
			InverseViewProjectionMatrix = 3,
		};
	};

	void initialize() {
		id = createProgram({
			std::make_tuple("content/Watersimulation/sky.vert", GL_VERTEX_SHADER),
			std::make_tuple("content/Watersimulation/sky.frag", GL_FRAGMENT_SHADER) 
		});
	}

	void worldMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::WorldMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void viewMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::ViewMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void projectionMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::ProjectionMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void inverseViewProjectionMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::InverseViewProjectionMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void skyCubemap(GLuint id) { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_CUBE_MAP, id); }
};

struct SimpleWaterProgram : public Program {
	struct Uniforms {
		enum {
			WorldMatrix = 0,
			ViewMatrix = 1,
			ProjectionMatrix = 2,
			NormalMatrix = 3
		};
	};

	void initialize() {
		id = createProgram({
			std::make_tuple("content/Watersimulation/simplewater.vert", GL_VERTEX_SHADER),
			std::make_tuple("content/Watersimulation/simplewater.frag", GL_FRAGMENT_SHADER) 
		});
	}
	
	void worldMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::WorldMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void viewMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::ViewMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void projectionMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::ProjectionMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void normalMatrix(const Mat3& m) { glUniformMatrix3fv(Uniforms::NormalMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
};

struct TextureProgram : public Program {
	struct Uniforms {
		enum {
			WorldMatrix = 0,
			ViewMatrix = 1,
			ProjectionMatrix = 2,
		};
	};

	void initialize() {
		id = createProgram({
			std::make_tuple("content/Watersimulation/texture.vert", GL_VERTEX_SHADER),
			std::make_tuple("content/Watersimulation/texture.frag", GL_FRAGMENT_SHADER) 
		});
	}

	void worldMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::WorldMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void viewMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::ViewMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void projectionMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::ProjectionMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void texture(GLuint id) { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, id); }
};

struct NormalsProgram : public Program {
	struct Uniforms {
		enum {
			WorldMatrix = 0,
			ViewMatrix = 1,
			ProjectionMatrix = 2,
			NormalMatrix = 3,
			NormalLength = 4,
			NormalColor = 5,
		};
	};

	void initialize() {
		id = createProgram({
			std::make_tuple("content/Watersimulation/normals.vert", GL_VERTEX_SHADER),
			std::make_tuple("content/Watersimulation/normals.geom", GL_GEOMETRY_SHADER),
			std::make_tuple("content/Watersimulation/normals.frag", GL_FRAGMENT_SHADER) 
		});
	}

	void worldMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::WorldMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void viewMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::ViewMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void projectionMatrix(const Mat4& m) { glUniformMatrix4fv(Uniforms::ProjectionMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void normalMatrix(const Mat3& m) { glUniformMatrix3fv(Uniforms::NormalMatrix, 1, GL_FALSE, glm::value_ptr(m)); }
	void normalLength(float f) { glUniform1f(Uniforms::NormalLength, f); }
	void normalColor(const Vec4& v) { glUniform4f(Uniforms::NormalColor, v.x, v.y, v.z, v.w); }
};

struct WaterSimulationProgram : public Program {
	void initialize() {
		id = createProgram({ std::make_tuple("content/Watersimulation/watersimulation.comp", GL_COMPUTE_SHADER) });
	}

	void dimension(int i) { glUniform1i(0, i); }
	void deltaTime(float f) { glUniform1f(1, f); }
	void time(float f) { glUniform1f(2, f); }
	void noiseTexture(GLuint id) { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, id); }

	void positionBuffer1(GLuint id) { glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, id); }
	void positionBuffer2(GLuint id) { glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, id); }
	void normalBuffer(GLuint id) { glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, id); }
	void terrainPositionsBuffer(GLuint id) { glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, id); }
};

struct {
	void initialize() {
		waterProgram.initialize();
		combineProgram.initialize();
		groundProgram.initialize();
		skyProgram.initialize();
		simpleWaterProgram.initialize();
		textureProgram.initialize();
		normalsProgram.initialize();
		waterSimulationProgram.initialize();
	}

	GLFWwindow* window;

	bool wireframe;
	bool normals;
	bool manualCamera;
	RenderMode renderMode;
	FramebufferData backgroundFramebuffer;
	FramebufferData waterFramebuffer;
	FramebufferData waterMapFramebuffer;
	FramebufferData topFramebuffer;
	glm::ivec2 topViewSize;
	glm::ivec2 waterMapSize;
	Mat4 topViewMatrix;
	Mat4 topProjectionMatrix;
	Mat4 projectionMatrix;

	Vec2 framebufferSize;

	GLuint debugTexture1;
	GLuint grassTexture;
	GLuint sandTexture;
	GLuint noiseTexture;
	GLuint noiseNormalTexture;
	GLuint causticTexture;
	GLuint subsurfaceScatteringTexture;

	GLuint skyCubemap;

	WaterProgram waterProgram;
	CombineProgram combineProgram;
	GroundProgram groundProgram;
	SkyProgram skyProgram;
	SimpleWaterProgram simpleWaterProgram;
	TextureProgram textureProgram;
	NormalsProgram normalsProgram;
	WaterSimulationProgram waterSimulationProgram;

	Vec3 lightPos;
	Mat4 lightProjectionMatrix;
	Mat4 lightViewMatrix;

	Mat4 camera;
	Vec2 mousePosition;
	bool rightMouseButtonPressed;
	bool moveDirection[Direction::DirectionCount];
} appData;

std::vector<Vertex> createMeshVertices(unsigned dimension, std::function<float(Vec2)> heightFunction) {
	std::vector<Vertex> vertices;
	auto cellSize = 1.0f / dimension;
	for0(x, dimension + 1) {
		for0(y, dimension + 1) {
			const auto NORMAL_EPSILON = cellSize / 10.0f;

			auto normalizedPosition = Vec2{ x * cellSize, y * cellSize }; // [0,1]
			auto epsilonPositionX = normalizedPosition + Vec2{ NORMAL_EPSILON, 0 };
			auto epsilonPositionY = normalizedPosition + Vec2{ 0, NORMAL_EPSILON };

			auto currentHeight = heightFunction(normalizedPosition);
			auto epsilonHeightX = heightFunction(epsilonPositionX);
			auto epsilonHeightY = heightFunction(epsilonPositionY);
			
			auto position = Vec3{ normalizedPosition.x, currentHeight, normalizedPosition.y };

			auto toEpsilonX = Vec3{ epsilonPositionX.x, epsilonHeightX, epsilonPositionX.y } - position;
			auto toEpsilonY = Vec3{ epsilonPositionY.x, epsilonHeightY, epsilonPositionY.y } -position;

			auto normal = glm::normalize(glm::cross(toEpsilonY, toEpsilonX));

			auto texCoord = normalizedPosition;
			vertices.push_back(Vertex{ position, normal, texCoord });
		}
	}
	return vertices;
}

std::vector<GLuint> createMeshIndices(unsigned dimension) {
	std::vector<GLuint> indices;
	const GLuint verticesPerRow = dimension + 1;
	for0(x, dimension) {
		for0(y, dimension) {
			// triangle 1
			indices.push_back(y * verticesPerRow + x);
			indices.push_back(y * verticesPerRow + (x + 1));
			indices.push_back((y + 1) * verticesPerRow + (x + 1));

			// triangle 2
			indices.push_back(y * verticesPerRow + x);
			indices.push_back((y + 1) * verticesPerRow + (x + 1));
			indices.push_back((y + 1) * verticesPerRow + x);

		}
	}
	return indices;
}

class Terrain {
public:
	Terrain(int width, int height, std::function<float(Vec2)> heightFunction)
		: m_width{ width }, m_height{height }
	{
		m_data.resize(width * height);

		for0(x, width) {
			for0(y, height) {
				m_data[index(x, y)] = heightFunction(Vec2(x / float(m_width), y / float(m_height)));
			}
		}
	}

	int getWidth() const { return m_width; }
	int getHeight() const { return m_height; }
	float getData(int x, int y) const { return m_data[index(x, y)]; }

private:
	std::vector<float> m_data;
	int m_width;
	int m_height;

	int index(int x, int y) const { return y*m_width + x; }
};

class Mesh {
public:
	Mesh(const Terrain& terrain)
		: m_dimension{ terrain.getWidth() }
	{
		unsigned dimension = terrain.getWidth();

		auto vertices = createMeshVertices(dimension, [](Vec2 v){ return 0; });
		auto indices = createMeshIndices(dimension);

		auto vertexCount = (dimension + 1) * (dimension + 1);
		m_indexCount = dimension * dimension * 6;

		auto c = [&](int v){ return std::min<int>(v, dimension - 1); };

		for0(i, vertices.size()) {
			auto x = i % (dimension + 1);
			auto y = i / (dimension + 1);
			vertices[i].position.y = terrain.getData(c(x), c(y));
		}			

		for0(i, vertices.size()) {
			auto x = i % (dimension + 1);
			auto y = i / (dimension + 1);
			auto index = [&](int x, int y){ return y*(dimension + 1) + x; };
			auto toPositiveX = vertices[index(std::min<int>(x + 1, dimension), y)].position - vertices[i].position;
			auto toPositiveY = vertices[index(x, std::min<int>(y + 1, dimension))].position - vertices[i].position;
			vertices[i].normal = x < dimension && y < dimension ? glm::normalize(glm::cross(toPositiveX, toPositiveY)) : Vec3{ 0, 1, 0 };
		}

		std::vector<Vec4> positionData;
		for0(i, vertices.size()) {
			positionData.push_back(Vec4{ vertices[i].position, 0.0 });
		}

		std::vector<Vec4> normalData;
		for0(i, vertices.size()) {
			normalData.push_back(Vec4{ vertices[i].normal, 0.0 });
		}

		std::vector<Vec2> texCoordData;
		for0(i, vertices.size()) {
			texCoordData.push_back(vertices[i].texCoord);
		}

		// Position Buffer
		glGenBuffers(2, m_positionBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vec4) * vertexCount, positionData.data(), GL_DYNAMIC_COPY);
		glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vec4) * vertexCount, positionData.data(), GL_DYNAMIC_COPY);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Normal Buffer
		glGenBuffers(1, &m_normalBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_normalBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vec4) * vertexCount, normalData.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// TexCoord Buffer
		glGenBuffers(1, &m_texCoordBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_texCoordBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vec2) * vertexCount, texCoordData.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Element Buffer
		glGenBuffers(1, &m_elementArrayBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementArrayBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)* m_indexCount, indices.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	Mesh(int dimension, std::function<float(Vec2)> heightFunction)
		: m_dimension{ dimension }
	{
		auto vertices = createMeshVertices(dimension, heightFunction);
		auto indices = createMeshIndices(dimension);

		auto vertexCount = (dimension + 1) * (dimension + 1);
		m_indexCount = dimension * dimension * 6;

		std::vector<Vec4> positionData;
		for0(i, vertices.size())
			positionData.push_back(Vec4{ vertices[i].position, 0.0 });

		std::vector<Vec4> normalData;
		for0(i, vertices.size())
			normalData.push_back(Vec4{ vertices[i].normal, 0.0f });

		std::vector<Vec2> texCoordData;
		for0(i, vertices.size())
			texCoordData.push_back(vertices[i].texCoord);

		// Position Buffer
		glGenBuffers(2, m_positionBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vec4) * vertexCount, positionData.data(), GL_DYNAMIC_COPY);
		glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vec4) * vertexCount, positionData.data(), GL_DYNAMIC_COPY);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Normal Buffer
		glGenBuffers(1, &m_normalBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_normalBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vec4) * vertexCount, normalData.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// TexCoord Buffer
		glGenBuffers(1, &m_texCoordBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_texCoordBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vec2) * vertexCount, texCoordData.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Element Buffer
		glGenBuffers(1, &m_elementArrayBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementArrayBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)* m_indexCount, indices.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void render() const
	{		
		glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer[0]);
		glEnableVertexAttribArray(Attributes::Position);
		glVertexAttribPointer(Attributes::Position, 3, GL_FLOAT, GL_FALSE, sizeof(Vec4), nullptr);

		glBindBuffer(GL_ARRAY_BUFFER, m_normalBuffer);
		glEnableVertexAttribArray(Attributes::Normal);
		glVertexAttribPointer(Attributes::Normal, 3, GL_FLOAT, GL_TRUE, sizeof(Vec4), reinterpret_cast<void*>(sizeof(Vec4)));

		glBindBuffer(GL_ARRAY_BUFFER, m_texCoordBuffer);
		glEnableVertexAttribArray(Attributes::TexCoord);
		glVertexAttribPointer(Attributes::TexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(Vec2), reinterpret_cast<void*>(sizeof(Vec4) * 2));

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementArrayBuffer);

		glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glDisableVertexAttribArray(Attributes::Position);
		glDisableVertexAttribArray(Attributes::Normal);
		glDisableVertexAttribArray(Attributes::TexCoord);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	int dimension() const { return m_dimension; }

	GLuint positionBuffer1() const { return m_positionBuffer[0]; }
	GLuint positionBuffer2() const { return m_positionBuffer[1]; }
	GLuint normalBuffer() const { return m_normalBuffer; }
	void swapBuffer() { std::swap(m_positionBuffer[0], m_positionBuffer[1]); }

private:
	int m_dimension;
	GLuint m_positionBuffer[2];
	GLuint m_normalBuffer;
	GLuint m_texCoordBuffer;
	GLuint m_elementArrayBuffer;
	GLuint m_vertexArrayObject;
	unsigned m_indexCount;
};

FramebufferData createGeneralFramebuffer(const unsigned width, const unsigned height)
{
	FramebufferData framebufferData;

	glGenTextures(1, &framebufferData.colorTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, framebufferData.colorTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &framebufferData.depthTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, framebufferData.depthTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &framebufferData.id);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebufferData.id);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferData.colorTexture, 0);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, framebufferData.depthTexture, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	return framebufferData;
}

GLFWwindow* createContext() {
	if (!glfwInit()) {
		quit("Could not initialize GLFW.");
	}		

	GLFWwindow* window = glfwCreateWindow(1200, 800, "Hello World", NULL, NULL);
	if (!window) {
		glfwTerminate();
		quit("Could not create the window.");
	}

	glfwMakeContextCurrent(window);

	glewExperimental = true;
	if (glewInit()) {
		if (!GLEW_ARB_debug_output) {
			printf("Could not load ARB_debug_output!");
		}
		quit("Could not initialize GLEW.");
	}

	glfwSetKeyCallback(window, [](GLFWwindow*, int key, int, int action, int modifier) {
		if (action != GLFW_REPEAT) {
			switch (key) {
				breakcase GLFW_KEY_W: appData.moveDirection[Direction::Front] = action == GLFW_PRESS;
				breakcase GLFW_KEY_A: appData.moveDirection[Direction::Left] = action == GLFW_PRESS;
				breakcase GLFW_KEY_S: appData.moveDirection[Direction::Back] = action == GLFW_PRESS;
				breakcase GLFW_KEY_D: appData.moveDirection[Direction::Right] = action == GLFW_PRESS;
				breakcase GLFW_KEY_SPACE: appData.moveDirection[Direction::Up] = action == GLFW_PRESS;
			}
		}
		appData.moveDirection[Direction::Down] = modifier == GLFW_MOD_SHIFT;

		if (action != GLFW_PRESS) {
			return;
		}

		switch (key) {
			breakcase GLFW_KEY_F1: appData.wireframe = !appData.wireframe; printf("Wireframe: %s\n", appData.wireframe ? "on" : "off");
			breakcase GLFW_KEY_F2: appData.normals = !appData.normals; printf("Normals: %s\n", appData.normals ? "on" : "off");
			breakcase GLFW_KEY_F3: appData.manualCamera = !appData.manualCamera; printf("Camera: %s", appData.manualCamera ? "on" : "off");
			breakcase GLFW_KEY_F5: appData.renderMode = RenderMode::Normal;
			breakcase GLFW_KEY_F6: appData.renderMode = RenderMode::Background;
			breakcase GLFW_KEY_F7: appData.renderMode = RenderMode::WaterMap;
			breakcase GLFW_KEY_F8: appData.renderMode = RenderMode::Water;
			breakcase GLFW_KEY_F9: appData.renderMode = RenderMode::TopView;
			breakcase GLFW_KEY_ESCAPE: exit(0);
		}
	});

	glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) { 
		appData.rightMouseButtonPressed = button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS; 
	});

	glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x, double y) { 
		appData.mousePosition = Vec2{ x, y };	
	});

	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
		glViewport(0, 0, width, height);

		glDeleteFramebuffers(1, &appData.backgroundFramebuffer.id);
		appData.backgroundFramebuffer = createGeneralFramebuffer(width, height);
		glDeleteFramebuffers(1, &appData.waterFramebuffer.id);
		appData.waterFramebuffer = createGeneralFramebuffer(width, height);

		appData.framebufferSize = Vec2{ width, height };
	});

	if (GLEW_ARB_debug_output) {
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
			if (type == GL_DEBUG_TYPE_ERROR_ARB) {
				quit("GLEW_ARB_debug_output: %s\n", message);
			}
		}, nullptr);
		glEnable(GL_DEBUG_OUTPUT);
	}

	return window;
}

GLuint createSubsurfaceScatteringTexture() {
	GLuint id;
	glGenTextures(1, &id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_1D, id);

	struct Pixel  { GLubyte r, g, b; };

	const int size = 3;
	Pixel data[3];
	data[0] = Pixel{ 2, 204, 147 };
	data[1] = Pixel{ 2, 127, 199 };
	data[2] = Pixel{ 1, 9, 100 };

	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);

	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	glBindTexture(GL_TEXTURE_1D, 0);

	return id;
}

GLuint createDebugTexture(int width, int height, int cellSize = 32) {
	GLuint id;
	glGenTextures(1, &id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, id);

	struct Pixel  { GLubyte r, g, b, a; };

	std::vector<Pixel> data;
	data.resize(sizeof(Pixel) * width * height);
	for0(x, width) {
		for0(y, height) {
			data[y*width + x] = Pixel { 
				GLubyte(((x / cellSize) + (y / cellSize)) % 2 == 0 ? 0xFF : 0x00),
				GLubyte((((x / cellSize) + (y / cellSize)) % 2 == 1) && (y / cellSize) % 2 == 0 ? 0xFF : 0x00),
				GLubyte((((x / cellSize) + (y / cellSize)) % 2 == 1) && (x / cellSize) % 2 == 0 ? 0xFF : 0x00),
				GLubyte(1)
			};
		}
	}		

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

	glBindTexture(GL_TEXTURE_2D, 0);

	return id;
}

corona::Image* importImage(const std::string& filename) {
	auto extension = filename.substr(filename.find_last_of(".") + 1);
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

	corona::FileFormat format;
	if (extension == "jpg" || extension == "jpeg") {
		format = corona::FileFormat::FF_JPEG;
	} else if (extension == "bmp") {
		format = corona::FileFormat::FF_BMP;
	} else if (extension == "gif") {
		format = corona::FileFormat::FF_GIF;
	} else if (extension == "png") {
		format = corona::FileFormat::FF_PNG;
	} else if (extension == "pcx") {
		format = corona::FileFormat::FF_PCX;
	} else if (extension == "tga") {
		format = corona::FileFormat::FF_TGA;
	} else {
		quit("Format of image %s is not supported.\n", filename.c_str());
	}

	auto imageOriginal = corona::OpenImage(filename.c_str(), format);
	auto imageConverted = corona::ConvertImage(imageOriginal, corona::PixelFormat::PF_R8G8B8A8);

	return imageConverted;
}

GLuint importTexture(const std::string& filename) {
	auto imageConverted = importImage(filename);

	auto dataPtr = static_cast<char*>(imageConverted->getPixels());
	auto pixelCount = imageConverted->getHeight() * imageConverted->getWidth();

	std::vector<unsigned char> data{ dataPtr, dataPtr + pixelCount * 4 };

	auto width = static_cast<unsigned>(imageConverted->getWidth());
	auto height = static_cast<unsigned>(imageConverted->getHeight());

	GLuint id;
	glGenTextures(1, &id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); // <---- test this

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // GL_REPEAT

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, dataPtr);

	glGenerateMipmap(GL_TEXTURE_2D); // <---- test this

	glBindTexture(GL_TEXTURE_2D, 0);

	return id;
}


void render(const float dt, const Mat4& projectionMatrix, const Mesh& waterMesh, const Mesh& groundMesh) {
	float CAMERA_DISTANCE = 0.8f;

	float time = float(glfwGetTime()) * 0.2f;
	auto x = cos(time) * CAMERA_DISTANCE;
	auto z = sin(time) * CAMERA_DISTANCE;

	Mat4 viewMatrix = appData.manualCamera ? 
		glm::inverse(appData.camera) : 
		glm::lookAt(Vec3(x, 0.2f, z), Vec3(0, -0.2f, 0), Vec3(0, 1, 0));
	
	if (appData.wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	auto waterWorldMatrix = glm::translate(glm::scale(IDENTITY4, Vec3{ 2.0f }), Vec3{ -0.5f, 0, -0.5f });
	auto waterNormalMatrix = makeMat3(glm::transpose(glm::inverse(waterWorldMatrix)));
	auto groundWorldMatrix = glm::translate(glm::scale(IDENTITY4, Vec3{ 2.0f }), Vec3{ -0.5f, 0, -0.5f });
	auto groundNormalMatrix = makeMat3(glm::transpose(glm::inverse(groundWorldMatrix)));

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	//
	// Render water-map
	//
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, appData.waterMapFramebuffer.id);
		glClearColor(0.1f, 0.2f, 0.7f, 1.0f);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		Program::use(appData.simpleWaterProgram);
		{
			appData.simpleWaterProgram.worldMatrix(waterWorldMatrix);
			appData.simpleWaterProgram.viewMatrix(appData.lightViewMatrix);
			appData.simpleWaterProgram.projectionMatrix(appData.lightProjectionMatrix);

			glViewport(0, 0, appData.waterMapSize.x, appData.waterMapSize.y);
			waterMesh.render();
			glViewport(0, 0, GLsizei(appData.framebufferSize.x), GLsizei(appData.framebufferSize.y));
		}
		Program::unuse();

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}

	//
	// Render top-view
	//
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, appData.topFramebuffer.id);
		glClearColor(0.1f, 0.2f, 0.7f, 1.0f);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		Program::use(appData.simpleWaterProgram);
		{
			appData.simpleWaterProgram.worldMatrix(IDENTITY4);
			appData.simpleWaterProgram.viewMatrix(appData.topViewMatrix);
			appData.simpleWaterProgram.projectionMatrix(appData.topProjectionMatrix);
			appData.simpleWaterProgram.normalMatrix(IDENTITY3);

			glViewport(0, 0, appData.topViewSize.x, appData.topViewSize.y);
			groundMesh.render();
			glViewport(0, 0, GLsizei(appData.framebufferSize.x), GLsizei(appData.framebufferSize.y));
		} 
		Program::unuse();

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}

	//
	// Render ground
	//
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, appData.backgroundFramebuffer.id);
		glClearColor(0.1f, 0.5f, 0.2f, 1.0f);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
		Program::use(appData.skyProgram);
		{
			auto inverseViewProjectionMatrix = glm::inverse(projectionMatrix * viewMatrix);
			appData.skyProgram.worldMatrix(IDENTITY4);
			appData.skyProgram.viewMatrix(viewMatrix);
			appData.skyProgram.projectionMatrix(projectionMatrix);
			appData.skyProgram.inverseViewProjectionMatrix(inverseViewProjectionMatrix);
		
			appData.skyProgram.skyCubemap(appData.skyCubemap);

			glDisable(GL_DEPTH_TEST);
			renderUnitQuad();
			glEnable(GL_DEPTH_TEST);
		}
		Program::use(appData.groundProgram);
		{
			appData.groundProgram.worldMatrix(groundWorldMatrix);
			appData.groundProgram.viewMatrix(viewMatrix);
			appData.groundProgram.projectionMatrix(projectionMatrix);
			appData.groundProgram.normalMatrix(IDENTITY3);
			appData.groundProgram.waterMapProjectionMatrix(appData.lightProjectionMatrix);
			appData.groundProgram.waterMapViewMatrix(appData.lightViewMatrix);
			appData.groundProgram.lightPosition(appData.lightPos);
			appData.groundProgram.grassTextureScale(24.0f);
			appData.groundProgram.sandTextureScale(36.0f);
			appData.groundProgram.time(time);
		
			appData.groundProgram.waterMapDepth(appData.waterMapFramebuffer.depthTexture);
			appData.groundProgram.waterMapNormals(appData.waterMapFramebuffer.colorTexture);
			appData.groundProgram.grassTexture(appData.grassTexture);
			appData.groundProgram.sandTexture(appData.sandTexture);
			appData.groundProgram.noiseNormalTexture(appData.noiseNormalTexture);
			appData.groundProgram.causticTexture(appData.causticTexture);
			appData.groundProgram.subSurfaceScatteringTexture(appData.subsurfaceScatteringTexture);

			groundMesh.render();
		}
		Program::unuse();
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}
	
	//
	// Render water
	//
	Program::use(appData.waterProgram);
	{	
		appData.waterProgram.framebufferSize(appData.framebufferSize);
		appData.waterProgram.worldMatrix(waterWorldMatrix);
		appData.waterProgram.viewMatrix(viewMatrix);
		appData.waterProgram.projectionMatrix(projectionMatrix);
		appData.waterProgram.normalMatrix(waterNormalMatrix);
		appData.waterProgram.topViewMatrix(appData.topViewMatrix);
		appData.waterProgram.topProjectionMatrix(appData.topProjectionMatrix);

		appData.waterProgram.deltaTime(dt);
		appData.waterProgram.time((float)glfwGetTime());

		appData.waterProgram.backgroundColorTexture(appData.backgroundFramebuffer.colorTexture);
		appData.waterProgram.backgroundDepthTexture(appData.backgroundFramebuffer.depthTexture);
		appData.waterProgram.topViewDepthTexture(appData.topFramebuffer.depthTexture);
		appData.waterProgram.noiseTexture(appData.noiseTexture);
		appData.waterProgram.noiseNormalTexture(appData.noiseNormalTexture);
		appData.waterProgram.subSurfaceScatteringTexture(appData.subsurfaceScatteringTexture);
		appData.waterProgram.skyCubemapTexture(appData.skyCubemap);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, appData.waterFramebuffer.id);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDisable(GL_CULL_FACE);
		waterMesh.render();
		glEnable(GL_CULL_FACE);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}
	Program::unuse();


	//
	// Combine framebuffer
	//
	Program::use(appData.combineProgram);
	{
		glClearColor(0.5f, 0.5f, 0.2f, 1.0f);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		appData.combineProgram.backgroundColorTexture(appData.backgroundFramebuffer.colorTexture);
		appData.combineProgram.backgroundDepthTexture(appData.backgroundFramebuffer.depthTexture);
		appData.combineProgram.waterColorTexture(appData.waterFramebuffer.colorTexture);
		appData.combineProgram.waterDepthTexture(appData.waterFramebuffer.depthTexture);

		glDisable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		renderUnitQuad();
	}
	Program::unuse();

	//
	// Debug Rendering
	//
	if (appData.renderMode != RenderMode::Normal) {
		glDisable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		Program::use(appData.textureProgram);
		{
			appData.textureProgram.worldMatrix(IDENTITY4);
			appData.textureProgram.viewMatrix(IDENTITY4);
			appData.textureProgram.projectionMatrix(IDENTITY4);

			switch (appData.renderMode) {
				breakcase RenderMode::Background: appData.textureProgram.texture(appData.backgroundFramebuffer.colorTexture);
				breakcase RenderMode::WaterMap: appData.textureProgram.texture(appData.waterMapFramebuffer.colorTexture);
				breakcase RenderMode::Water: appData.textureProgram.texture(appData.waterFramebuffer.colorTexture);
				breakcase RenderMode::TopView: appData.textureProgram.texture(appData.topFramebuffer.colorTexture);
				breakdefault: appData.textureProgram.texture(appData.waterFramebuffer.colorTexture);
			}

			glClearColor(0.5f, 0.5f, 0.2f, 1.0f);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			renderUnitQuad();
		}
		Program::unuse();
	}

	if (appData.normals) {
		Program::use(appData.normalsProgram);
		{
			appData.normalsProgram.worldMatrix(waterWorldMatrix);
			appData.normalsProgram.viewMatrix(viewMatrix);
			appData.normalsProgram.projectionMatrix(projectionMatrix);
			appData.normalsProgram.normalColor(Vec4(0, 0, 1, 1));
	
			waterMesh.render();
	
			appData.normalsProgram.worldMatrix(groundWorldMatrix);
			appData.normalsProgram.normalMatrix(groundNormalMatrix);
			appData.normalsProgram.normalColor(Vec4(0, 1, 0, 1));

			groundMesh.render();
		}
		Program::unuse();
	}
}

class FpsCounter {
public:
	int frame() {
		double currentTime = glfwGetTime();
		m_frames++;
		if (currentTime - m_lastTime >= 1.0)
		{
			m_lastFps = m_frames;
			m_frames = 0;
			m_lastTime += 1.0;
		}
		return m_lastFps;
	}

private:
	int m_frames = 0;
	int m_lastFps = 0;
	double m_lastTime = glfwGetTime();
};

GLuint createCubemap() {
	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_CUBE_MAP, id);

	auto imagePositiveX = importImage("content/Watersimulation/sky_posx.jpg");
	auto imageNegativeX = importImage("content/Watersimulation/sky_negx.jpg");
	auto imagePositiveY = importImage("content/Watersimulation/sky_posy.jpg");
	auto imageNegativeY = importImage("content/Watersimulation/sky_negy.jpg");
	auto imagePositiveZ = importImage("content/Watersimulation/sky_posz.jpg");
	auto imageNegativeZ = importImage("content/Watersimulation/sky_negz.jpg");

	auto size = 2048;

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, imagePositiveX->getPixels());
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageNegativeX->getPixels());
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, imagePositiveY->getPixels());
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageNegativeY->getPixels());
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, imagePositiveZ->getPixels());
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageNegativeZ->getPixels());

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	return id;
}

void update() {
	float cameraSpeed = 0.01f;

	if (appData.moveDirection[Direction::Front]) appData.camera = glm::translate(appData.camera, Vec3{ 0, 0, -1 } * cameraSpeed);
	if (appData.moveDirection[Direction::Back])  appData.camera = glm::translate(appData.camera, Vec3{ 0, 0, +1 } * cameraSpeed);
	if (appData.moveDirection[Direction::Right]) appData.camera = glm::translate(appData.camera, Vec3{ +1, 0, 0 } * cameraSpeed);
	if (appData.moveDirection[Direction::Left])  appData.camera = glm::translate(appData.camera, Vec3{ -1, 0, 0 } * cameraSpeed);
	if (appData.moveDirection[Direction::Up])	 appData.camera = glm::translate(appData.camera, Vec3{ 0, +1, 0 } * cameraSpeed);
	if (appData.moveDirection[Direction::Down])  appData.camera = glm::translate(appData.camera, Vec3{ 0, -1, 0 } * cameraSpeed);

	if (appData.manualCamera)
	{
		auto middle = Vec2{ appData.framebufferSize.x / 2.0f, appData.framebufferSize.y / 2.0f };
		auto offset = middle - appData.mousePosition;

		const auto rotationSpeed = 0.0001f;

		if (appData.rightMouseButtonPressed)
		{
			auto toVec3 = [](Vec4 v){ return Vec3{ v.x / v.w, v.y / v.w, v.z / v.w }; };

			auto normalMatrix = glm::transpose(glm::inverse(appData.camera));
			auto yAxisInWorld = glm::inverse(normalMatrix) * Vec4{ 0, 1, 0, 1 };
			auto yAxisInWorldWDiv = toVec3(yAxisInWorld);
			
			auto xAxis = Vec3{ 1, 0, 0 };

			appData.camera = glm::rotate(appData.camera, float(offset.y * rotationSpeed), xAxis);
			appData.camera = glm::rotate(appData.camera, float(offset.x * rotationSpeed), yAxisInWorldWDiv);
			
		}

		glfwSetCursorPos(appData.window, middle.x, middle.y);
		appData.mousePosition = middle;
	}
}

void simulateWater(const Mesh& waterMesh, const Mesh& terrain, float dt, float time) {
	glUseProgram(appData.waterSimulationProgram.id);

	auto dimension = waterMesh.dimension() + 1;

	appData.waterSimulationProgram.dimension(dimension);
	appData.waterSimulationProgram.deltaTime(dt);
	appData.waterSimulationProgram.time(time);
	appData.waterSimulationProgram.noiseTexture(appData.noiseTexture);

	appData.waterSimulationProgram.positionBuffer1(waterMesh.positionBuffer1());
	appData.waterSimulationProgram.positionBuffer2(waterMesh.positionBuffer2());
	appData.waterSimulationProgram.normalBuffer(waterMesh.normalBuffer());
	appData.waterSimulationProgram.terrainPositionsBuffer(terrain.positionBuffer1());

	glDispatchCompute(dimension, dimension, 1);

	glUseProgram(0);
}

int main(int argc, char* argv[]) {
	appData.window = createContext();

	appData.initialize();

	initialize();

	int framebufferWidth, framebufferHeight;
	glfwGetFramebufferSize(appData.window, &framebufferWidth, &framebufferHeight);
	appData.projectionMatrix = glm::perspectiveFov(45.0f, float(framebufferWidth), float(framebufferHeight), 0.001f, 100.0f);
	appData.framebufferSize = Vec2{ framebufferWidth, framebufferHeight };

	// camera
	appData.camera = IDENTITY4;
	appData.topViewMatrix = glm::lookAt(Vec3{ 0.5f, 0.5f, 0.5f }, Vec3{ 0.5f, 0.0f, 0.5f }, Vec3{ 1, 0, 0 });
	appData.topProjectionMatrix = glm::ortho(-0.5, 0.5, -0.5, 0.5);

	// set up light
	appData.lightPos = Vec3{ -1, 1, -1 };
	appData.lightProjectionMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.01f, 5.0f);
	appData.lightViewMatrix = glm::lookAt(appData.lightPos, Vec3{ 0 }, Vec3{ 0, 1, 0 });

	// textures
	appData.debugTexture1 = createDebugTexture(512, 512);
	appData.grassTexture = importTexture("content/Watersimulation/grassTexture.jpg");
	appData.sandTexture = importTexture("content/Watersimulation/sandTexture.jpg");
	appData.noiseTexture = importTexture("content/Watersimulation/noiseTexture.png");
	appData.noiseNormalTexture = importTexture("content/Watersimulation/noiseNormalTexture.jpg");
	appData.causticTexture = importTexture("content/Watersimulation/causticTexture.png");
	appData.subsurfaceScatteringTexture = createSubsurfaceScatteringTexture();

	// cubemap
	appData.skyCubemap = createCubemap();

	appData.waterMapSize = glm::ivec2{ 1024 };
	appData.topViewSize = glm::ivec2{ 1024 };

	// create framebuffer
	appData.backgroundFramebuffer = createGeneralFramebuffer(framebufferWidth, framebufferHeight);
	appData.waterFramebuffer = createGeneralFramebuffer(framebufferWidth, framebufferHeight);
	appData.waterMapFramebuffer = createGeneralFramebuffer(appData.waterMapSize.x, appData.waterMapSize.y);
	appData.topFramebuffer = createGeneralFramebuffer(appData.topViewSize.x, appData.topViewSize.y);

	int terrainSize = 200;

	auto groundHeightFunction = [](Vec2 coordinate) {
		float COORDINATE_STRETCH = 5.0f;
		float EFUNCTION_STRETCH = 10.0f;
		float EFUNCTION_WEIGHT = -0.7f;
		float NOISE_WEIGHT = 0.015f;
		float NOISE_STRETCH = 6.0f;
		float HEIGHT = 0.6f;

		auto correctedCoordinate = (Vec2{ 1.0 } - coordinate) * COORDINATE_STRETCH - Vec2{ 0.0f };
		auto efunction = (1.0f / sqrt(2.0f * M_PI)) * exp(-(1.0f / EFUNCTION_STRETCH) * correctedCoordinate.x * correctedCoordinate.x);

		return static_cast<float>(HEIGHT * (0.1 + EFUNCTION_WEIGHT * efunction + NOISE_WEIGHT * glm::simplex(coordinate * NOISE_STRETCH)));
	};
	Terrain terrain(terrainSize, terrainSize, groundHeightFunction);
	Mesh groundMesh(terrain);

	Mesh waterMesh(terrainSize, [](Vec2 coordinate) { return 0; });

	FpsCounter fpsCounter;

	glClearColor(0, 0.5f, 0.5f, 1);
	glClearDepth(1);
	glEnable(GL_DEPTH_TEST);

	float lastFrameTime = 0;
	
	while (!glfwWindowShouldClose(appData.window)) {
		glfwSetWindowTitle(appData.window, std::to_string(fpsCounter.frame()).c_str());

		update();

		auto time = (float)glfwGetTime();
		auto timeDelta = time - lastFrameTime;
		lastFrameTime = time;
		simulateWater(waterMesh, groundMesh, timeDelta, time);

		// render the scene
		render(timeDelta, appData.projectionMatrix, waterMesh, groundMesh);

		waterMesh.swapBuffer();

		//glfwSwapInterval(0);
		glfwSwapBuffers(appData.window);
		glfwPollEvents();
	}

	glfwTerminate();

	std::cin.get();

	return 0;
}