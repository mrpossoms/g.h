#include ".test.h"
#include "g.h"

/**
 * A test is nothing more than a stripped down C program
 * returning 0 is success. Use asserts to check for errors
 */
TEST
{
    uint32_t data1[] = { 0x88888888, 0x99999999, 0xaaaaaaaa, 0xbbbbbbbb, 0xcccccccc, 0xdddddddd, 0xeeeeeeee, 0xffffffff };
	g::game::voxels<uint32_t> vox1(data1, 2, 2, 2);
    uint32_t data2[] = { 0x88888881, 0x99999999, 0xaaaaaaaa, 0xbbbbbbbb, 0xcccccccc, 0xdddddddd, 0xeeeeeeee, 0xffffffff };
    g::game::voxels<uint32_t> vox2(data2, 2, 2, 2);

    assert(vox1.hash() != vox2.hash());

	return 0;
}
