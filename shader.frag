#version 330 core
out vec4 FragColor;

in vec3 color;
uniform float lightness;

void main() {
  FragColor = vec4( lightness * 0.5 + color * 0.5, 1.0 );
}
