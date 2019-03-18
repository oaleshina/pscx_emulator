#include "pscx_renderer.h"

#include <string>
#include <fstream>

static char* loadShaderSource(const std::string& filename)
{
	std::ifstream shaderSource(filename, std::ios::in | std::ios::binary);
	if (!shaderSource.good()) return nullptr;

	shaderSource.seekg(0, std::ios::end);
	size_t shaderSourceSize = shaderSource.tellg();

	char* shaderSourceStr = new char[shaderSourceSize + 1];

	shaderSource.seekg(0, std::ios::beg);
	shaderSource.read(shaderSourceStr, shaderSourceSize);
	shaderSourceStr[shaderSourceSize] = '\0';
	
	shaderSource.close();
	
	return shaderSourceStr;
}

Renderer::Renderer()
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_DisplayMode current;

	m_window = SDL_CreateWindow("PSX", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);

	m_glContext = SDL_GL_CreateContext(m_window);

	gladLoadGL();

	SDL_GL_MakeCurrent(m_window, m_glContext);
	
	//glViewport(0, 0, 1024, 512);

	// Clear the window
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//SDL_GL_SwapWindow(m_window);

	char* vsSrc = loadShaderSource(".\\assets\\vertex.glsl");
	char* fsSrc = loadShaderSource(".\\assets\\fragment.glsl");

	// Compile shaders
	m_vertexShader = compileShader(vsSrc, GL_VERTEX_SHADER);
	m_fragmentShader = compileShader(fsSrc, GL_FRAGMENT_SHADER);

	GLuint shaders[] = { m_vertexShader, m_fragmentShader };

	// Link program
	m_program = linkProgram(shaders);

	glUseProgram(m_program);

	// Generate vertex attribute object
	glGenVertexArrays(1, &m_vertexArrayObject);
	glBindVertexArray(m_vertexArrayObject);

	m_positions.OnCreate();

	// Setup the "position" attribute
	{
		GLuint index = glGetAttribLocation(m_program, "vertex_position");
		glEnableVertexAttribArray(index);
		glVertexAttribIPointer(index, 2, GL_SHORT, 0, nullptr);
	}

	m_colors.OnCreate();

	// Setup the "color" attribute
	{
		GLuint index = glGetAttribLocation(m_program, "vertex_color");
		glEnableVertexAttribArray(index);
		glVertexAttribIPointer(index, 3, GL_UNSIGNED_BYTE, 0, nullptr);
	}

	// Retrieve and initialize the draw offset
	m_uniformOffset = glGetUniformLocation(m_program, "offset");
	glUniform2i(m_uniformOffset, 0, 0);

	m_numOfVertices = 0x0;
}

Renderer::~Renderer()
{
	/*SDL_GL_DeleteContext(m_glContext);
	SDL_DestroyWindow(m_window);
	SDL_Quit();*/
}

GLuint Renderer::compileShader(char* src, GLenum shaderType)
{
	GLuint shaderHandle = 0;

	GLint shaderSize = strlen(src);
	shaderHandle = glCreateShader(shaderType);

	glShaderSource(shaderHandle, 1, &src, &shaderSize);
	glCompileShader(shaderHandle);
	
	GLint compileStatus;
	glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileStatus);
	
	if (compileStatus == GL_FALSE)
	{
		GLint infoLogLength = 0;
		glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &infoLogLength);
		
		if (infoLogLength > 0)
		{
			char* infoLog = new char[infoLogLength];
			glGetShaderInfoLog(shaderHandle, infoLogLength, nullptr, infoLog);
			printf("%s\n", infoLog);
			delete[] infoLog;
		}
	}
	return shaderHandle;
}

GLuint Renderer::linkProgram(GLuint shaders[])
{
	GLuint programHandle = 0;

	programHandle = glCreateProgram();
	glAttachShader(programHandle, shaders[0]);
	glAttachShader(programHandle, shaders[1]);

	glLinkProgram(programHandle);
	
	GLint linkStatus;
	glGetProgramiv(programHandle, GL_LINK_STATUS, &linkStatus);
	
	if (linkStatus == GL_FALSE)
	{
		GLint infoLogLength = 0;
		glGetProgramiv(programHandle, GL_INFO_LOG_LENGTH, &infoLogLength);
		
		if (infoLogLength > 0)
		{
			char* infoLog = new char[infoLogLength];
			glGetProgramInfoLog(programHandle, infoLogLength, nullptr, infoLog);
			printf("%s\n", infoLog);
			delete[] infoLog;
		}
	}

	return programHandle;
}

void Renderer::drop()
{
	glDeleteVertexArrays(1, &m_vertexArrayObject);
	glDeleteShader(m_vertexShader);
	glDeleteShader(m_fragmentShader);
	glDeleteProgram(m_program);
}

void Renderer::pushTriangle(Position positions[], Color colors[])
{
	// Make sure we have enough room left to queue the vertex
	if (m_numOfVertices + 3 > VERTEX_BUFFER_LEN)
		draw();
	
	for (size_t i = 0; i < 3; ++i)
	{
		m_positions.set(m_numOfVertices, positions[i]);
		m_colors.set(m_numOfVertices, colors[i]);
		m_numOfVertices += 1;
	}
}

void Renderer::pushQuad(Position positions[], Color colors[])
{
	// Make sure we have enough room left to queue the vertex.
	// We need to push two triangles to draw a quad ( 6 vertices )
	if (m_numOfVertices + 6 > VERTEX_BUFFER_LEN)
	{
		// The vertex attribute buffers are full, force an early draw
		draw();
	}

	// Push the first triangle
	for (size_t i = 0; i < 3; ++i)
	{
		m_positions.set(m_numOfVertices, positions[i]);
		m_colors.set(m_numOfVertices, colors[i]);
		m_numOfVertices += 1;
	}

	// Push the second triangle
	for (size_t i = 1; i < 4; ++i)
	{
		m_positions.set(m_numOfVertices, positions[i]);
		m_colors.set(m_numOfVertices, colors[i]);
		m_numOfVertices += 1;
	}
}

void Renderer::setDrawOffset(int16_t x, int16_t y)
{
	// Force draw for the primitives with the current offset
	draw();

	// Update the uniform value
	glUniform2i(m_program, x, y);
}

void Renderer::draw()
{
	{
		glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, m_numOfVertices);

		// Wait for GPU to complete
		GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

		while (true)
		{
			GLenum status = glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 10000000);
			if (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED)
				break; // Drawing Done
		}

		// Reset the buffers
		m_numOfVertices = 0x0;
	}
}

void Renderer::display()
{
	draw();
	SDL_GL_SwapWindow(m_window);
}
