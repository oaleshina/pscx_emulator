#pragma once

#include "SDL.h"
#include "glad.h"

// Position on VRAM
struct Position
{
	Position(GLshort x, GLshort y) : m_x(x), m_y(y) {}

	// Parse position from a GP0 parameter
	static Position fromPacked(uint32_t value)
	{
		int16_t x = value;
		int16_t y = value >> 16;

		return Position(x, y);
	}

	GLshort getX() const { return m_x; }
	GLshort getY() const { return m_y; }

private:
	GLshort m_x, m_y;
};

// RGB color
struct Color
{
	Color(GLubyte r, GLubyte g, GLubyte b) : m_r(r), m_g(g), m_b(b) {}

	// Parse color from a GP0 parameter
	static Color fromPacked(uint32_t value)
	{
		uint8_t r = value;
		uint8_t g = value >> 8;
		uint8_t b = value >> 16;

		return Color(r, g, b);
	}

private:
	GLubyte m_r, m_g, m_b;
};

struct Vertex
{
	Vertex(Position position, Color color, float alpha = 1.0f) :
		m_position(position),
		m_color(color),
		m_alpha(alpha)
	{}

	const Position& getPosition() const { return m_position; }
	const Color& getColor() const { return m_color; }

//private:
	// Position in PlayStation VRAM coordinates
	Position m_position;
	// RGB color, 8 bits per component
	Color m_color;
	// Vertex alpha value, used for blending
	float m_alpha;
};

// Maximum number of vertex that can be stored in an attribute buffers
const uint32_t VERTEX_BUFFER_LEN = 64 * 1024;

// Write only buffer with enough size for VERTEX_BUFFER_LEN elements
template<typename T>
struct Buffer
{
	Buffer()
	{
		m_object = 0;
		m_map = nullptr;
	}

	void OnCreate()
	{
		// Generate buffer object
		glGenBuffers(1, &m_object);

		// Bind buffer object
		glBindBuffer(GL_ARRAY_BUFFER, m_object);

		// Compute the size of the buffer
		GLsizeiptr elementSize = sizeof(T);
		GLsizeiptr bufferSize = elementSize * VERTEX_BUFFER_LEN;

		// Write only persistent mapping. Not coherent!
		// Allocate buffer memory
		glBufferStorage(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);

		// Remap the entire buffer
		m_map = (T*)glMapBufferRange(GL_ARRAY_BUFFER, 0, bufferSize, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);

		// Reset the buffer to 0 to avoid hard-to-reproduce bugs
		// if we do something wrong with uninitialized memory
		memset(m_map, 0x0, bufferSize);
	}

	// Set entry at 'index' to 'value' in the buffer
	void set(uint32_t index, T value)
	{
		m_map[index] = value;
	}

	void drop()
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_object);
		glUnmapBuffer(GL_ARRAY_BUFFER);
		glDeleteBuffers(1, &m_object);
	}

private:
	// OpenGL buffer object
	GLuint m_object;

	// Mapped buffer memory
	T* m_map;
};

struct Renderer
{
	Renderer();
	~Renderer();

	GLuint compileShader(char* src, GLenum shaderType);
	GLuint linkProgram(GLuint shaders[]);

	void drop();

	// Add a triangle to the draw buffer
	void pushTriangle(Vertex vertices[]);

	// Add a quad to the draw buffer
	void pushQuad(Vertex vertices[]);

	// Set the value of the uniform draw offset
	void setDrawOffset(int16_t x, int16_t y);

	// Set the drawing area. Coordinates are offsets in the PlayStation VRAM.
	void setDrawingArea(uint16_t left, uint16_t top, uint16_t right, uint16_t bottom);

	// Draw the buffered commands and reset the buffers
	void draw();

	// Draw the buffered commands and display them
	void display();

private:
	SDL_GLContext m_glContext;
	SDL_Window* m_window;

	// Framebuffer horizontal resolution (native:1024)
	uint16_t m_framebufferXResolution;
	// Framebuffer vertical resolution (native: 512)
	uint16_t m_framebufferYResolution;

	// Vertex shader object
	GLuint m_vertexShader;

	// Fragment shader object
	GLuint m_fragmentShader;

	// OpenGL Program object
	GLuint m_program;

	// OpenGL Vertex array object
	GLuint m_vertexArrayObject;

	// Buffer containing vertices
	Buffer<Vertex> m_vertices;

	// Current number of vertices in the buffers
	uint32_t m_numOfVertices;

	// Index of the "offset" shader uniform
	GLint m_uniformOffset;
};
