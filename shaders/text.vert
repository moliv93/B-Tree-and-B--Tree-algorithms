#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform vec4 crop;
uniform vec2 transform;

// layout (location = 0) uniform mat4 projection;
// layout (location = 1) uniform mat4 view;
// layout (location = 2) uniform vec4 crop;
// layout (location = 3) uniform vec2 transform;

void main()
{
	gl_Position = projection * view * (vec4(aPos+vec3(transform, 0), 1.0));
	TexCoord = vec2(crop.x + (aTexCoord.x*crop.z), crop.y + (aTexCoord.y * crop.w));
	// TexCoord = vec2(aTexCoord.x/crop.z + crop.x, aTexCoord.y/crop.w + crop.y);
}
