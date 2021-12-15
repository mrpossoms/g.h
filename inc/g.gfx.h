

#pragma once
#define XMTYPE float
#include <xmath.h>
#include <g.game.h>

#include <iostream>
#include <unordered_map>
#include <vector>
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
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
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

size_t width();

size_t height();

float aspect();

struct texture
{
	GLenum type;
	unsigned component_count = 0;
	unsigned bytes_per_component = 0;
	unsigned size[3] = { 1, 1, 1 };
	GLuint hnd = -1;
	unsigned char* data = nullptr;

	inline bool is_initialized() const { return hnd != -1; }

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

	void abort(std::string message);

	texture_factory& from_png(const std::string& path);

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
	GLuint fbo;
	unsigned size[3];
	texture color;
	texture depth;

	void bind_as_target()
	{
		glViewport(0, 0, size[0], size[1]);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		assert(gl_get_error());
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
			assert(gl_get_error());
			MV::attributes(shader.program);
			assert(gl_get_error());
			return *this;
		}

		usage set_camera(g::game::camera& cam);

		uniform_usage set_uniform(const std::string& name);

		uniform_usage operator[](const std::string& name);

		template<GLenum PRIM>
		usage& draw()
		{
			assert(gl_get_error());
			if (indices > 0)
			{
				glDrawElements(PRIM, indices, GL_UNSIGNED_INT, NULL);
				assert(gl_get_error());
			}
			else
			{
				glDrawArrays(PRIM, 0, vertices);
				assert(gl_get_error());
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

			shaders[ST] = compile_shader(ST, src, (GLsizei)size);

			::close(fd);
			delete[] src;
		}

		return *this;
	}

	template<GLenum ST>
	shader_factory add_src(const std::string& src)
	{
		{ // read and compile the shader
			shaders[ST] = compile_shader(ST, src.c_str(), (GLsizei)src.length());
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

	mesh& set_vertices(const std::vector<V>& verts)
	{
		return set_vertices(verts.data(), verts.size());
	}

	mesh& set_vertices(const V* verts, size_t count)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		assert(gl_get_error());
		vertex_count = count;
		assert(gl_get_error());
		glBufferData(
			GL_ARRAY_BUFFER,
			count * sizeof(V),
			verts,
			GL_STATIC_DRAW
		);
		assert(gl_get_error());

		return *this;
	}

	mesh& set_indices(const std::vector<uint32_t>& inds)
	{
		return set_indices(inds.data(), inds.size());
	}

	mesh& set_indices(const uint32_t* inds, size_t count)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		assert(gl_get_error());
		assert(GL_TRUE == glIsBuffer(ibo));
		index_count = count;
		glBufferData(
			GL_ELEMENT_ARRAY_BUFFER,
			count * sizeof(uint32_t),
			inds,
			GL_STATIC_DRAW
		);
		assert(gl_get_error());

		return *this;
	}

	shader::usage using_shader (shader& shader)
	{
		assert(gl_get_error());
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		assert(gl_get_error());

		if (index_count > 0)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
			assert(gl_get_error());
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
	static mesh<VERT> from_heightmap(const texture& tex, std::function<VERT(const texture& tex, int x, int y)> generator)
	{
		mesh<VERT> m;

		glGenBuffers(2, &m.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ibo);

		std::vector<VERT> vertices;
		std::vector<uint32_t> indices;

		for (int i = 0; i < tex.size[0]; i++)
		for (int j = 0; j < tex.size[1]; j++)
		{
			vertices.push_back(generator(tex, i, j));
		}

		for (int y = 0; y < tex.size[0] - 1; y++)
		for (int x = 0; x < tex.size[1] - 1; x++)
		{
			int i = x + y * (tex.size[0]);
			int j = x + (y + 1) * (tex.size[0]);
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
	};
}


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
