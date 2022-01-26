#include ".test.h"
#include "g.proc.h"

/**
 * A test is nothing more than a stripped down C program
 * returning 0 is success. Use asserts to check for errors
 */
TEST
{
    g::proc::thread_pool<3> pool;

    assert(pool.idle_threads() == 3);

    int res[3] = {};
    int a[3] = { 1, 2, 3 };
    int b[3] = { 4, 5, 6 };

    for (unsigned i = 0; i < 3; i++)
    {
        pool.run([&res, &a, &b, i](){
            std::this_thread::sleep_for(std::chrono::seconds(1));
            res[i] = a[i] + b[i];
        });
    }

    assert(pool.idle_threads() == 0);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    assert(pool.idle_threads() == 3);

    for (unsigned i = 0; i < 3; i++)
    {
        std::cerr << "res[" << i << "] = " << res[i] << " expected: " << a[i] + b[i] << std::endl;
        assert(res[i] == a[i] + b[i]);
    }


	return 0;
}
