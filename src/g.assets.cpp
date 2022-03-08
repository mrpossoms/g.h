#include "g.assets.h"
#include "g.utils.h"
#include "g.io.h"

#define OGT_VOX_IMPLEMENTATION
#define OGT_VOXEL_MESHIFY_IMPLEMENTATION

#ifdef __unix__
#include <strings.h>
#endif

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

g::gfx::texture& g::asset::store::tex(const std::string& partial_path)
{
	auto itr = textures.find(partial_path);
	if (itr == textures.end())
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


g::gfx::font& g::asset::store::font(const std::string& partial_path)
{
	auto itr = fonts.find(partial_path);
	if (itr == fonts.end())
	{
		fonts[partial_path] = {time(nullptr), g::gfx::font_factory{}.from_true_type(root + "/font/" + partial_path, 32)};
	}

	return fonts[partial_path].get();
}


g::gfx::mesh<g::gfx::vertex::pos_uv_norm>& g::asset::store::geo(const std::string& partial_path)
{
	auto itr = geos.find(partial_path);
	if (itr == geos.end())
	{
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


g::game::voxels_paletted& g::asset::store::vox(const std::string& partial_path)
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


g::snd::track& g::asset::store::sound(const std::string& partial_path)
{
	auto itr = sounds.find(partial_path);
	if (itr == sounds.end())
	{
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
