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

float get_distance_cube(vec3 m, vec3 pos)
{
	vec3 uvw = (m - pos) + vec3(0.3535);
	// return distance(m, pos) - 0.5;
	float d = distance(m, pos) - 1.0;
	if (d <= 1.0 + DIST_EPS)
	{
		float r = texture(u_cube, uvw).r;
		return r;// < 1.0 ? 0.0 : r;
	}

	return d;
}

float get_distance(vec3 march_pos)
{
	// TODO
	return min(min(march_pos.y, distance(march_pos, vec3(0.0, 0.5, 4.0)) - 0.5), get_distance_cube(march_pos, vec3(0.0, 2.0, -4.0)));
}

void main(void)
{
	vec4 ray = inverse(u_proj) * vec4(v_screen_pos.xy, 1.0, 1.0);
	ray /= ray.w;

	vec3 d = normalize(v_view_rotation * ray.xyz);
	vec3 o = v_view_pos;

	// color = vec4(0.0, 0.0, 0.0, 1.0);
	// color = ray;

	float net_dist = 0.0;
	for (int t = 0; t < MAX_STEPS; t++)
	{
		vec3 p = o + d * net_dist;
		float step_dist = get_distance(p);

		if (step_dist <= DIST_EPS)
		{
			color = vec4(vec3(net_dist / 10.0), 1.0);
			break;
		}

		net_dist += step_dist; 
	}

	// color.xyz += texture(u_cube, vec3(v_uv, 0.5)).rgb;
}
