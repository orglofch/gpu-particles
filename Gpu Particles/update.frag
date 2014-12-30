uniform sampler2D uPositions;
uniform sampler2D uVelocities;
uniform vec2 uMousePosition;
uniform float uMouseDown;

varying vec4 vPosition;
varying vec4 vVelocity;

void main()
{
	vec4 position = texture2D(uPositions, gl_TexCoord[0].st);
	vec3 velocity = texture2D(uVelocities, gl_TexCoord[0].st).xyz;
	velocity *= 0.97; // entropy

	gl_FragData[0] = position + vec4(velocity, 0.0);
	gl_FragData[1] = vec4(velocity, 0.0);

	if (uMouseDown > 0.5) {
		vec3 vecToMouse = position.xyz - vec3(uMousePosition, 0.0);
		float vecToMouseDistance = length(vecToMouse);
		vec3 normVecToMouse = normalize(vecToMouse);
		gl_FragData[1].xyz -= normVecToMouse * min(0.008, 1.0 / (vecToMouseDistance * vecToMouseDistance * vecToMouseDistance) / 500);
	} 
}