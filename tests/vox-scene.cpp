#include ".test.h"
#include "g.h"


TEST
{
    g::asset::store assets;

	{
		auto& scene = assets.vox("twofer.vox");

		auto box = scene.corners();

		auto min_corner = std::get<0>(box);
		auto max_corner = std::get<1>(box);
		auto size = max_corner - min_corner;

		std::cerr << "corners: " << min_corner.to_string() << ", " << max_corner.to_string() << std::endl;
		std::cerr << "size: " << size.to_string() << std::endl;

		assert(size.is_near(vec<3, int>{1, 1, 3}));

        // test mating
        auto& pink = scene.instances["pink"];
        auto& black = scene.instances["black"];
        
		assert(!pink.position_of({ 0, 0, 0 }, true).is_near(black.position_of({ 0, 0, 0 }, true)));

        pink.mate(black, {0, 0, 0}, {0, 0, 0});

		assert(pink.position_of({ 0, 0, 0 }, true).is_near(black.position_of({ 0, 0, 0 }, true)));
	}

	{
		auto& scene = assets.vox("tree.vox");

		auto box = scene.corners();

		auto min_corner = std::get<0>(box);
		auto max_corner = std::get<1>(box);
		auto size = max_corner - min_corner;

		std::cerr << "corners: " << min_corner.to_string() << ", " << max_corner.to_string() << std::endl;
		std::cerr << "size: " << size.to_string() << std::endl;

		assert(size.is_near(vec<3, int>{10, 10, 21}));
	}

	{
		auto& scene = assets.vox("farm.vox");

		auto box = scene.corners();

		auto min_corner = std::get<0>(box);
		auto max_corner = std::get<1>(box);
		auto size = max_corner - min_corner;

		std::cerr << "corners: " << min_corner.to_string() << ", " << max_corner.to_string() << std::endl;
		std::cerr << "size: " << size.to_string() << std::endl;

		assert(size.is_near(vec<3, int>{40, 39, 17}));
	}
	return 0;
}
