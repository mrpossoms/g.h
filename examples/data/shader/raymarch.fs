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
uniform int u_show;
uniform float u_sub_step;


struct step_t
{
	float distance;
	vec3 color;
};


step_t get_distance_cube(vec3 m, vec3 pos)
{
	const float r = sqrt(3.0) / 2.0; 

	vec3 uvw = ((m - pos)) + vec3(0.5);
	// return distance(m, pos) - 0.5;
	float d = distance(m, pos) - r;
	if (u_show == 1 && d <= DIST_EPS)
	{
		float rd = texture(u_cube, uvw).r;
		return step_t(rd * u_sub_step, uvw);// < 1.0 ? 0.0 : r;
	}

	return step_t(d, uvw);
}

step_t get_distance(vec3 march_pos)
{
	// TODO
	step_t cd = get_distance_cube(march_pos, vec3(0.0, 2.0, -4.0));
	step_t pd = step_t(march_pos.y - 0.5, vec3(march_pos.y));
	// step_t sd = step_t(distance(march_pos, vec3(0.0, 0.5, 4.0)) - 0.5, vec3(distance(march_pos, vec3(0.0, 0.5, 4.0)) - 0.5));
	
	if (cd.distance < pd.distance)
	{
		return cd;
	}
	else
	{
		return pd;
	}
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
		step_t step = get_distance(p);

		if (step.distance <= DIST_EPS)
		{
			color = vec4((step.color + vec3(net_dist / 10.0)), 1.0);
			break;
		}

		net_dist += step.distance; 
	}

	// color.xyz += texture(u_cube, vec3(v_uv, 0.5)).rgb;
}
