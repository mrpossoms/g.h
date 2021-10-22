#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <iostream>


bool near(double x, double y, double tol=0.000001f)
{
	return fabs(x - y) <= tol;
}

#define TEST int main (int argc, const char* argv[])
