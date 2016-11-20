// Vertex shader for rendering particles.

attribute vec2 index;
//attribute float linehead;

uniform sampler2D positions;
uniform sampler2D velocities;

uniform mat4 lightMVP;
uniform mat4 lightBias;

varying float life;
varying vec3 FragmentPosition;
varying vec3 EyeVector;
varying vec3 ShadowCoord;

void main()
{
	vec4 position = texture2D(positions, index);
	FragmentPosition = position.xyz;
	life = position.w;

	ShadowCoord = (lightBias * lightMVP) * vec4(FragmentPosition, 1);
	EyeVector = normalize(gl_ModelViewMatrix * vec4(0, 0, 0, 1) - gl_ModelViewMatrix * vec4(FragmentPosition, 1));

	gl_Position = vec4(FragmentPosition, 1.0);
	//vec3 velocity = texture2D(velocities, index).xyz;
	//if (linehead < 0.5) {
	//	gl_Position += vec4(velocity * 0.3, 0.0);
	//}
	gl_Position = gl_ModelViewProjectionMatrix * gl_Position;

	gl_TexCoord[0].st = index;
}