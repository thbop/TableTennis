#version 330

#define PI 3.141592

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
// in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform int time;

out vec4 finalColor;

float crt_curve = 0.03;

float frac( float v ) {
    return v - floor(v);
}

float random( float v ) {
    return frac( sin(v) * 34809.2361 );
}


void main() {
    vec2 crt_curve_shift = (vec2(1.0) - sin(fragTexCoord.yx * PI)) * crt_curve;
    vec2 crt_curve_scale = vec2(1.0) + crt_curve_shift * 2.0;

    vec2 newUV = fragTexCoord.xy * crt_curve_scale - crt_curve_shift;
    vec4 source = texture(texture0, newUV);

    float radd = random( dot( newUV, vec2(time, 371.62) ) )*0.1;
    if ( newUV.x < 0.0 || newUV.y < 0.0 || newUV.x > 1.0 || newUV.y > 1.0 ) {
        source = vec4( 0.1, 0.1, 0.1, 1.0 );
        radd = 0;
    }

    finalColor = source*vec4(1.0, 1.0, 1.3, 1.0) + radd;
}