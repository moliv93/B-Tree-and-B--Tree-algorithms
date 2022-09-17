#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec2 TexCoord;

uniform vec2 transform;
uniform mat4 projection;
uniform mat4 view;

void main()
{
	gl_Position = projection * view * (vec4(aPos+vec3(transform, 0), 1.0));
	// gl_Position = (vec4(aPos, 1.0));
	TexCoord = vec2(aTexCoord.x, aTexCoord.y);
}
