#include "pscx_renderer.h"
#include "pscx_common.h"

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

	m_framebufferXResolution = 1024;
	m_framebufferYResolution = 512;

	m_window = SDL_CreateWindow("PSX", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_framebufferXResolution, m_framebufferYResolution, SDL_WINDOW_OPENGL);

	m_glContext = SDL_GL_CreateContext(m_window);

	gladLoadGL();

	SDL_GL_MakeCurrent(m_window, m_glContext);
	
	//glViewport(0, 0, 320, 240);

	// Clear the window
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_SCISSOR_TEST);
	glScissor(0x0, 0x0, (GLint)m_framebufferXResolution, (GLint)m_framebufferYResolution);

	//SDL_GL_SwapWindow(m_window);

	char* vsSrc = loadShaderSource(".\\assets\\vertex.glsl");
	char* fsSrc = loadShaderSource(".\\assets\\fragment.glsl");

	// Compile shaders
	m_vertexShader = compileShader(vsSrc, GL_VERTEX_SHADER);
	m_fragmentShader = compileShader(fsSrc, GL_FRAGMENT_SHADER);

	GLuint shaders[] = { m_vertexShader, m_fragmentShader };

	// Link program
	m_program = linkProgram(shaders);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(m_program);

	// Generate vertex attribute object
	glGenVertexArrays(1, &m_vertexArrayObject);
	glBindVertexArray(m_vertexArrayObject);

	m_vertices.OnCreate();

	// Setup the "position" attribute
	{
		GLuint index = glGetAttribLocation(m_program, "vertex_position");
		glEnableVertexAttribArray(index);
		glVertexAttribIPointer(index, 2, GL_SHORT, sizeof(Vertex),(GLvoid*)offsetof(Vertex, m_position));
	}

	// Setup the "color" attribute
	{
		GLuint index = glGetAttribLocation(m_program, "vertex_color");
		glEnableVertexAttribArray(index);
		glVertexAttribPointer(index, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, m_color));
	}

	// Setup "alpha" attribute
	{
		GLuint index = glGetAttribLocation(m_program, "alpha");
		glEnableVertexAttribArray(index);
		glVertexAttribPointer(index, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, m_alpha));
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

	GLint shaderSize = static_cast<GLint>(strlen(src));
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

void Renderer::pushTriangle(Vertex vertices[])
{
	// Make sure we have enough room left to queue the vertex
	if (m_numOfVertices + 3 > VERTEX_BUFFER_LEN)
		draw();
	
	for (size_t i = 0; i < 3; ++i)
	{
		m_vertices.set(m_numOfVertices, vertices[i]);
		m_numOfVertices += 1;
	}
}

void Renderer::pushQuad(Vertex vertices[])
{
	// Push first triangle
	pushTriangle(vertices);

	// Push second triangle
	pushTriangle(vertices + 1);
}

void Renderer::setDrawOffset(int16_t x, int16_t y)
{
	// Force draw for the primitives with the current offset
	draw();

	// Update the uniform value
	glUniform2i(m_program, x, y);
}

void Renderer::setDrawingArea(uint16_t left, uint16_t top, uint16_t right, uint16_t bottom)
{
	// Render any pending primitives
	draw();

	// Scale PlayStation VRAM coordinates if our framebuffer is not
	// at the native resolution
	GLint leftCoordinate = ((GLint)left * (GLint)m_framebufferXResolution) / 1024;
	GLint rightCoordinate = ((GLint)right * (GLint)m_framebufferXResolution) / 1024;

	GLint topCoordinate = ((GLint)top * (GLint)m_framebufferYResolution) / 512;
	GLint bottomCoordinate = ((GLint)bottom * (GLint)m_framebufferYResolution) / 512;

	// Width and height are inclusive
	GLint width = rightCoordinate - leftCoordinate + 1;
	GLint height = bottomCoordinate - topCoordinate + 1;

	// OpenGL has (0, 0) at the bottom left, the PSX at the top left
	bottomCoordinate = (GLint)m_framebufferYResolution - bottomCoordinate - 1;

	if (width < 0x0 || height < 0x0)
	{
		LOG("Unsupported drawing area " << width << "x" << height << " ["
			<< leftCoordinate << "x" << topCoordinate << " -> "
			<< rightCoordinate << "x" << bottomCoordinate << "]");

		// Don't draw anything
		glScissor(0x0, 0x0, 0x0, 0x0);
	}
	else
	{
		glScissor(leftCoordinate, bottomCoordinate, width, height);
	}
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
