#version 410

#define DIST_EPS (0.01)
#define MAX_DIST (1000.0)
#define MAX_STEPS (200)

in vec4 v_screen_pos;

in mat3 v_view_rotation;
in vec3 v_view_pos;

out vec4 color;

uniform mat4 u_proj;
uniform sampler3D u_cube;
uniform int u_show;
uniform float u_sub_step;
uniform vec3 u_light_pos;

float get_distance(vec3 march_pos)
{
	float d = length(march_pos.xz);
	float p = march_pos.y - 0.5 + cos(d);
	// float v = texture(u_cube, vec3(0, 2.0, -5.0) - march_pos).r;
	float s = length(vec3(0, 4.0, -5.0) - march_pos) - 1.0;
	float l = length(u_light_pos - march_pos) - 1.0;

	return min(p, s);
}

/**
 * -------|----X       |
 */

/**
 * @brief      { function_description }
 *
 * @param[in]  vec3  ray origin
 * @param[in]  vec3  ray direction
 * @param[in]  vec3  The vector 3
 * @param[in]  vec3  The vector 3
 *
 * @return     distance along the ray direction where an intersection was detected
 */
float cast_ray( in vec3 ro, in vec3 rd, out vec3 oVos, out vec3 oDir )
{
	vec3 ro_floor = floor(ro);
	vec3 rd_inv = 1.0/rd;
	vec3 rd_sign = sign(rd); // values > 0 become +1, where values < 0 become -1
	vec3 dis = (ro_floor-ro + 0.5 + rd_sign*0.5) * rd_inv; //

	float res = -1.0;
	vec3 mm = vec3(0.0);
	for( int i=0; i<128; i++ )
	{
		if( get_distance(ro_floor)<0.5 )
		{
			res = 1.0;
			break;
		}

		// the step(a, b) function returns 1 when a is less than b. 0 otherwise
		//
		mm = step(dis.xyz, dis.yzx) * step(dis.xyz, dis.zxy);
		dis += mm * rd_sign * rd_inv;
        ro_floor += mm * rd_sign;
	}

	vec3 nor = -mm*rd_sign;
	vec3 vos = ro_floor;

    // intersect the cube
	vec3 mini = (ro_floor-ro + 0.5 - 0.5*vec3(rd_sign))*rd_inv;
	float t = max ( mini.x, max ( mini.y, mini.z ) );

	oDir = mm;
	oVos = vos;

	return t*res;
}

void main(void)
{
	vec4 ray = inverse(u_proj) * vec4(v_screen_pos.xy, 1.0, 1.0);
	ray /= ray.w;

	vec3 d = v_view_rotation * normalize(ray.xyz);
	vec3 ds = sign(d);
	vec3 o = v_view_pos;

	color = vec4(0.0, 0.0, 0.0, 1.0);
	vec3 vos, dir;

	float t = cast_ray(o, d, vos, dir);
	vec3 n = -dir * sign(d);

	vec3 so = o + d * t;
	vec3 ld = normalize(u_light_pos - so);
	float shade = dot(ld, n) * float(t >= 0);

	if (t >= 0)
	{
		float st = cast_ray(so + n*DIST_EPS, ld, vos, dir);

		shade = mix(shade, shade * 0.5, float(st > 0));
	}

	// for (int t = 0; t < MAX_STEPS; t++)
	// {
	// 	float step = get_distance(p);

	// 	p += d * step;

	// 	if (step <= 0.5)
	// 	{
	// 		p = round(p);
	// 		break;
	// 	}
	// }

	vec3 disp_n = (n * 0.5 + 0.5);
	float v = texture(u_cube, o + dir * t).r;
	color = vec4( vec3(1.0) * shade, 1.0);

	// color.xyz += texture(u_cube, vec3(v_uv, 0.5)).rgb;
}
