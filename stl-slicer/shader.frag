#version 450

layout (location = 0) in vec3 fragColor;

layout (location=0) out vec4 outColor;


void main() {
	if(!gl_FrontFacing) {outColor = vec4(fragColor, 1.0);} else {outColor = vec4(1.0,0.0,0.0,0.0);}
}
