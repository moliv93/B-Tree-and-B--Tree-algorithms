#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 projection;
uniform mat4 view;

uniform vec3 linePoints[3];

void main()
{
    gl_Position = projection * view * vec4(linePoints[gl_VertexID], 1.0);
    // gl_FragColor = paintColor;
}
