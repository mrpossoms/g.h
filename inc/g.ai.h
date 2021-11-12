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


namespace evolution
{

struct generation_desc
{
    float select_top=0.1f;
    float mutation_rate=0.01f;
    float fresh_percent = 0.1f;
};

template<typename T>
T breed(T& a, T& b, float mutation_rate)
{
    T child = rand() % 2 ? a : b;

    uint8_t* a_genome = a.genome_buf();
    uint8_t* b_genome = b.genome_buf();
    uint8_t* child_genome = child.genome_buf();

    for (unsigned i = 0; i < child.genome_size(); i++)
    {
        if(rand() % 2 == 0)
        {
            child_genome[i] = a_genome[i];
        }
        else
        {
            child_genome[i] = b_genome[i];
        }

        for (unsigned j = 0; j < 8; j++)
        {
            float r = (rand() % 100) / 100.f;
            if (r < mutation_rate)
            {
                child_genome[i] ^= 1 << j;
            }
        }
    }

    return child;
}

template<typename T>
void generation(std::vector<T>& g_0, std::vector<T>& g_1, const generation_desc& desc)
{
    g_1.resize(0); // empty without freeing

    // sort the input generation
    std::sort(g_0.begin(), g_0.end(), [](const T& a, const T& b){
        return a.score() > b.score();
    });


    // take top percentage and add them to the next generation
    auto top_performer_count = static_cast<unsigned>(g_0.size() * desc.select_top);
    for (unsigned i = 0; i < top_performer_count; i++)
    {
        g_1.push_back(g_0[i]);
    }

    // create some completely new randomized solutions
    auto fresh_count = static_cast<unsigned>(g_0.size() * desc.fresh_percent);
    for (unsigned i = 0; i < fresh_count; i++)
    {
        g_1.push_back({});
    }

    static std::default_random_engine generator;
    std::normal_distribution<float> distribution(0, top_performer_count * 0.3);

    // breed the top performers
    while(g_1.size() < g_0.size())
    {
        unsigned i = std::min<float>(abs(distribution(generator)), g_0.size()-1);
        unsigned j = std::min<float>(abs(distribution(generator)), g_0.size()-1);
        g_1.push_back(breed(g_0[i], g_0[j], desc.mutation_rate));
    }
}

} // end namespace evolution



} // end namespace ai
} // end namespace g
