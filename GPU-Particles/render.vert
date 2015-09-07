attribute vec2 aIndex;
attribute float linehead;

uniform sampler2D uPositions;
uniform sampler2D uVelocities;

varying float vLife;

void main()
{
	vec4 position = texture2D(uPositions, aIndex);
	vLife = position.w;

	gl_Position = vec4(position.xyz, 1.0);
	vec3 velocity = texture2D(uVelocities, aIndex).xyz;
	if (linehead < 0.5) {
		gl_Position += vec4(velocity * 0.3, 0.0);
	}
	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Position;

	gl_TexCoord[0].st = aIndex;
}