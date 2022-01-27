#ifdef GL_ES
precision mediump float;
#endif

in vec4 v_world_pos;
in vec3 v_normal;
in vec3 v_up;
in vec3 v_left;
in vec3 v_forward;

uniform sampler2D u_wall;
uniform sampler2D u_ground;

out vec4 color;

void main (void)
{
    float w = pow((dot(v_up, v_normal) + 1.0) * 0.5, 8);

    vec3 uvw = v_world_pos.xyz;
    // vec3 uvw = v_world_pos.xyz;
    float w_x = abs(dot(v_normal, vec3(1, 0, 0)));
    float w_y = abs(dot(v_normal, vec3(0, 1, 0)));
    float w_z = abs(dot(v_normal, vec3(0, 0, 1)));

    float sum = w_x + w_y + w_z;
    w_x /= sum;
    w_y /= sum;
    w_z /= sum;

    vec4 ground_color = texture(u_ground, uvw.yz * 0.1) * w_x;
    ground_color += texture(u_ground, uvw.xz * 0.1) * w_y;
    ground_color += texture(u_ground, uvw.xy * 0.1) * w_z;


    vec4 wall_color = texture(u_wall, uvw.yz * 0.1) * w_x;
    wall_color += texture(u_wall, uvw.xz * 0.1) * w_y;
    wall_color += texture(u_wall, uvw.xy * 0.1) * w_z;
    // vec4 wall_color = texture(u_wall, uvw.xy * 0.1) * texture(u_wall, uvw.xz * 0.1);

    color = mix(wall_color, ground_color, w); //vec4(v_normal * 0.5 + 0.5, 1.0);

    color.rgb *= (dot(v_normal, vec3(0, 1, 0)) + 1) * 0.5;
    // color.xyz = vec3(uvw.xz, 0);
}
