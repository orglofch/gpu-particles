// Fragment shader for updating particle positions, velocities and lifetimes.

float rand(vec2 co) {
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec4 mod289(vec4 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0; }

float mod289(float x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0; }

vec4 permute(vec4 x) {
     return mod289(((x*34.0)+1.0)*x);
}

float permute(float x) {
     return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

float taylorInvSqrt(float r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

vec4 grad4(float j, vec4 ip)
  {
  const vec4 ones = vec4(1.0, 1.0, 1.0, -1.0);
  vec4 p,s;

  p.xyz = floor( fract (vec3(j) * ip.xyz) * 7.0) * ip.z - 1.0;
  p.w = 1.5 - dot(abs(p.xyz), ones.xyz);
  s = vec4(lessThan(p, vec4(0.0)));
  p.xyz = p.xyz + (s.xyz*2.0 - 1.0) * s.www; 

  return p;
  }
						
// (sqrt(5) - 1)/4 = F4, used once below
#define F4 0.309016994374947451

float snoise(vec4 v)
  {
  const vec4  C = vec4( 0.138196601125011,  // (5 - sqrt(5))/20  G4
                        0.276393202250021,  // 2 * G4
                        0.414589803375032,  // 3 * G4
                       -0.447213595499958); // -1 + 4 * G4

// First corner
  vec4 i  = floor(v + dot(v, vec4(F4)) );
  vec4 x0 = v -   i + dot(i, C.xxxx);

// Other corners

// Rank sorting originally contributed by Bill Licea-Kane, AMD (formerly ATI)
  vec4 i0;
  vec3 isX = step( x0.yzw, x0.xxx );
  vec3 isYZ = step( x0.zww, x0.yyz );
//  i0.x = dot( isX, vec3( 1.0 ) );
  i0.x = isX.x + isX.y + isX.z;
  i0.yzw = 1.0 - isX;
//  i0.y += dot( isYZ.xy, vec2( 1.0 ) );
  i0.y += isYZ.x + isYZ.y;
  i0.zw += 1.0 - isYZ.xy;
  i0.z += isYZ.z;
  i0.w += 1.0 - isYZ.z;

  // i0 now contains the unique values 0,1,2,3 in each channel
  vec4 i3 = clamp( i0, 0.0, 1.0 );
  vec4 i2 = clamp( i0-1.0, 0.0, 1.0 );
  vec4 i1 = clamp( i0-2.0, 0.0, 1.0 );

  //  x0 = x0 - 0.0 + 0.0 * C.xxxx
  //  x1 = x0 - i1  + 1.0 * C.xxxx
  //  x2 = x0 - i2  + 2.0 * C.xxxx
  //  x3 = x0 - i3  + 3.0 * C.xxxx
  //  x4 = x0 - 1.0 + 4.0 * C.xxxx
  vec4 x1 = x0 - i1 + C.xxxx;
  vec4 x2 = x0 - i2 + C.yyyy;
  vec4 x3 = x0 - i3 + C.zzzz;
  vec4 x4 = x0 + C.wwww;

// Permutations
  i = mod289(i); 
  float j0 = permute( permute( permute( permute(i.w) + i.z) + i.y) + i.x);
  vec4 j1 = permute( permute( permute( permute (
             i.w + vec4(i1.w, i2.w, i3.w, 1.0 ))
           + i.z + vec4(i1.z, i2.z, i3.z, 1.0 ))
           + i.y + vec4(i1.y, i2.y, i3.y, 1.0 ))
           + i.x + vec4(i1.x, i2.x, i3.x, 1.0 ));

// Gradients: 7x7x6 points over a cube, mapped onto a 4-cross polytope
// 7*7*6 = 294, which is close to the ring size 17*17 = 289.
  vec4 ip = vec4(1.0/294.0, 1.0/49.0, 1.0/7.0, 0.0) ;

  vec4 p0 = grad4(j0,   ip);
  vec4 p1 = grad4(j1.x, ip);
  vec4 p2 = grad4(j1.y, ip);
  vec4 p3 = grad4(j1.z, ip);
  vec4 p4 = grad4(j1.w, ip);

// Normalise gradients
  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;
  p4 *= taylorInvSqrt(dot(p4,p4));

// Mix contributions from the five corners
  vec3 m0 = max(0.6 - vec3(dot(x0,x0), dot(x1,x1), dot(x2,x2)), 0.0);
  vec2 m1 = max(0.6 - vec2(dot(x3,x3), dot(x4,x4)            ), 0.0);
  m0 = m0 * m0;
  m1 = m1 * m1;
  return 49.0 * ( dot(m0*m0, vec3( dot( p0, x0 ), dot( p1, x1 ), dot( p2, x2 )))
               + dot(m1*m1, vec2( dot( p3, x3 ), dot( p4, x4 ) ) ) ) ;

  }

vec3 snoiseVec3(vec4 x) {
  float s  = snoise(vec4(x));
  float s1 = snoise(vec4(x.y - 19.1, x.z + 33.4, x.x + 47.2, x.w));
  float s2 = snoise(vec4(x.z + 74.2, x.x - 124.5, x.y + 99.4, x.w));
  vec3 c = vec3(s, s1, s2);
  return c;
}

vec4 curl(vec4 p)
{
  const float epsilon = 0.001;
  vec4 dx = vec4(epsilon, 0.0, 0.0, 0.0);
  vec4 dy = vec4(0.0, epsilon, 0.0, 0.0);
  vec4 dz = vec4(0.0, 0.0, epsilon, 0.0);

  vec3 x0 = snoiseVec3(p - dx).xyz;
  vec3 x1 = snoiseVec3(p + dx).xyz;
  vec3 y0 = snoiseVec3(p - dy).xyz;
  vec3 y1 = snoiseVec3(p + dy).xyz;
  vec3 z0 = snoiseVec3(p - dz).xyz;
  vec3 z1 = snoiseVec3(p + dz).xyz;

  float x = y1.z - y0.z - z1.y + z0.y;
  float y = z1.x - z0.x - x1.z + x0.z;
  float z = x1.y - x0.y - y1.x + y0.x;

  const float divisor = 1.0 / (2.0 * epsilon);
  return vec4(normalize(vec3(x, y, z) * divisor), 1.0);
}

vec3 perturbance(sampler2D noise, vec4 pos, vec2 id) 
{
  return mix(mix(mix(texture2D(noise, id).xyz, texture2D(noise, pos.xy).xyz, 0.5), texture2D(noise, pos.yz), 0.05), texture2D(noise, pos.xz), 0.5) / 10000.0;
}

uniform sampler2D positions;
uniform sampler2D velocities;
uniform sampler2D normals;

uniform vec2 mouse_position;
uniform float mouse_down;

uniform float curl_noise;
uniform float curl_perturbance;

uniform float lift;
uniform float drag;
uniform float decay;

uniform float time;

float PI = 3.1415926535897932384626433832795;

void main()
{
	vec4 position = texture2D(positions, gl_TexCoord[0].st);
	vec3 velocity = texture2D(velocities, gl_TexCoord[0].st).xyz;
	vec3 normal = texture2D(normals, gl_TexCoord[0].st).xyz;

	if (mouse_down > 0.5) {
		vec3 vecToMouse = position.xyz - vec3(mouse_position, 0.0);
		float vecToMouseDistance = length(vecToMouse);
		vec3 normVecToMouse = normalize(vecToMouse);
		velocity -= normVecToMouse * min(0.001, 1.0 / (vecToMouseDistance * vecToMouseDistance * vecToMouseDistance) / 1000);
	} 

	if (curl_noise > 0.5) {
		velocity += curl(vec4(position.xyz * 3, time)).xyz / 1000;
	}

	velocity += vec3(0.0, lift, 0.0);
	velocity *= drag;

	position.w -= decay;
	// Reset if 0 life
	if (position.w < 0) {
		position.w = rand(position.xy);
		position.x = mouse_position.x + rand(position.xy) / 50;
		position.y = mouse_position.y + rand(position.yz) / 50;
		position.z = 0.0;

		float theta = rand(vec2(position.x, time)) * 2.0 * PI;

		float z = rand(vec2(position.y, time)) * 2.0 - 1.0;
		float radius = 0.002;
		velocity.x = radius * sqrt(1 - pow(z, 2)) * cos(theta);
		velocity.y = radius * sqrt(1 - pow(z, 2)) * sin(theta);
		velocity.z = radius * z;
	}

	// Update textures
	gl_FragData[0] = position + vec4(velocity, 0.0);
	gl_FragData[1] = vec4(velocity, 0.0);
	gl_FragData[2] = vec4(normal, 0.0);
}