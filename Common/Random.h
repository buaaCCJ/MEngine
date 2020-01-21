#pragma once
#include <random>
typedef unsigned int UINT;
class Random
{
private:
	std::mt19937 eng;
	std::uniform_int_distribution<UINT> dist;// (eng.min(), eng.max());
public:
	Random();
	double GetNormFloat();
	UINT GetInt();
	double GetRangedFloat(double min, double max);
};