

#pragma once
#define XMTYPE float
#include <xmath.h>
#include <g.game.h>
#include <g.camera.h>
#include <g.proc.h>

#include <iostream>
#include <unordered_map>
#include <vector>
#include <set>
#include <algorithm>

#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef _WIN32
#include <gl/glew.h>
#include <gl/GL.h>
#include <io.h>
#define open _open
#define lseek _lseek
#endif

#ifdef __linux__
#include <unistd.h>
#include <GL/glew.h>
#include <GL/glx.h>
#include <GL/glext.h>
#include <GL/gl.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <unistd.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#include <GL/glew.h>
#include <GL/glext.h>
#include <GL/gl.h>
#endif

#ifdef __APPLE__
#include <unistd.h>
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>


#include <ogt_vox.h>
#include <ogt_voxel_meshify.h>

using namespace xmath;

namespace g {
namespace gfx {

static bool gl_get_error()
{
	GLenum err = GL_NO_ERROR;
	bool good = true;

	while((err = glGetError()) != GL_NO_ERROR)
	{
		std::cerr << "GL_ERROR: 0x" << std::hex << err << std::endl;
		good = false;
	}

	return good;
}

extern GLFWwindow* GLFW_WIN;

bool has_graphics();

size_t width();

size_t height();

float aspect();

namespace noise
{

float perlin(const vec<3>& p, const std::vector<int8_t>& entropy);

float value(const vec<3>& p, const std::vector<int8_t>& entropy);

} // namespace noise

struct texture
{
	GLenum type;
	unsigned component_count = 0;
	unsigned bytes_per_component = 0;
	unsigned size[3] = { 1, 1, 1 };
	GLuint hnd = -1;
	unsigned char* data = nullptr;

	inline bool is_initialized() const { return hnd != (unsigned)-1; }

	void release_bitmap();

	void create(GLenum texture_type);

	void destroy();

	void set_pixels(size_t w, size_t h, size_t d, unsigned char* data, GLenum color_type=GL_RGBA, GLenum storage=GL_UNSIGNED_BYTE);

	unsigned char* sample(unsigned x, unsigned y=0, unsigned z=0) const
	{
		const unsigned textel_stride = bytes_per_component * component_count;

		const unsigned x_stride = 1;
		const unsigned y_stride = size[0];
		const unsigned z_stride = size[0] * size[1];

		return &data[(x * x_stride + y * y_stride + z * z_stride) * textel_stride];
	}

	inline bool is_1D() const { return size[0] > 1 && size[1] == 1 && size[2] == 1; }
	inline bool is_2D() const { return size[0] > 1 && size[1] > 1 && size[2] == 1; }
	inline bool is_3D() const { return size[0] > 1 && size[1] > 1 && size[2] > 1; }

	void bind() const;

	void unbind() const;
};


struct texture_factory
{
	texture* existing = nullptr;
	unsigned size[3] = {1, 1, 1};
	unsigned char* data = nullptr;
	GLenum texture_type;
	GLenum min_filter = GL_LINEAR, mag_filter = GL_LINEAR;
	GLenum wrap_s = GL_CLAMP_TO_EDGE, wrap_t = GL_CLAMP_TO_EDGE, wrap_r = GL_CLAMP_TO_EDGE;
	GLenum color_type = GL_RGBA;
	GLenum storage_type = GL_UNSIGNED_BYTE;
	unsigned component_count = 0;
	unsigned bytes_per_component = 0;

	explicit texture_factory() = default;

	explicit texture_factory(unsigned w, unsigned h);

	explicit texture_factory(unsigned w, unsigned h, unsigned d);

	explicit texture_factory(texture* existing_texture);

	void abort(std::string message);

	texture_factory& from_png(const std::string& path);

	texture_factory& to_png(const std::string& path);

	texture_factory& type(GLenum t);

	texture_factory& components(unsigned count);

	texture_factory& color();

	texture_factory& depth();

	texture_factory& pixelated();

	texture_factory& smooth();

	texture_factory& clamped();

	texture_factory& clamped_to_border();

	texture_factory& repeating();

	texture_factory& fill(std::function<void(int x, int y, int z, unsigned char* pixel)> filler);

	texture_factory& fill(unsigned char* buffer);

	texture create();
};


struct framebuffer
{
	struct scoped_draw
	{
		framebuffer& fb_ref;

		scoped_draw(framebuffer& fb) : fb_ref(fb) { fb_ref.bind_as_target(); }
		~scoped_draw() { fb_ref.unbind_as_target(); }
	};

	GLuint fbo;
	unsigned size[3];
	texture color;
	texture depth;

	void bind_as_target()
	{
		glViewport(0, 0, size[0], size[1]);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		//assert(gl_get_error());
	}

	void unbind_as_target()
	{
		if (color.hnd != (GLuint)-1)
		{
			color.bind();
			glGenerateMipmap(GL_TEXTURE_2D);
		}

		glViewport(0, 0, g::gfx::width(), g::gfx::height());
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
};


struct framebuffer_factory
{
	unsigned size[2] = {1, 1};
	texture color_tex, depth_tex;

	framebuffer_factory(unsigned w, unsigned h);

	framebuffer_factory(texture& dst);

	framebuffer_factory();

	framebuffer_factory& color();

	framebuffer_factory& depth();

	framebuffer_factory& shadow_map();

	framebuffer create();
};

/**
 * @brief      { struct_description }
 */
struct shader
{
	GLuint program = 0;
	std::unordered_map<std::string, GLint> uni_locs;

	inline bool is_initialized() const { return program != 0; }

	void destroy();

	shader& bind();

	struct uniform_usage;
	/**
	 * @brief      shader::usage type represents the start of some invocation
	 * of an interaction with a shader.
	 */
	struct usage
	{
		shader* shader_ref = nullptr;
		size_t vertices = 0, indices = 0;
		int texture_unit = 0;

		usage() = default;
		usage (shader* ref, size_t verts, size_t inds);

		template<typename MV>
		usage attach_attributes(const shader& shader)
		{
			//assert(gl_get_error());
			MV::attributes(shader.program);
			//assert(gl_get_error());
			return *this;
		}

		usage set_camera(g::game::camera& cam);

		uniform_usage set_uniform(const std::string& name);

		uniform_usage operator[](const std::string& name);

		template<GLenum PRIM>
		usage& draw()
		{
			//assert(gl_get_error());
			if (indices > 0)
			{
				glDrawElements(PRIM, indices, GL_UNSIGNED_INT, NULL);
				//assert(gl_get_error());
			}
			else
			{
				glDrawArrays(PRIM, 0, vertices);
				//assert(gl_get_error());
			}

			return *this;
		}

		usage& draw_tri_fan()
		{
			return draw<GL_TRIANGLE_FAN>();
		}
	};

	/**
	 * @brief      Offers interaction with the uniforms defined for a given shader
	 */
	struct uniform_usage
	{
		GLuint uni_loc;
		usage& parent_usage;

		uniform_usage(usage& parent, GLuint loc);

		usage mat4 (const mat<4, 4>& m);

		usage mat3 (const mat<3, 3>& m);

		usage vec2 (const vec<2>& v);
		usage vec2n (const vec<2>* v, size_t count);

		usage vec3 (const vec<3>& v);
		usage vec3n (const vec<3>* v, size_t count);

		usage vec4(const vec<4>& v);

		usage flt(float f);
		usage fltn(float* f, size_t count);

		usage int1(const int i);

		usage texture(const texture& tex);
	};
};


struct shader_factory
{
	std::unordered_map<GLenum, GLuint> shaders;

	static std::string shader_header;

	static GLuint compile_shader (GLenum type, const GLchar* src, GLsizei len);

	template<GLenum ST>
	shader_factory add(const std::string& path)
	{
		auto fd = ::open(path.c_str(), O_RDONLY);
		auto size = ::lseek(fd, 0, SEEK_END);

		if (fd < 0 || size <= 0)
		{
			std::cerr << G_TERM_RED << "Could not open: '" << path << "' file did not exist, or was empty" << std::endl;
			return *this;
		}

		{ // read and compile the shader
			GLchar* src = new GLchar[size];
			::lseek(fd, 0, SEEK_SET);
			size = ::read(fd, src, size);

			std::cerr << "Compiling: " << path << "... ";

			auto src_str = shader_header + std::string(src, size);
			shaders[ST] = compile_shader(ST, src_str.c_str(), (GLsizei)src_str.length() - 1);

			::close(fd);
			delete[] src;
		}

		return *this;
	}

	template<GLenum ST>
	shader_factory add_src(const std::string& src)
	{
		{ // read and compile the shader
			auto src_str = shader_header + src;
			shaders[ST] = compile_shader(ST, src_str.c_str(), (GLsizei)src_str.length());
		}

		return *this;
	}

	shader create();
};


namespace vertex
{
	struct pos
	{
		vec<3> position;

		static void attributes(GLuint prog)
		{
			auto pos_loc = glGetAttribLocation(prog, "a_position");

			if (pos_loc > -1) glEnableVertexAttribArray(pos_loc);
			if (pos_loc > -1) glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, sizeof(pos), (void*)0);
		}
	};

	struct pos_uv
	{
		vec<3> position;
		vec<2> uv;

		static void attributes(GLuint prog)
		{
			auto pos_loc = glGetAttribLocation(prog, "a_position");
			auto uv_loc = glGetAttribLocation(prog, "a_uv");

			if (pos_loc > -1) glEnableVertexAttribArray(pos_loc);
			if (uv_loc > -1) glEnableVertexAttribArray(uv_loc);

			auto p_size = sizeof(position);

			if (pos_loc > -1) glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, sizeof(pos_uv), (void*)0);
			if (uv_loc > -1) glVertexAttribPointer(uv_loc, 2, GL_FLOAT, false, sizeof(pos_uv), (void*)p_size);
		}
	};

	struct pos_norm
	{
		vec<3> position;
		vec<3> normal;

		static void attributes(GLuint prog)
		{
			auto pos_loc = glGetAttribLocation(prog, "a_position");
			auto norm_loc = glGetAttribLocation(prog, "a_normal");

			if (pos_loc > -1) glEnableVertexAttribArray(pos_loc);
			if (norm_loc > -1) glEnableVertexAttribArray(norm_loc);

			auto p_size = sizeof(position);

			if (pos_loc > -1) glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, sizeof(pos_norm), (void*)0);
			if (norm_loc > -1) glVertexAttribPointer(norm_loc, 3, GL_FLOAT, false, sizeof(pos_norm), (void*)(p_size));
		}
	};

	struct pos_uv_norm
	{
		vec<3> position;
		vec<2> uv;
		vec<3> normal;

		static void attributes(GLuint prog)
		{
			auto pos_loc = glGetAttribLocation(prog, "a_position");
			auto uv_loc = glGetAttribLocation(prog, "a_uv");
			auto norm_loc = glGetAttribLocation(prog, "a_normal");

			if (pos_loc > -1) glEnableVertexAttribArray(pos_loc);
			if (uv_loc > -1) glEnableVertexAttribArray(uv_loc);
			if (norm_loc > -1) glEnableVertexAttribArray(norm_loc);

			auto p_size = sizeof(position);
			auto uv_size = sizeof(uv);

			if (pos_loc > -1) glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, sizeof(pos_uv_norm), (void*)0);
			if (uv_loc > -1) glVertexAttribPointer(uv_loc, 2, GL_FLOAT, false, sizeof(pos_uv_norm), (void*)p_size);
			if (norm_loc > -1) glVertexAttribPointer(norm_loc, 3, GL_FLOAT, false, sizeof(pos_uv_norm), (void*)(p_size + uv_size));
		}
	};

	struct pos_norm_tan
	{
		vec<3> position;
		vec<3> normal;
		vec<3> tangent;

		static void attributes(GLuint prog)
		{
			auto pos_loc = glGetAttribLocation(prog, "a_position");
			auto norm_loc = glGetAttribLocation(prog, "a_normal");
			auto tan_loc = glGetAttribLocation(prog, "a_tangent");

			if (pos_loc > -1) glEnableVertexAttribArray(pos_loc);
			if (norm_loc > -1) glEnableVertexAttribArray(norm_loc);
			if (tan_loc > -1) glEnableVertexAttribArray(tan_loc);

			auto p_size = sizeof(position);
			auto norm_size = sizeof(normal);

			if (pos_loc > -1) glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, sizeof(pos_norm_tan), (void*)0);
			if (norm_loc > -1) glVertexAttribPointer(norm_loc, 3, GL_FLOAT, false, sizeof(pos_norm_tan), (void*)p_size);
			if (tan_loc > -1) glVertexAttribPointer(tan_loc, 3, GL_FLOAT, false, sizeof(pos_norm_tan), (void*)(p_size + norm_size));
		}
	};

	struct pos_norm_color
	{
		vec<3> position;
		vec<3> normal;
		vec<4, uint8_t> color;

		static void attributes(GLuint prog)
		{
			auto pos_loc = glGetAttribLocation(prog, "a_position");
			auto norm_loc = glGetAttribLocation(prog, "a_normal");
			auto color_loc = glGetAttribLocation(prog, "a_color");

			if (pos_loc > -1) glEnableVertexAttribArray(pos_loc);
			if (norm_loc > -1) glEnableVertexAttribArray(norm_loc);
			if (color_loc > -1) glEnableVertexAttribArray(color_loc);

			auto p_size = sizeof(position);
			auto n_size = sizeof(normal);

			if (pos_loc > -1) glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, sizeof(pos_norm_color), (void*)0);
			if (norm_loc > -1) glVertexAttribPointer(norm_loc, 3, GL_FLOAT, false, sizeof(pos_norm_color), (void*)(p_size));
			if (color_loc > -1) glVertexAttribPointer(color_loc, 4, GL_UNSIGNED_BYTE, false, sizeof(pos_norm_color), (void*)(p_size + n_size));
		}
	};
}


template<typename V>
struct mesh
{
	GLuint vbo = 0, ibo = 0;
	size_t index_count = 0;
	size_t vertex_count = 0;


	inline bool is_initialized() const { return vbo != 0; }

	void destroy()
	{
		if (GL_TRUE == glIsBuffer(vbo))
		{
			glDeleteBuffers(1, &vbo);
			vbo = 0;
		}

		if (GL_TRUE == glIsBuffer(ibo))
		{
			glDeleteBuffers(1, &ibo);
			ibo = 0;
		}
	}

	mesh& set_vertices(const std::vector<V>& verts)
	{
		return set_vertices(verts.data(), verts.size());
	}

	mesh& set_vertices(const V* verts, size_t count)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		//assert(gl_get_error());
		vertex_count = count;
		//assert(gl_get_error());
		glBufferData(
			GL_ARRAY_BUFFER,
			count * sizeof(V),
			verts,
			GL_STATIC_DRAW
		);
		//assert(gl_get_error());

		return *this;
	}

	mesh& set_indices(const std::vector<uint32_t>& inds)
	{
		return set_indices(inds.data(), inds.size());
	}

	mesh& set_indices(const uint32_t* inds, size_t count)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		//assert(gl_get_error());
		assert(GL_TRUE == glIsBuffer(ibo));
		index_count = count;
		glBufferData(
			GL_ELEMENT_ARRAY_BUFFER,
			count * sizeof(uint32_t),
			inds,
			GL_STATIC_DRAW
		);
		//assert(gl_get_error());

		return *this;
	}

	shader::usage using_shader (shader& shader) const
	{
		//assert(gl_get_error());
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		//assert(gl_get_error());

		if (index_count > 0)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
			//assert(gl_get_error());
		}

		shader.bind();
		shader::usage usage = {&shader, vertex_count, index_count};
		usage.attach_attributes<V>(shader);

		// unbind any previously bound textures to prevent
		// unexpected behavior
		glBindTexture(GL_TEXTURE_1D, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindTexture(GL_TEXTURE_3D, 0);

		return usage;
	}


	static void compute_normals(std::vector<V>& vertices, const std::vector<uint32_t>& indices)
	{
		for(unsigned i = 0; i < indices.size(); i += 3)
		{
			vec<3> diff[2] = {
				vertices[indices[i + 0]].position - vertices[indices[i + 1]].position,
				vertices[indices[i + 0]].position - vertices[indices[i + 2]].position,
			};

			auto& vert = vertices[indices[i]];
			auto cross = vec<3>::cross(diff[0], diff[1]);
			vert.normal = cross.unit();

			if(isnan(vert.normal[0]) || isnan(vert.normal[1]) || isnan(vert.normal[2]))
			{
				// assert(0);
			}

			i = (i + 1) - 1;
		}
	}


	static void compute_tangents(std::vector<V>& vertices, const std::vector<uint32_t>& indices)
	{
		for(unsigned int i = 0; i < indices.size(); i += 3)
		{

			vertices[indices[i + 0]].tangent = vertices[indices[i]].position - vertices[indices[i + 1]].position;

			for(int j = 3; j--;)
			{
				vertices[indices[i + j]].tangent = vertices[indices[i + j]].tangent.unit();
			}
		}
	}

	void from_sdf_r(
		std::vector<V>& vertices_out,
		std::vector<uint32_t>& indices_out,
		const g::game::sdf& sdf, 
		std::function<V (const g::game::sdf& sdf, const vec<3>& pos)> generator, 
		vec<3> volume_corners[2],
		unsigned max_depth=4)
	{
		#include "data/marching.cubes.lut"

		vertices_out.clear();
		indices_out.clear();

		std::function<int(vec<3> corners[2], unsigned)> subdivider = [&](vec<3> corners[2], unsigned depth)
		{
			vec<3> c[8] = {
				{ 0.f, 0.f, 0.f },
				{ 0.f, 1.f, 0.f },
				{ 1.f, 1.f, 0.f },
				{ 1.f, 0.f, 0.f },

				{ 0.f, 0.f, 1.f },
				{ 0.f, 1.f, 1.f },
				{ 1.f, 1.f, 1.f },
				{ 1.f, 0.f, 1.f },
			};
			auto& p0 = corners[0];
			auto& p1 = corners[1];
			auto block_delta = (p1 - p0);
			auto mid = p0 + block_delta * 0.5f;

			// compute positions of the corners for voxel x,y,z as well
			// as the case for the voxel
			uint8_t voxel_case = 0;
			vec<3> p[8];  // voxel corners
			float d[8]; // densities at each corner
			float avg_density = 0;

			for (int i = 8; i--;)
			{
				c[i] *= block_delta;
				p[i] = p0 + c[i];
				d[i] = sdf(p[i]);
				avg_density += d[i];

				// if d[i] >= 0
				voxel_case |= ((d[i] >= 0) << i);
			}				

			// std::cerr << "voxel_case: " << static_cast<int>(voxel_case) << std::endl;

			if (depth > 0)
			{ // test for density function boundaries
				int verts_generated = 0;

				avg_density += sdf(mid);
				avg_density /= 9.f;

				//if ((voxel_case != 0 && voxel_case != 255))// || d_mid >= 0)
				if (fabs(avg_density) < 200)
				{
					for (int i = 8; i--;)
					{
						//if (d[i] > 0  && d_mid > 0) { continue; }

						vec<3> next_corners[2];

						next_corners[0] = p[i];
						next_corners[0].take_min(mid);
						next_corners[1] = p[i];
						next_corners[1].take_max(mid);

						verts_generated += subdivider(next_corners, depth - 1);
					}

					if (verts_generated == 0) { verts_generated = subdivider(corners, 0); }
				}

				return verts_generated;
			}
			else
			{ // time to generate geometry
			// compute lerp weights between verts for each edge
				int verts_generated = 0;
				if (voxel_case == 0 || voxel_case == 255) { return verts_generated; }

				float w[12];
				for (int i = 0; i < 12; ++i)
				{
					int e_i = tri_edge_list_case[voxel_case][i];

					if (e_i == -1) break;

					// v0 * w + v1 * (1 - w)
					// w = 0.5
					// -1 * w + 1 * (1 - w) = 0
					//
					// d0 * w + d1 * (1 - w) = 0
					// d0 * w + d1 - d1 * w = 0
					// (d0 * w - d1 * w) / w = -d1 / w
					// d0 - d1 = -d1 / w
					// -d1 / (d0 - d1) = w

					int p0_i = edge_list[e_i][0];
					int p1_i = edge_list[e_i][1];

					// solve for the weight that will lerp between
					w[i] = d[p0_i] / (d[p0_i] - d[p1_i]);

					vec<3> _p = p[p1_i] * w[i] + p[p0_i] * (1 - w[i]);
					
					indices_out.push_back(vertices_out.size());
					vertices_out.push_back(generator(sdf, _p));
					verts_generated++;
				}

				return verts_generated;
			}

			return 0;
		};

		subdivider(volume_corners, max_depth);
	}

	void from_sdf_r(
		const g::game::sdf& sdf, 
		std::function<V (const g::game::sdf& sdf, const vec<3>& pos)> generator, 
		vec<3> volume_corners[2],
		unsigned max_depth=4)
	{
		#include "data/marching.cubes.lut"

		static std::vector<V> vertices;
		static std::vector<uint32_t> indices;

		vertices.clear();
		indices.clear();

		from_sdf_r(vertices, indices, sdf, generator, volume_corners, max_depth);
		
		set_vertices(vertices);
		set_indices(indices);
	}


	void from_sdf(
		std::vector<V>& vertices_out,
		std::vector<uint32_t>& indices_out,
		const g::game::sdf& sdf, 
		std::function<V (const g::game::sdf& sdf, const vec<3>& pos)> generator, 
		vec<3> corners[2],
		unsigned divisions=32)
	{
		#include "data/marching.cubes.lut"

		vertices_out.clear();
		indices_out.clear();

		float div = divisions;

		auto& p0 = corners[0];
		auto& p1 = corners[1];
		auto block_delta = (p1 - p0);
		auto voxel_delta = block_delta / div;

		for (int x = divisions; x--;)
		for (int y = divisions; y--;)
		for (int z = divisions; z--;)
		{
			vec<3> voxel_index({ (float)x, (float)y, (float)z });
			vec<3> p[8];  // voxel corners
			float d[8]; // densities at each corner
			uint8_t voxel_case = 0;

			vec<3> c[8] = {
				{ 0.f, 0.f, 0.f },
				{ 0.f, 1.f, 0.f },
				{ 1.f, 1.f, 0.f },
				{ 1.f, 0.f, 0.f },

				{ 0.f, 0.f, 1.f },
				{ 0.f, 1.f, 1.f },
				{ 1.f, 1.f, 1.f },
				{ 1.f, 0.f, 1.f },
			};

			// compute positions of the corners for voxel x,y,z as well
			// as the case for the voxel
			for (int i = 8; i--;)
			{
				c[i] *= voxel_delta;
				p[i] = p0 + (voxel_delta * voxel_index) + c[i];
				d[i] = sdf(p[i]);

				voxel_case |= ((d[i] >= 0) << i);
			}

			// compute lerp weights between verts for each edge
			float w[12];
			for (int i = 0; i < 12; ++i)
			{
				int e_i = tri_edge_list_case[voxel_case][i];

				if (e_i == -1) break;

				// v0 * w + v1 * (1 - w)
				// w = 0.5
				// -1 * w + 1 * (1 - w) = 0
				//
				// d0 * w + d1 * (1 - w) = 0
				// d0 * w + d1 - d1 * w = 0
				// (d0 * w - d1 * w) / w = -d1 / w
				// d0 - d1 = -d1 / w
				// -d1 / (d0 - d1) = w

				int p0_i = edge_list[e_i][0];
				int p1_i = edge_list[e_i][1];

				// solve for the weight that will lerp between
				w[i] = d[p0_i] / (d[p0_i] - d[p1_i]);

				vec<3> _p = p[p1_i] * w[i] + p[p0_i] * (1 - w[i]);
				
				indices_out.push_back(vertices_out.size());
				vertices_out.push_back(generator(sdf, _p));
			}

		}
	}

	void from_sdf(
		const g::game::sdf& sdf,
		std::function<V(const g::game::sdf& sdf, const vec<3>& pos)> generator,
		vec<3> corners[2],
		unsigned divisions = 32)
	{
		static std::vector<V> vertices;
		static std::vector<uint32_t> indices;

		from_sdf(vertices, indices, sdf, generator, corners, divisions);

		set_vertices(vertices);
		set_indices(indices);
	}
};


struct mesh_factory
{
	static mesh<vertex::pos_uv_norm> plane(const vec<3>& normal={0, 0, 1}, const vec<2>& size={1, 1});

	static mesh<vertex::pos> slice_cube(unsigned slices);

	static mesh<vertex::pos_uv_norm> cube();

	static mesh<vertex::pos_uv_norm> from_obj(const std::string& path);

	template<typename VERT>
	static mesh<VERT> from_voxels(g::game::voxels_paletted& vox, std::function<VERT(ogt_mesh_vertex* vert_in)> generator)
	{
		ogt_voxel_meshify_context empty_ctx = {};
		mesh<VERT> m;
		glGenBuffers(2, &m.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ibo);

		assert(GL_TRUE == glIsBuffer(m.vbo));
		assert(GL_TRUE == glIsBuffer(m.ibo));

		auto mesh = ogt_mesh_from_paletted_voxels_simple(&empty_ctx, vox.v.data(), vox.width, vox.height, vox.depth, (const ogt_mesh_rgba*)vox.palette.color);

		VERT* verts = new VERT[mesh->vertex_count];
		uint32_t* inds = new uint32_t[mesh->index_count];
		for (unsigned i = 0; i < mesh->vertex_count; i++)
		{
			verts[i] = generator(mesh->vertices + i);
		}

		// reverse index order so backface culling works correctly
		for (unsigned i = 0; i < mesh->index_count; i++)
		{
			inds[i] = mesh->indices[(mesh->index_count - 1) - i];
		}

		m.set_vertices(verts, mesh->vertex_count);
		m.set_indices(inds, mesh->index_count);

		ogt_mesh_destroy(&empty_ctx, mesh);
		delete[] verts;
		delete[] inds;

		return m;
	}

	template<typename VERT>
	static mesh<VERT> from_voxels(g::game::vox_scene& vox, std::function<VERT(ogt_mesh_vertex* vert_in)> generator)
	{
		ogt_voxel_meshify_context empty_ctx = {};
		mesh<VERT> m;
		std::vector<VERT> verts;
		std::vector<uint32_t> indices;

		glGenBuffers(2, &m.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ibo);

		assert(GL_TRUE == glIsBuffer(m.vbo));
		assert(GL_TRUE == glIsBuffer(m.ibo));

		for (auto& kvp : vox.instances)
		{
			auto& inst = kvp.second;
			auto T = inst.global_transform();
			auto mesh = ogt_mesh_from_paletted_voxels_simple(&empty_ctx, inst.model->v.data(), inst.model->width, inst.model->height, inst.model->depth, (const ogt_mesh_rgba*)vox.palette.color);
			auto half = (inst.model->size / 2).cast<float>();

			verts.reserve(verts.size() + mesh->vertex_count);
			auto last_vert_count = verts.size();
			for (unsigned i = 0; i < mesh->vertex_count; i++)
			{
				auto v = T * vec<3>{ mesh->vertices[i].pos.x, mesh->vertices[i].pos.y, mesh->vertices[i].pos.z } - half;
				mesh->vertices[i].pos.x = v[0];
				mesh->vertices[i].pos.y = v[1];
				mesh->vertices[i].pos.z = v[2];
				auto vert = generator(mesh->vertices + i);
				verts.push_back(vert);
			}

			// reverse index order so backface culling works correctly
			indices.reserve(indices.size() + mesh->index_count);
			auto last_ind_count = indices.size();
			for (unsigned i = 0; i < mesh->index_count; i++)
			{
				indices.push_back(mesh->indices[(mesh->index_count - 1) - i] + last_vert_count);
			}

			ogt_mesh_destroy(&empty_ctx, mesh);
		}

		m.set_vertices(verts);
		m.set_indices(indices);

		return m;
	}

	template<typename VERT>
	static mesh<VERT> from_heightmap(const texture& tex, std::function<VERT(const texture& tex, int x, int y)> generator)
	{
		mesh<VERT> m;

		glGenBuffers(2, &m.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ibo);

		std::vector<VERT> vertices;
		std::vector<uint32_t> indices;

		for (unsigned i = 0; i < tex.size[0]; i++)
		for (unsigned j = 0; j < tex.size[1]; j++)
		{
			vertices.push_back(generator(tex, i, j));
		}

		for (unsigned y = 0; y < tex.size[0] - 1; y++)
		for (unsigned x = 0; x < tex.size[1] - 1; x++)
		{
			unsigned i = x + y * (tex.size[0]);
			unsigned j = x + (y + 1) * (tex.size[0]);
			indices.push_back(j);
			indices.push_back(i + 1);
			indices.push_back(i);

			indices.push_back(i + 1);
			indices.push_back(j);
			indices.push_back(j + 1);
		}

		mesh<VERT>::compute_normals(vertices, indices);
		m.set_vertices(vertices);
		m.set_indices(indices);

		return m;
	}

	static mesh<vertex::pos_uv_norm> from_heightmap(const texture& tex)
	{


		return from_heightmap<vertex::pos_uv_norm>(tex, [](const texture& tex, int x, int y) -> vertex::pos_uv_norm {
			return {
				// position
				{
					x - tex.size[0] * 0.5f,
					static_cast<float>(tex.sample(x, y)[0] * 0.25f),
					y - tex.size[1] * 0.5f,
				},
				// uv
				{
					x / (float)tex.size[0],
					y / (float)tex.size[1],
				},
				// normal
				{ 0, 1, 0 }
			};
		});
	}

	template<typename VERT>
	static mesh<VERT> from_sdf(
		g::game::sdf sdf,
		std::function<VERT(const g::game::sdf& sdf, const vec<3>& pos)> generator,
		vec<3> volume_corners[2],
		unsigned max_depth=4)
	{
		mesh<VERT> m;
		glGenBuffers(2, &m.vbo);

		m.from_sdf_r(sdf, generator, volume_corners, max_depth);

		return m;
	}

	template<typename V>
	static mesh<V> empty_mesh()
	{
		mesh<V> m;
		glGenBuffers(2, &m.vbo);
		return m;
	}
};


struct font
{
	struct glyph
	{
		vec<2> uv_top_left;
		vec<2> uv_bottom_right;
		float width, height;
		vec<2> left_top;
		vec<2> advance;
	};
	unsigned point;
	texture face; /**< Texture containing all characters of the font face */
	std::unordered_map<unsigned char, glyph> char_map; /**< associates string characters with their corresponding glyph */
	std::unordered_map<unsigned char, std::unordered_map<unsigned char, vec<2>>> kerning_map;
};


struct font_factory
{
	font from_true_type(const std::string& path, unsigned point=16);
};


namespace debug
{
	struct print
	{
		vec<4> cur_color;
		g::game::camera* cur_cam;
		mat<4, 4> cur_model = mat<4, 4>::I();

		print(g::game::camera* cam);

		print& color(const vec<4>& c);

		print& model(const mat<4, 4>& m);

		void ray(const vec<2>& o, const vec<2>& d);

		void ray(const vec<3>& o, const vec<3>& d);

		void point(const vec<2>& o);

		void point(const vec<3>& o);

		void box(const vec<3> corners[2]);
	};
}


template<typename V>
struct density_volume
{
    struct block
    {
        g::gfx::mesh<V> mesh;
        std::vector<V> vertices;
        std::vector<uint32_t> indices;

        vec<3, int> index;
        vec<3> bounding_box[2];
        uint8_t vertex_case = 0;
        bool regenerating = false;
        std::chrono::time_point<std::chrono::system_clock> start;

        inline bool contains(const vec<3>& pos) const
        {
            return pos[0] > bounding_box[0][0] && pos[0] <= bounding_box[1][0] &&
                   pos[1] > bounding_box[0][1] && pos[1] <= bounding_box[1][1] &&
                   pos[2] > bounding_box[0][2] && pos[2] <= bounding_box[1][2];
        }

        inline bool contains(const vec<3, int> idx) const
        {
            return index == idx;
        }
    };

    std::vector<density_volume::block> blocks;
    std::vector<vec<3>> offsets;

    g::game::sdf sdf;
    std::function<V(const g::game::sdf& sdf, const vec<3>& pos)> generator;
    float scale = 1;
    unsigned depth = 1;
    unsigned kernel = 2;
    std::vector<density_volume::block*> to_regenerate;
    g::proc::thread_pool<10> generator_pool;

    density_volume() = default;

    density_volume(
        const g::game::sdf& sdf,
        std::function<V(const g::game::sdf& sdf, const vec<3>& pos)> generator,
        const std::vector<vec<3>>& offset_config) :
        
        offsets(offset_config)
    {
        this->sdf = sdf;
        this->generator = generator;

        for (auto offset : offsets)
        {
            density_volume::block block;
            block.mesh = g::gfx::mesh_factory{}.empty_mesh<V>();

            // auto pipo = offset.template cast<int>();

            // block.bounding_box[0] = (pipo * scale).template cast<float>();
            // block.bounding_box[1] = ((pipo + 1) * scale).template cast<float>();

            // block.index = (offset * scale).template cast<int>();

            // // block.mesh.from_sdf_r(sdf, generator, block.bounding_box, depth);
            // block.mesh.from_sdf(sdf, generator, block.bounding_box);

            blocks.push_back(block);
        }
    }

    void update(const g::game::camera& cam)
    {
        auto pos = cam.position;
        std::set<unsigned> unvisited;

        for (unsigned i = 0; i < offsets.size(); i++) { unvisited.insert(i); }

        auto pidx = ((pos / scale) - 0.5f).template cast<int>();

        generator_pool.update();

        for (auto& block : blocks)
        {
            if (block.regenerating) { continue; }

            bool needs_regen = true;

            for (auto oi : unvisited)
            {
                auto pipo = pidx + offsets[oi].template cast<int>();

                if (block.contains(pipo))
                {
                    unvisited.erase(oi);
                    needs_regen = false;
                    break;
                }
            }

            if (needs_regen) { to_regenerate.push_back(&block); }
        }

        // remaining 'unvisited' offset positions need to be regenerated
        for (auto oi : unvisited)
        {
            if (to_regenerate.size() == 0) { break; }

            auto offset = offsets[oi];
            auto block_ptr = to_regenerate.back();
            block_ptr->regenerating = true;
            to_regenerate.pop_back();

            generator_pool.run(
            // generation task
            [this, oi, pidx, offset, block_ptr](){
            	block_ptr->start = std::chrono::system_clock::now();

                auto pipo = pidx + offsets[oi].template cast<int>();

                block_ptr->bounding_box[0] = (pipo * scale).template cast<float>();
                block_ptr->bounding_box[1] = ((pipo + 1) * scale).template cast<float>();
                block_ptr->index = pipo;

                block_ptr->mesh.from_sdf_r(block_ptr->vertices, block_ptr->indices, sdf, generator, block_ptr->bounding_box, depth);
                // block_ptr->mesh.from_sdf(block_ptr->vertices, block_ptr->indices, sdf, generator, block_ptr->bounding_box);
                block_ptr->regenerating = false;
            },
            // on finish
            [this, block_ptr](){
                block_ptr->mesh.set_vertices(block_ptr->vertices);
                block_ptr->mesh.set_indices(block_ptr->indices);

                char buf[256];
                std::chrono::duration<float> diff = std::chrono::system_clock::now() - block_ptr->start;
                snprintf(buf, sizeof(buf), "%lu vertices - block %s in %f sec\n", block_ptr->vertices.size(), block_ptr->index.to_string().c_str(), diff.count());
                write(1, buf, strlen(buf));
            });
        }
    }

    void draw(g::game::camera& cam, g::gfx::shader& s, std::function<void(g::gfx::shader::usage&)> draw_config=nullptr)
    {
        for (auto& block : blocks)
        {
            auto chain = block.mesh.using_shader(s)
				                   .set_camera(cam);

            if (draw_config) { draw_config(chain); }

            chain.template draw<GL_TRIANGLES>();

           g::gfx::debug::print{&cam}.color({1, 1, 1, 1}).box(block.bounding_box);
        }
    }
};

namespace effect
{

mesh<vertex::pos_uv_norm>& fullscreen_quad();

template<typename V>
void shadow_0(const mesh<V>& m, const framebuffer& shadow_map, game::camera& light, game::camera& cam, std::function<void(g::gfx::shader::usage&)> draw_config=nullptr)
{
    // TODO: the assumption that the appropriate textel in depth_map can be
    // sampled to loop up a uv is not correct. I think the correct approach is
    // to involve the vertex shader like it has been done traditionally
	static shader shadow_shader;
    static std::string shadow_map_vs_src =
    "in vec3 a_position;"
    "uniform mat4 u_model;"
    "uniform mat4 u_view;"
    "uniform mat4 u_proj;"
    "uniform mat4 u_light_view;"
    "uniform mat4 u_light_proj;"
    "out vec4 v_light_proj_pos;"
    "void main (void)"
    "{"
    "   vec4 v_world_pos = u_model * vec4(a_position, 1.0);"
    "   gl_Position = u_proj * u_view * v_world_pos;"
    "   v_light_proj_pos = u_light_proj * u_light_view * v_world_pos;"
    "   v_light_proj_pos /= v_light_proj_pos.w;"
    "   v_light_proj_pos = (v_light_proj_pos + 1.0) / 2.0;"
    "}";

    static std::string shadow_map_fs_src =
    "in vec4 v_light_proj_pos;"
    "uniform sampler2D u_shadow_map;"
    "out vec4 color;"
    "void main (void)"
    "{"
    "   float bias = 0.00006;"
    "   float depth = v_light_proj_pos.z - bias;"
    "   float shadowing = 0.0;"
    "   for(float y = -2.0; y <= 2.0; y++)"
    "   for(float x = -2.0; x <= 2.0; x++)"
    "   {"
    "       float sampled_depth = texture(u_shadow_map, v_light_proj_pos.xy + (vec2(x, y) * 0.0005)).r;"
    "       if (depth <= sampled_depth)"
    "       {"
    "           shadowing += 1.0 / 25.0;"
    "       }"
    "   }"
    "   color = vec4(vec3(shadowing + 0.1), 1.0);"
    "}";


    if (!shadow_shader.is_initialized())
    {
        shadow_shader = shader_factory{}
            .add_src<GL_VERTEX_SHADER>(shadow_map_vs_src)
            .add_src<GL_FRAGMENT_SHADER>(shadow_map_fs_src)
            .create();
    }

    // TODO: set appropriate blending

    auto model = mat<4, 4>::I();

	glEnable( GL_POLYGON_OFFSET_FILL );
	glPolygonOffset( 0, -1.0 );
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
    // glClear(GL_DEPTH_BUFFER_BIT);

    auto chain = m.using_shader(shadow_shader)
    .set_camera(cam)
    ["u_shadow_map"].texture(shadow_map.depth)
    ["u_model"].mat4(model)
    ["u_light_view"].mat4(light.view())
    ["u_light_proj"].mat4(light.projection());

    if (draw_config) { draw_config(chain); }

    chain.template draw<GL_TRIANGLES>();

	glPolygonOffset( 0, 0 );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void blit(const framebuffer& fb);

} // end namespace effect

namespace primative
{

template <typename D>
struct renderer
{
	virtual shader::usage using_shader(g::gfx::shader& shader, const D& data, g::game::camera& cam, const mat<4, 4>& model) = 0;
	virtual void draw(g::gfx::shader& shader, const D& data, g::game::camera& cam, const mat<4, 4>& model) = 0;
};


struct volume_slicer : public renderer<texture>
{
	g::gfx::mesh<g::gfx::vertex::pos> slices;

	volume_slicer() = default;

	volume_slicer(unsigned num_slices)
	{
        slices = g::gfx::mesh_factory::slice_cube(num_slices);
	}

	shader::usage using_shader(g::gfx::shader& shader,
					  const g::gfx::texture& vox,
			          g::game::camera& cam,
			          const mat<4, 4>& model)
	{
		assert(vox.is_3D());

        // rotate volume slices to align with camera view
        // create a rotation matrix to represente this
        auto O = cam.forward();
        auto u = cam.up();
        auto R_align = mat<4, 4>::look_at({0, 0, 0}, O, u).invert();

        auto x_mag = sqrtf(powf(model[0][0], 2.f) + powf(model[1][0], 2.f) + powf(model[2][0], 2.f));
        auto y_mag = sqrtf(powf(model[0][1], 2.f) + powf(model[1][1], 2.f) + powf(model[2][1], 2.f));
        auto z_mag = sqrtf(powf(model[0][2], 2.f) + powf(model[1][2], 2.f) + powf(model[2][2], 2.f));

        // extract the rotation matrix from model by dividing each basis column vector
        // by the magnitude computed for each in the model matrix above
        auto R_orient = mat<4, 4>{
        	{ model[0][0] / x_mag, model[0][1] / y_mag, model[0][2] / z_mag, 0 },
        	{ model[1][0] / x_mag, model[1][1] / y_mag, model[1][2] / z_mag, 0 },
        	{ model[2][0] / x_mag, model[2][1] / y_mag, model[2][2] / z_mag, 0 },
        	{ 0                  , 0                  , 0                  , 1 }
        };

        // reconstruct the transform (model matrix) from only the scale and
        // translation. The reason for this is that rotating the actual slicing
        // mesh as desired by the model matrix will cause misalignment with the
        // view frustum and undesired artifacts will be visible.
        auto T = mat<4, 4>{
        	{ x_mag,     0,     0,    model[0][3] },
        	{     0, y_mag,     0,    model[1][3] },
        	{     0,     0, z_mag,    model[2][3] },
        	{     0,     0,     0,              1 }
        };

        const vec<3> delta = { 1.f/(float)vox.size[0], 1.f/(float)vox.size[1], 1.f/(float)vox.size[2] };

        return slices.using_shader(shader)
               .set_camera(cam)
               ["u_model"].mat4(T * R_align)
               ["u_rotation"].mat4(R_orient)
               ["u_uvw_step"].vec3(delta)
               ["u_voxels"].texture(vox);
	}

	void draw(g::gfx::shader& shader,
		      const g::gfx::texture& vox,
			  g::game::camera& cam,
			  const mat<4, 4>& model)
	{
		using_shader(shader, vox, cam, model).draw<GL_TRIANGLES>();
	}
};

struct text : public renderer<std::string>
{
	g::gfx::font& font;
    static g::gfx::mesh<vertex::pos_uv_norm> plane;

	struct it
	{
	public:
		struct ctx
		{
			char c;
			g::gfx::font::glyph glyph;
			vec<2> pen = {};
		};

		it(const std::string &str, g::gfx::font& f, size_t pos=0);

		void operator++();

		vec<2> kerning();

		bool operator!=(it &i);

		text::it::ctx operator*();

	protected:
		const std::string& _str;
		g::gfx::font& _font;
		it::ctx _ctx;
		size_t _pos = 0;
		char _last_char;
	};

	text(g::gfx::font& f);

	shader::usage using_shader(g::gfx::shader& shader,
		const std::string& str,
		std::function<void(g::gfx::shader::usage&)> shader_config);

	shader::usage using_shader(g::gfx::shader& shader,
		const std::string& str,
		g::game::camera& cam,
		const mat<4, 4>& model);

	void draw(g::gfx::shader& shader,
		  const std::string& str,
	      g::game::camera& cam,
	      const mat<4, 4>& model);

	void measure(const std::string& str, vec<2>& dims_out, vec<2>& offset_out);
};

}; // end namespace primative
}; // end namespace gfx
}; // end namespace g
