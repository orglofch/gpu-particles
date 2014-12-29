attribute float linehead;
attribute vec2 aIndex;

uniform sampler2D uPositions;
uniform sampler2D uVelocities;

void main()
{
	gl_Position = texture2D(uPositions, aIndex);
	vec3 velocity = texture2D(uVelocities, aIndex).xyz;
	if (linehead == 0.0) {
		gl_Position += vec4(velocity / 8, 0.0);
	}
	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Position;

	gl_TexCoord[0].st = aIndex;
}