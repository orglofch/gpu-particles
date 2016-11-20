// Vertex shader for rendering particles as a depth texture.
// Simply performs pass through.

attribute vec2 index;
//attribute float linehead;

uniform sampler2D positions;
uniform sampler2D velocities;

varying vec3 fragment_position;

void main()
{
	vec4 position = texture2D(positions, index);
	fragment_position = position.xyz;

	gl_Position = vec4(fragment_position, 1.0);
	//vec3 velocity = texture2D(velocities, index).xyz;
	//if (linehead < 0.5) {
	//	gl_Position += vec4(velocity * 0.3, 0.0);
	//}

	gl_Position = gl_ModelViewProjectionMatrix * gl_Position;

	gl_TexCoord[0].st = index;
}