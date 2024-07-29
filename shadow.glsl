#version 330
// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
// in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform sampler2D mask;

out vec4 finalColor;

vec2 pix = gl_FragCoord.xy / vec2(200, -192);

void main() {
    vec4 m = texture(mask, pix);

    finalColor = vec4(0.0, 0.0, 0.0, m.x);
}