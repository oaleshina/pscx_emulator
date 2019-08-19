#version 450

// Drawing offset
uniform ivec2 offset;

in ivec2 vertex_position;
in vec3 vertex_color;

out vec3 color;

void main()
{
	ivec2 position = vertex_position + offset;

	// Convert VRAM coordinates (0; 1023, 0; 511) into
	// OpenGL coordinates (-1; 1, -1; 1)
	float xpos = (float(position.x) / 320.0) - 1.0;

	// VRAM puts 0 at the top, OpenGL at the bottom
	// just mirror it vertically
	float ypos = 1.0 - (float(position.y) / 240.0);

	gl_Position = vec4(xpos, ypos, 0.0, 1.0);

	// Convert the components from [0; 255] to [0; 1]
	color = vertex_color;
}
