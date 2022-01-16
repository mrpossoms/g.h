#ifdef GL_ES
precision mediump float;
#endif

in vec4 v_color;
in vec3 v_normal;
in vec4 v_light_proj_pos;

uniform vec3 u_light_diffuse;
uniform vec3 u_light_ambient;
uniform sampler2D u_shadow_map;

out vec4 color;

void main (void)
{
	vec3 normal = normalize(v_normal);
	vec3 light_dir = normalize(v_light_proj_pos.xyz);
	//float bias = 0.00001;
	float bias = 0.00006;//mix(0.00001, 0.0005, dot(v_normal, light_dir));

	float depth = v_light_proj_pos.z - bias;
	float shadowing = 0.0;


	for(float y = -2.0; y <= 2.0; y++)
	for(float x = -2.0; x <= 2.0; x++)
	{
		float sampled_depth = texture(u_shadow_map, v_light_proj_pos.xy + (vec2(x, y) * 0.0005)).r;

		if (depth <= sampled_depth)
		{
			shadowing += 1.0 / 25.0;
		}
	}

	if (abs(dot(normal, light_dir)) < 0.1) { shadowing = 0.0; }

	float ndl = max(0.0, dot(normal, light_dir));// + 1.0) / 2.0;
	float shading = ndl * shadowing;//min(ndl, shadowing);
	// shadowing = max(0.4, shadowing);

	vec4 c_diff = v_color * vec4(u_light_diffuse * shading, 1.0);
	vec4 c_ambi = v_color * vec4(u_light_ambient, 1.0);

	color = v_color * 0.1 + c_ambi + c_diff;

}
