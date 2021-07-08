

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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#ifdef __linux__
#include <GL/glx.h>
#include <GL/glext.h>
#endif

#include <GLFW/glfw3.h>

#ifdef __APPLE__
#undef __gl_h_
#include <OpenGL/gl3.h>
#endif

#include <ogt_vox.h>
#include <ogt_voxel_meshify.h>

#define G_TERM_GREEN "\033[0;32m"
#define G_TERM_RED "\033[1;31m"
#define G_TERM_YELLOW "\033[1;33m"
#define G_TERM_BLUE "\033[1;34m"
#define G_TERM_COLOR_OFF "\033[0m"

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

static size_t width()
{
	int width, height;
	glfwGetFramebufferSize(GLFW_WIN, &width, &height);
	return width;
}

static size_t height()
{
	int width, height;
	glfwGetFramebufferSize(GLFW_WIN, &width, &height);
	return height;
}

static float aspect()
{
	int width, height;
	glfwGetFramebufferSize(GLFW_WIN, &width, &height);
	return width / (float)height;
}

struct texture
{
	GLenum type;
	unsigned size[3] = { 1, 1, 1 };
	GLuint texture = (GLuint)-1;
	char* data = nullptr;

	void release_bitmap();

	void create(GLenum texture_type);

	void destroy();

	void set_pixels(size_t w, size_t h, char* data, GLenum format=GL_RGBA, GLenum storage=GL_UNSIGNED_BYTE, GLenum t=GL_TEXTURE_2D);

	void set_pixels(size_t w, size_t h, size_t d, char* data, GLenum format=GL_RGBA, GLenum storage=GL_UNSIGNED_BYTE, GLenum t=GL_TEXTURE_2D);

	inline bool is_1D() const { return size[0] > 1 && size[1] == 1 && size[2] == 1; }
	inline bool is_2D() const { return size[0] > 1 && size[1] > 1 && size[2] == 1; }
	inline bool is_3D() const { return size[0] > 1 && size[1] > 1 && size[2] > 1; }

	void bind() const;
};


struct texture_factory
{
	unsigned size[3];
	char* data = nullptr;
	GLenum texture_type;
	GLenum min_filter = GL_LINEAR, mag_filter = GL_LINEAR;
	GLenum wrap_s = GL_CLAMP_TO_EDGE, wrap_t = GL_CLAMP_TO_EDGE;
	GLenum color_type = GL_RGBA;
	GLenum storage_type = GL_UNSIGNED_BYTE;

	explicit texture_factory() = default;

	explicit texture_factory(unsigned w, unsigned h);

	explicit texture_factory(unsigned w, unsigned h, unsigned d);

	void abort(std::string message);

	texture_factory& from_png(const std::string& path);

	texture_factory& components(unsigned count);

	texture_factory& color();

	texture_factory& depth();

	texture_factory& pixelated();

	texture_factory& smooth();

	texture_factory& clamped();

	texture_factory& repeating();

	template<GLenum COLOR, GLenum STORAGE>
	texture_factory& custom(std::function<void(unsigned int x, unsigned int y, char* pixel)> filler)
	{
		int components = 0;
		int bytes_per_component;
		void* pixels;

		switch (COLOR)
		{
			case GL_RED:
				components = 1;
				break;
			case GL_RG:
				components = 2;
				break;
			case GL_RGB:
			case GL_BGR:
				components = 3;
				break;
			case GL_RGBA:
	        case GL_BGRA:
	        	components = 4;
	        	break;
		}


		switch (STORAGE)
		{
			case GL_UNSIGNED_BYTE:
			case GL_BYTE:
				bytes_per_component = 1;
				break;
			case GL_UNSIGNED_SHORT:
	        case GL_SHORT:
	        	bytes_per_component = 2;
	        	break;
	        case GL_UNSIGNED_INT:
	        case GL_INT:
	        case GL_FLOAT:
	        	bytes_per_component = 4;
	        	break;
		}

		auto bytes_per_pixel = bytes_per_component * components;
		auto bytes_per_row = bytes_per_pixel * size[0];
		data = (char*)calloc(bytes_per_pixel, size[0] * size[1]);

		for (int y = 0; y < size[1]; y++)
		{
			for (int x = 0; x < size[0]; x++)
			{
				filler(x, y, data + (x * bytes_per_pixel) + (y * bytes_per_row));
			}
		}

		return *this;
	}

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
		if (color.texture != (GLuint)-1)
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
	unsigned size[2];
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
	GLuint program;
	std::unordered_map<std::string, GLint> uni_locs;

	shader& bind();

	struct uniform_usage;
	/**
	 * @brief      shader::usage type represents the start of some invocation
	 * of an interaction with a shader.
	 */
	struct usage
	{
		shader& shader_ref;
		size_t vertices, indices;
		int texture_unit;

		usage (shader& ref, size_t verts, size_t inds);

		template<typename MV>
		usage attach_attributes(const shader& shader)
		{
			assert(gl_get_error());
			MV::attributes(shader.program);
			assert(gl_get_error());
			return *this;
		}

		usage set_camera(const g::game::camera& cam);

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

		usage vec3 (const vec<3>& v);

		usage flt(float f);

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
		auto fd = open(path.c_str(), O_RDONLY);
		auto size = lseek(fd, 0, SEEK_END);

		{ // read and compile the shader
			GLchar* src = new GLchar[size];
			lseek(fd, 0, SEEK_SET);
			read(fd, src, size);

			std::cerr << "Compiling: " << path << "... ";

			shaders[ST] = compile_shader(ST, src, (GLsizei)size);

			close(fd);
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
		shader::usage usage = {shader, vertex_count, index_count};
		usage.attach_attributes<V>(shader);
		return usage;
	}
};


struct mesh_factory
{
	static mesh<vertex::pos_uv_norm> plane()
	{
		mesh<vertex::pos_uv_norm> p;
		glGenBuffers(2, &p.vbo);

		p.set_vertices({
			{{-1, 1, 0}, {1, 1}, {0, 0, 1}},
			{{ 1, 1, 0}, {0, 1}, {0, 0, 1}},
			{{ 1,-1, 0}, {0, 0}, {0, 0, 1}},
			{{-1,-1, 0}, {1, 0}, {0, 0, 1}},
		});

		return p;
	}

	static mesh<vertex::pos> slice_cube(unsigned slices)
	{
		mesh<vertex::pos> c;
		std::vector<vertex::pos> verts;
		std::vector<uint32_t> indices;
		glGenBuffers(2, &c.vbo);

		const float h = 0.5 * sqrt(3);
		float dz = (2.f * h) / static_cast<float>(slices);

		for (;slices--;)
		{
			auto n = verts.size();
			indices.push_back(n + 2);
			indices.push_back(n + 3);
			indices.push_back(n + 0);
			indices.push_back(n + 1);
			indices.push_back(n + 2);
			indices.push_back(n + 0);

			auto z = dz * slices;
			verts.push_back({{-h, h, -h + z}});
			verts.push_back({{ h, h, -h + z}});
			verts.push_back({{ h,-h, -h + z}});
			verts.push_back({{-h,-h, -h + z}});

		}

		c.set_vertices(verts);
		c.set_indices(indices);

		return c;
	}

	static mesh<vertex::pos_uv_norm> cube()
	{
		mesh<vertex::pos_uv_norm> c;
		glGenBuffers(1, &c.vbo);
		c.set_vertices({
			// +x face
			{{ 1,-1, 1}, {1, 1}, {1, 0, 0}},
			{{ 1, 1, 1}, {0, 1}, {1, 0, 0}},
			{{ 1, 1,-1}, {0, 0}, {1, 0, 0}},
			{{ 1,-1,-1}, {1, 0}, {1, 0, 0}},

			// -x face
			{{-1,-1, 1}, {1, 1}, {-1, 0, 0}},
			{{-1, 1, 1}, {0, 1}, {-1, 0, 0}},
			{{-1, 1,-1}, {0, 0}, {-1, 0, 0}},
			{{-1,-1,-1}, {1, 0}, {-1, 0, 0}},

			// +y face
			{{-1, 1, 1}, {1, 1}, {0, 1, 0}},
			{{ 1, 1, 1}, {0, 1}, {0, 1, 0}},
			{{ 1, 1,-1}, {0, 0}, {0, 1, 0}},
			{{-1, 1,-1}, {1, 0}, {0, 1, 0}},

			// -y face
			{{-1,-1, 1}, {1, 1}, {0,-1, 0}},
			{{ 1,-1, 1}, {0, 1}, {0,-1, 0}},
			{{ 1,-1,-1}, {0, 0}, {0,-1, 0}},
			{{-1,-1,-1}, {1, 0}, {0,-1, 0}},

			// +z face
			{{-1, 1, 1}, {1, 1}, {0, 0, 1}},
			{{ 1, 1, 1}, {0, 1}, {0, 0, 1}},
			{{ 1,-1, 1}, {0, 0}, {0, 0, 1}},
			{{-1,-1, 1}, {1, 0}, {0, 0, 1}},

			// -z face
			{{-1, 1,-1}, {1, 1}, {0, 0, -1}},
			{{ 1, 1,-1}, {0, 1}, {0, 0, -1}},
			{{ 1,-1,-1}, {0, 0}, {0, 0, -1}},
			{{-1,-1,-1}, {1, 0}, {0, 0, -1}},
		});

		glGenBuffers(1, &c.ibo);
		c.set_indices({
			(0) + 2, (0) + 3, (0) + 0, (0) + 1, (0) + 2, (0) + 0,
			(4) + 0, (4) + 3, (4) + 2, (4) + 0, (4) + 2, (4) + 1,
			(8) + 0, (8) + 3, (8) + 2, (8) + 0, (8) + 2, (8) + 1,
			(12) + 2, (12) + 3, (12) + 0, (12) + 1, (12) + 2, (12) + 0,
			(16) + 2, (16) + 3, (16) + 0, (16) + 1, (16) + 2, (16) + 0,
			(20) + 0, (20) + 3, (20) + 2, (20) + 0, (20) + 2, (20) + 1,
		});

		return c;
	}

	template<typename VERT>
	static mesh<VERT> from_voxels(g::game::voxels_paletted& vox, std::function<VERT(ogt_mesh_vertex* vert_in)> converter)
	{
		ogt_voxel_meshify_context empty_ctx = {};
		mesh<VERT> m;
		glGenBuffers(2, &m.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ibo);

		assert(GL_TRUE == glIsBuffer(m.vbo));
		assert(GL_TRUE == glIsBuffer(m.ibo));

		auto mesh = ogt_mesh_from_paletted_voxels_simple(&empty_ctx, vox.v, vox.width, vox.height, vox.depth, (const ogt_mesh_rgba*)vox.palette.color);

		VERT* verts = new VERT[mesh->vertex_count];
		uint32_t* inds = new uint32_t[mesh->index_count];
		for (auto i = 0; i < mesh->vertex_count; i++)
		{
			verts[i] = converter(mesh->vertices + i);
		}
		memcpy(inds, mesh->indices, sizeof(uint32_t) * mesh->index_count);

		m.set_vertices(verts, mesh->vertex_count);
		m.set_indices(inds, mesh->index_count);

		ogt_mesh_destroy(&empty_ctx, mesh);
		delete[] verts;
		delete[] inds;

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

namespace primative
{

template <typename D>
struct renderer
{
	virtual void draw(const g::gfx::shader& shader, const D& data, const g::game::camera& cam, const mat<4, 4>& model) = 0;
};


struct volume_slicer : public renderer<texture>
{
	g::gfx::mesh<g::gfx::vertex::pos> slices;

	volume_slicer(unsigned num_slices=1000)
	{
        slices = g::gfx::mesh_factory::slice_cube(num_slices);
	}

	void draw(g::gfx::shader& shader,
			  const g::gfx::texture& vox,
	          const g::game::camera& cam,
	          const mat<4, 4>& model)
	{
		assert(vox.is_3D());

        // rotate volume slices to align with camera view
        // create a rotation matrix to represente this
        auto O = cam.forward();
        auto u = cam.up();
        auto l = vec<3>::cross(O, u);
        auto R_align = mat<4, 4>::look_at({0, 0, 0}, O, u).invert();

        auto x_mag = sqrtf(powf(model[0][0], 2.f) + powf(model[1][0], 2.f) + powf(model[2][0], 2.f));
        auto y_mag = sqrtf(powf(model[0][1], 2.f) + powf(model[1][1], 2.f) + powf(model[2][1], 2.f));
        auto z_mag = sqrtf(powf(model[0][2], 2.f) + powf(model[1][2], 2.f) + powf(model[2][2], 2.f));

        // extract the rotation matrix from model by dividing each basis column vector
        // by the magnitude computed for each in the model matrix above
        auto R_orient = mat<3, 3>{ 
        	{ model[0][0] / x_mag, model[0][1] / y_mag, model[0][2] / z_mag },
        	{ model[1][0] / x_mag, model[1][1] / y_mag, model[1][2] / z_mag },
        	{ model[2][0] / x_mag, model[2][1] / y_mag, model[2][2] / z_mag },
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

        const auto delta = 1.f/static_cast<float>(vox.size[0]);

        vox.bind();
        slices.using_shader(shader)
             .set_camera(cam)
             ["u_model"].mat4(T * R_align)
             ["u_rotation"].mat4(R_align)
             ["u_tex_coord_step"].flt(delta)
             ["u_voxels"].int1(0)
             .draw<GL_TRIANGLES>();
        // auto tranform =
	}
};

}; // end namespace renderer
}; // end namespace gfx
}; // end namespace g
