// Vertex shader for updating particles.
// Performs a simple pass-through, converting an index to a position.

attribute vec2 index;

void main()
{
	gl_Position.xy = index * 2.0 - vec2(1.0, 1.0);
	gl_Position.zw = vec2(0.0, 1.0);

	gl_TexCoord[0].st = index;
}