#version 450

layout (location = 0) in vec3 fragColor;

layout (location=0) out vec4 outColor;

layout (push_constant) uniform Push {
	mat4 transform; // projection * view * model
	mat4 modelMatrix;
} push;

void main() {
        if(!gl_FrontFacing) {outColor = vec4(fragColor.x,fragColor.y,fragColor.z,1.0);}
        else {outColor = vec4(0.0,0.0,0.0,1.0);}

}
