#ifdef GL_ES
precision mediump float;
#endif

in vec4 v_world_pos;
in vec3 v_normal;
in vec3 v_tangent;
in vec3 v_up;
in mat3 v_basis;

uniform sampler2D u_wall;
uniform sampler2D u_wall_normal;
uniform sampler2D u_ground;
uniform sampler2D u_ground_normal;
uniform float u_time;

out vec4 color;

void main (void)
{
    float w = pow((dot(v_up, v_normal) + 1.0) * 0.5, 8);

    vec3 light_dir = vec3(cos(u_time), 0, sin(u_time));

    vec3 uvw = v_world_pos.xyz;
    // vec3 uvw = v_world_pos.xyz;
    float w_x = abs(dot(v_normal, vec3(1, 0, 0)));
    float w_y = abs(dot(v_normal, vec3(0, 1, 0)));
    float w_z = abs(dot(v_normal, vec3(0, 0, 1)));

    float sum = w_x + w_y + w_z;
    w_x /= sum;
    w_y /= sum;
    w_z /= sum;

    vec4 ground_color = texture(u_ground, uvw.yz * 0.4) * w_x;
    ground_color += texture(u_ground, uvw.xz * 0.4) * w_y;
    ground_color += texture(u_ground, uvw.xy * 0.4) * w_z;

    vec3 ground_normal = texture(u_ground_normal, uvw.yz * 0.4).rbg * w_x;
    ground_normal += texture(u_ground_normal, uvw.xz * 0.4).rbg * w_y;
    ground_normal += texture(u_ground_normal, uvw.xy * 0.4).rbg * w_z;


    vec4 wall_color = texture(u_wall, uvw.yz * 0.4) * w_x;
    wall_color += texture(u_wall, uvw.xz * 0.4) * w_y;
    wall_color += texture(u_wall, uvw.xy * 0.4) * w_z;

    vec3 wall_normal = texture(u_wall_normal, uvw.yz * 0.4).rbg * w_x;
    wall_normal += texture(u_wall_normal, uvw.xz * 0.4).rbg * w_y;
    wall_normal += texture(u_wall_normal, uvw.xy * 0.4).rbg * w_z;
    // vec4 wall_color = texture(u_wall, uvw.xy * 0.1) * texture(u_wall, uvw.xz * 0.1);

    vec3 textel_norm = (normalize(mix(wall_normal, ground_normal, w)) - 0.5) * 2.0;
    //vec3 textel_norm = vec3(0, 1, 0);//(normalize(mix(wall_normal, ground_normal, w)) - 0.5) * 2.0;

    // color = vec4(1.0);
    color = mix(wall_color, ground_color, w);

    color.rgb *= (dot(v_basis * textel_norm, light_dir) + 1) * 0.5;
    // color.rgb *= dot(v_normal, light_dir);
    // color.rgb = (textel_norm + 1.0) * 0.5;
}
