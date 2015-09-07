varying float vLife;

void main()
{
    gl_FragColor = vec4(vLife / 5, vLife / 20, vLife / 100, 1.0f);
}