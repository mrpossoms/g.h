#pragma once
#define XMTYPE float
#include <xmath.h>
#include <random>

namespace g
{
namespace ai
{

using namespace xmath;

template<size_t X_SIZE, size_t Y_SIZE, size_t LAYER_SIZE=X_SIZE, size_t LAYERS=1>
struct mlp
{
    mat<LAYER_SIZE+1, X_SIZE+1> w_in;
    mat<LAYER_SIZE+1, LAYER_SIZE+1> w_hidden[LAYERS];
    mat<Y_SIZE, LAYER_SIZE+1> w_out;

    vec<Y_SIZE> evaluate(const vec<X_SIZE+1>& x_aug)
    {
        vec<LAYER_SIZE+1> a = w_in * x_aug;
        a[LAYER_SIZE] = 1;

        for (unsigned li = 0; li < LAYERS; li++)
        {
            // TODO apply activation functions
            a = w_hidden[li] * a;
            a[LAYER_SIZE] = 1;
        }

        return w_out * a;
    }

    void initialize()
    {
        static std::default_random_engine generator;
        std::normal_distribution<double> distribution(0.f, 0.5f);

        w_in.initialize([&](float r, float c) {
            return distribution(generator);
        });

        for (unsigned i = LAYERS; i--;)
        {
            w_hidden[i].initialize([&](float r, float c) {
                return distribution(generator);
            });
        }

        w_out.initialize([&](float r, float c) {
            return distribution(generator);
        });
    }
};

}
}
