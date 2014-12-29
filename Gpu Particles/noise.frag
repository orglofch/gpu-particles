uniform sampler2D uPermTexture;

#define ONE 0.00390625
#define ONEHALF 0.001953125

varying vec3 v_texCoord3D;

float fade(float t) {
  return t*t*t*(t*(t*6.0-15.0)+10.0); 
}
 
float noise(vec3 P)
{
  vec3 Pi = ONE*floor(P)+ONEHALF; 
                                 
  vec3 Pf = P-floor(P);
  
  // Noise contributions from (x=0, y=0), z=0 and z=1
  float perm00 = texture2D(uPermTexture, Pi.xy).a;
  vec3  grad000 = texture2D(uPermTexture, vec2(perm00, Pi.z)).xyz * 4.0 - 1.0;
  float n000 = dot(grad000, Pf);
  vec3  grad001 = texture2D(uPermTexture, vec2(perm00, Pi.z + ONE)).xyz * 4.0 - 1.0;
  float n001 = dot(grad001, Pf - vec3(0.0, 0.0, 1.0));

  // Noise contributions from (x=0, y=1), z=0 and z=1
  float perm01 = texture2D(uPermTexture, Pi.xy + vec2(0.0, ONE)).a;
  vec3  grad010 = texture2D(uPermTexture, vec2(perm01, Pi.z)).xyz * 4.0 - 1.0;
  float n010 = dot(grad010, Pf - vec3(0.0, 1.0, 0.0));
  vec3  grad011 = texture2D(uPermTexture, vec2(perm01, Pi.z + ONE)).xyz * 4.0 - 1.0;
  float n011 = dot(grad011, Pf - vec3(0.0, 1.0, 1.0));

  // Noise contributions from (x=1, y=0), z=0 and z=1
  float perm10 = texture2D(uPermTexture, Pi.xy + vec2(ONE, 0.0)).a;
  vec3  grad100 = texture2D(uPermTexture, vec2(perm10, Pi.z)).xyz * 4.0 - 1.0;
  float n100 = dot(grad100, Pf - vec3(1.0, 0.0, 0.0));
  vec3  grad101 = texture2D(uPermTexture, vec2(perm10, Pi.z + ONE)).xyz * 4.0 - 1.0;
  float n101 = dot(grad101, Pf - vec3(1.0, 0.0, 1.0));

  // Noise contributions from (x=1, y=1), z=0 and z=1
  float perm11 = texture2D(uPermTexture, Pi.xy + vec2(ONE, ONE)).a;
  vec3  grad110 = texture2D(uPermTexture, vec2(perm11, Pi.z)).xyz * 4.0 - 1.0;
  float n110 = dot(grad110, Pf - vec3(1.0, 1.0, 0.0));
  vec3  grad111 = texture2D(uPermTexture, vec2(perm11, Pi.z + ONE)).xyz * 4.0 - 1.0;
  float n111 = dot(grad111, Pf - vec3(1.0, 1.0, 1.0));

  // Blend contributions along x
  vec4 n_x = mix(vec4(n000, n001, n010, n011), vec4(n100, n101, n110, n111), fade(Pf.x));

  // Blend contributions along y
  vec2 n_xy = mix(n_x.xy, n_x.zw, fade(Pf.y));

  // Blend contributions along z
  float n_xyz = mix(n_xy.x, n_xy.y, fade(Pf.z));
 
  return n_xyz;
}

float turbulence(int octaves, vec3 P, float lacunarity, float gain)
{	
  float sum = 0.0;
  float scale = 1.0;
  float totalgain = 1.0;
  for(int i = 0; i < octaves; ++i){
    sum += totalgain*noise(P*scale);
    scale *= lacunarity;
    totalgain *= gain;
  }
  return abs(sum);
}

void main()
{
  float n = noise(vec3(4.0 * gl_TexCoord[0].xyz));
  gl_FragColor = vec4(0.5 + 0.85 * vec3(n, n, n), 1.0);  
}