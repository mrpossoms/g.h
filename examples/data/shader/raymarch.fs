#version 410

#define DIST_EPS (0.01)
#define MAX_DIST (1000.0)
#define MAX_STEPS (200)

in vec4 v_screen_pos;
in vec2 v_uv;

in mat3 v_view_rotation;
in vec3 v_view_pos;

out vec4 color;

uniform mat4 u_proj;
uniform sampler3D u_cube;

float get_distance(vec3 march_pos)
{
	// TODO
	return min(march_pos.y, distance(march_pos, vec3(0.0, 0.5, 4.0)) - 0.5);
}

void main(void)
{
	vec4 ray = inverse(u_proj) * vec4(v_screen_pos.xy, 1.0, 1.0);
	ray /= ray.w;

	vec3 d = normalize(v_view_rotation * ray.xyz);
	vec3 o = v_view_pos;
	color = vec4((v_screen_pos.xy + vec2(0.5)) + 0.5, 0.0, 0.0);

	// color = vec4(0.0, 0.0, 0.0, 1.0);
	// color = ray;

	float net_dist = 0.0;
	for (int t = 0; t < MAX_STEPS; t++)
	{
		float step_dist = get_distance(o + d * net_dist);

		if (step_dist <= DIST_EPS)
		{
			color = vec4(vec3(net_dist / 10.0), 1.0);
			break;
		}

		net_dist += step_dist; 
	}
}
