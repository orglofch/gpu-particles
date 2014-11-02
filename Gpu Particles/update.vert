attribute vec2 aIndex;

void main()
{
	gl_Position.xy = aIndex * 2.0 - vec2(1.0, 1.0);
	gl_Position.zw = vec2(0.0, 1.0);

	gl_TexCoord[0].st = aIndex;
}