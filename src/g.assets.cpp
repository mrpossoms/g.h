#include "g.assets.h"
#include "g.utils.h"
#include "g.io.h"

#define OGT_VOX_IMPLEMENTATION
#define OGT_VOXEL_MESHIFY_IMPLEMENTATION

#ifdef __unix__
#include <strings.h>
#endif

#include <regex>
#include <ogt_vox.h>
#include <ogt_voxel_meshify.h>

g::asset::store::~store()
{
	// tear down assets

	for (auto& t : textures)
	{
		t.second.get().destroy();
	}

	for (auto& s : shaders)
	{
		s.second.get().destroy();
	}

	for (auto& f : fonts)
	{
		f.second.get().face.destroy();
	}

	for (auto& g : geos)
	{
		g.second.get().destroy();
	}
}

g::gfx::texture& g::asset::store::tex(const std::string& partial_path, bool make_if_missing)
{
	auto itr = textures.find(partial_path);
	if (itr == textures.end())
	{
		if (make_if_missing && g::io::file{root + "/tex/" + partial_path}.exists() == false)
		{
			auto path = root + "/tex/" + partial_path;
			g::io::file::make_path(path.c_str());

			// TODO:
			auto tex = g::gfx::texture_factory(8, 8)
			.components(3)
			.type(GL_UNSIGNED_BYTE)
			.fill([](int x, int y, int z, unsigned char* pixel){
				bool on_line = (x + y) & 0x1;
				pixel[0] = !on_line * 255;
				pixel[1] = !on_line * 128;
				pixel[2] = 0;
			}).to_png(path)
			.pixelated()
			.create();
			textures[partial_path] = { time(nullptr), tex };
		}
		else if (partial_path.find(".png") == partial_path.length() - 4)
		{
			auto chain = g::gfx::texture_factory().from_png(root + "/tex/" + partial_path).pixelated();
			// do spicy chain thing with processors here
			if (std::string::npos != partial_path.find("repeating"))
			{
				chain = chain.repeating();
			}

			if (std::string::npos != partial_path.find("smooth"))
			{
				chain = chain.smooth();
			}

			auto tex = chain.create();
			textures[partial_path] = { time(nullptr), tex };
		}
		else if (partial_path.find(".tiff") == partial_path.length() - 5)
		{
			auto chain = g::gfx::texture_factory().from_tiff(root + "/tex/" + partial_path).pixelated();
			// do spicy chain thing with processors here
			if (std::string::npos != partial_path.find("repeating"))
			{
				chain = chain.repeating();
			}

			if (std::string::npos != partial_path.find("smooth"))
			{
				chain = chain.smooth();
			}

			auto tex = chain.create();
			textures[partial_path] = { time(nullptr), tex };
		}
	}
	else if (hot_reload)
	{
		auto mod_time = g::io::file(root + "/tex/" + partial_path).modified();
		// std::cerr << mod_time << " - " << itr->second.last_accessed << std::endl;
		if (mod_time < itr->second.last_accessed && itr->second.loaded_time < mod_time)
		{
			std::cerr << partial_path << " has been updated, reloading" << std::endl;

			itr->second.get().destroy();
			textures.erase(itr);

			return this->tex(partial_path);
		}
	}

	return textures[partial_path].get();
}


g::gfx::shader& g::asset::store::shader(const std::string& program_collection)
{
	auto itr = shaders.find(program_collection);
	if (itr == shaders.end())
	{
		g::gfx::shader_factory factory;
		for (auto shader_path : g::utils::split(program_collection, "+"))
		{
			if (std::string::npos != shader_path.find(".vs"))
			{
				factory = factory.add<GL_VERTEX_SHADER>(root + "/shader/" + shader_path);
			}
			else if (std::string::npos != shader_path.find(".fs"))
			{
				factory = factory.add<GL_FRAGMENT_SHADER>(root + "/shader/" + shader_path);
			}
		}

		shaders[program_collection] = { time(nullptr), factory.create() };
	}
	else if(hot_reload)
	{
		auto do_reload = false;

		for (auto shader_path : g::utils::split(program_collection, "+"))
		{
			if (std::string::npos != shader_path.find(".vs"))
			{
				auto mod_time = g::io::file(root + "/shader/" + shader_path).modified();
				do_reload |= mod_time < itr->second.last_accessed && itr->second.loaded_time < mod_time;
			}
			else if (std::string::npos != shader_path.find(".fs"))
			{
				auto mod_time = g::io::file(root + "/shader/" + shader_path).modified();
				do_reload |= mod_time < itr->second.last_accessed && itr->second.loaded_time < mod_time;
			}
		}

		if (do_reload)
		{
			itr->second.get().destroy();
			shaders.erase(itr);
			return this->shader(program_collection);
		}
	}


	return shaders[program_collection].get();
}


g::gfx::font& g::asset::store::font(const std::string& partial_path, bool make_if_missing)
{
	unsigned point = 16;

	auto itr = fonts.find(partial_path);
	if (itr == fonts.end())
	{
		std::cmatch m;
		std::regex re("[0-9]+pt[.]");
		if(std::regex_search (partial_path.c_str(), m, re))
		{
			std::cout << "match: " << m[0] << std::endl;
			auto num_end = m.str(0).find("pt");

			char pt_str[8] = {};
			strncpy(pt_str, m.str(0).c_str(), num_end);
			int pt = atoi(pt_str);

			if (pt >= 0) { point = pt; }
			else
			{
				std::cerr << G_TERM_RED << "Could not parse point attribute '" << m.str(0) << "' for font '" << partial_path << "'" << G_TERM_COLOR_OFF << std::endl;
			}
		}

		fonts[partial_path] = {time(nullptr), g::gfx::font_factory{}.from_true_type(root + "/font/" + partial_path, point)};
	}

	return fonts[partial_path].get();
}


g::gfx::mesh<g::gfx::vertex::pos_uv_norm>& g::asset::store::geo(const std::string& partial_path, bool make_if_missing)
{
	auto itr = geos.find(partial_path);
	if (itr == geos.end())
	{
		if (make_if_missing)
		{
			const char* tri_obj =
			"o Tri\n"
			"v -1 -1 0\n"
			"v 1 -1 0\n"
			"v 0 1 0\n"
			"vt 1.000000 1.000000\n"
			"vt 0.968750 0.500000\n"
			"vt 1.000000 0.500000\n"
			"vn 0 -1 0\n"
			"s off\n"
			"f 2/1/1 3/2/1 1/3/1\n";

			g::io::file out(root + "/geo/" + partial_path, g::io::file::mode::write_only());
			out.write((void*)tri_obj, strlen(tri_obj));
		}

		if (std::string::npos != partial_path.find(".obj"))
		{
			geos[partial_path] = { time(nullptr), g::gfx::mesh_factory{}.from_obj(root + "/geo/" + partial_path) };
		}
	}
	else if (hot_reload)
	{
		auto mod_time = g::io::file(root + "/geo/" + partial_path).modified();
		if (mod_time < itr->second.last_accessed && itr->second.loaded_time < mod_time)
		{
			itr->second.get().destroy();
			geos.erase(itr);

			return this->geo(partial_path);
		}
	}

	return geos[partial_path].get();
}


g::game::voxels_paletted& g::asset::store::vox(const std::string& partial_path, bool make_if_missing)
{
	auto itr = voxels.find(partial_path);
	if (itr == voxels.end())
	{
		std::string filename = root + "/vox/" + partial_path;
	    // open the file
#if defined(_MSC_VER) && _MSC_VER >= 1400
    	FILE * fp;
    	if (0 != fopen_s(&fp, filename.c_str(), "rb")) { fp = nullptr; }
#else
    	FILE * fp = fopen(filename.c_str(), "rb");
#endif
    	if (!fp) { throw std::runtime_error(partial_path + ": vox file could not be opened"); }

	    // get the buffer size which matches the size of the file
	    fseek(fp, 0, SEEK_END);
	    uint32_t buffer_size = ftell(fp);
	    fseek(fp, 0, SEEK_SET);

	    // load the file into a memory buffer
	    uint8_t * buffer = new uint8_t[buffer_size];
	    fread(buffer, buffer_size, 1, fp);
	    fclose(fp);

	    // construct the scene from the buffer
	    const ogt_vox_scene* scene = ogt_vox_read_scene_with_flags(buffer, buffer_size, 0);

	    if (scene->num_models == 0)
	    {
	    	delete[] buffer;
	    	ogt_vox_destroy_scene(scene);
	    	throw std::runtime_error(partial_path + ": vox file contained no models");
	    }

	    voxels[partial_path] = { time(nullptr), g::game::voxels_paletted{
	    	scene->palette,
	    	scene->models[0]->voxel_data,
	    	scene->models[0]->size_x,
	    	scene->models[0]->size_y,
	    	scene->models[0]->size_z
	    } };

	    // the buffer can be safely deleted once the scene is instantiated.
	    delete[] buffer;
	    ogt_vox_destroy_scene(scene);
	}

    return voxels[partial_path].get();
}


g::snd::track& g::asset::store::sound(const std::string& partial_path, bool make_if_missing)
{
	auto itr = sounds.find(partial_path);
	if (itr == sounds.end())
	{
		if (make_if_missing && g::io::file{root + "/tex/" + partial_path}.exists() == false)
		{ // TODO: this isn't exactly right since the extension is ignored and assumed to be wav
			std::vector<int16_t> channel;
			g::snd::track::description desc;

			for (unsigned i = 0; i < desc.frequency; i++)
			{
				float t = i / (float)desc.frequency;
				channel.push_back(0x7FFF * sin(400 * t * M_PI));
			}

			auto tone = std::vector<std::vector<int16_t>>{channel};

			auto path = root + "/snd/" + partial_path;
			g::io::file::make_path(path.c_str());
			g::snd::track_factory::to_wav(path, tone, desc);
		}

		if (std::string::npos != partial_path.find(".wav"))
		{
			sounds[partial_path] = { time(nullptr), g::snd::track_factory::from_wav(root + "/snd/" + partial_path) };
		}
		else if (std::string::npos != partial_path.find(".aiff"))
		{
			sounds[partial_path] = { time(nullptr), g::snd::track_factory::from_wav(root + "/snd/" + partial_path) };
		}
		else if (std::string::npos != partial_path.find(".ogg"))
		{
			sounds[partial_path] = { time(nullptr), g::snd::track_factory::from_ogg(root + "/snd/" + partial_path) };
		}
	}
	else if(hot_reload)
	{
		auto mod_time = g::io::file(root + "/snd/" + partial_path).modified();
		if (mod_time < itr->second.last_accessed && itr->second.loaded_time < mod_time)
		{
			sounds.erase(itr);

			return this->sound(partial_path);
		}
	}

	return sounds[partial_path].get();
}

// g::game::object& g::asset::store::game_object(const std::string& partial_path);
