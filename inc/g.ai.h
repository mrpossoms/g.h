#pragma once
#define XMTYPE float
#include <xmath.h>

namespace g::ai
{

using xmath;

template<size_t X_SIZE, size_t Y_SIZE, size_t LAYER_SIZE=X_SIZE, size_t LAYERS=1>
struct mlp
{
    mat<X_SIZE+1, LAYER_SIZE> in;
    mat<LAYER_SIZE+1, LAYER_SIZE> hidden[LAYERS];
    mat<LAYER_SIZE+1, Y_SIZE> out;

};


}
