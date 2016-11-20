// Fragment shader for rendering particles.

varying float life;
varying vec3 FragmentPosition;
varying vec3 EyeVector;
varying vec3 ShadowCoord;

uniform sampler2D shadowMap;
uniform sampler2D normals;
uniform vec4 global_ambient;

uniform float life_fade;
uniform vec4 light[4];

void main()
{
	vec3 normal = normalize(FragmentPosition);///texture2D(normals, gl_TexCoord[0].st).xyz;

    vec3 lightPos = light[3].xyz;
	// THE BACK STUFF IS CLOSER TO THE LIGHT THAN THE FAR STUFF! z-coordinate mix-up??
	lightPos.z *= -1;

	vec3 fragmentToLight = lightPos - FragmentPosition;
	vec3 lightReflection = normalize(-reflect(fragmentToLight, normal));

	float lightDist = length(fragmentToLight);
	vec3 lightDir = normalize(fragmentToLight);

	gl_FragColor = vec4(0.1, 0.0, 0.0, 1); // Scene color

	float bias = 0.01;
	if (texture2D(shadowMap, ShadowCoord.xy).z >= ShadowCoord.z - bias) {
		// light.falloff.x
		// light.falloff.y
		// light.falloff.z
		float attenuation = 1.0 / (0.01 +
			0.01 * lightDist +
			0.01 * lightDist * lightDist);
		attenuation = 1;//clamp(attenuation, 0.0, 1.0);

		// Ambient
		float ambient = vec3(0.4, 0.0, 0.0);

		// Specular
		float spec_angle = max(dot(lightReflection, normalize(EyeVector)), 0.0);
		float specular = pow(spec_angle, 1) * vec3(0.4, 0.0, 0.0);// intersection.material.shininess);
		specular = clamp(specular, 0.0, 1.0);

		// Diffuse.
		float lambertian = max(dot(normal, lightDir), 0.0);
		float diffuse = lambertian * vec3(0.4, 0.0, 0.0) * attenuation;
		diffuse = clamp(diffuse, 0.0, 1.0);

		gl_FragColor += specular + ambient;
	}

    if (life_fade > 0.5 && life <= 1) {
		//gl_FragColor = gl_FragColor * life;
	}
}