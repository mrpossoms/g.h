#include ".test.h"
#include "g.h"


TEST
{
    g::asset::store assets;

	auto& farm_scene = assets.vox("farm.vox");

	auto T = farm_scene.instances["crops"].global_transform();

	std::cerr << "crops transform:\n" << T.to_string() << std::endl;

	return 0;
}
