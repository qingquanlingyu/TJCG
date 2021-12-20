#ifndef OCEAN_H
#define OCEAN_H
#include <random>
#include <complex>
#include <cmath>
#include "area.h"

#define PI 3.1415926535897932384626433832795
#define G 9.80665

struct WavesVerticiesStaticVal
{
	std::vector<std::complex<float>> h0;
	std::vector<std::complex<float>> h0Conj;
	std::vector<float> dispersion;
};
class Ocean : public Area
{
private:
	std::complex<float> i = std::complex<float>(0,1);
	int N, M;
	float amplitude = 0.001f;
	glm::vec2 windDir = normalize(glm::vec2(1, 0.3f));
	float windSpeed = 30;
	float lambda = -1;

	std::default_random_engine generator;
	std::normal_distribution<float> gaussian;
	std::normal_distribution<float> gaussian2;

	std::complex<float> h_(int n, int m, float t);
	std::complex<float> h_0(int n, int m);
	float dispersion_relation(int n, int m);
	std::complex<float> philipsSpectrum(int n, int m);

public:
	WavesVerticiesStaticVal vVar;
	Ocean(int nSampleX, int nSampleZ, float LengthX, float LengthY, int nInstanceX, int nInstanceZ);
	float LengthX, LengthZ;

};

Ocean::Ocean(int nSampleX, int nSampleZ, float LengthX, float LengthZ, int nInstanceX, int nInstanceZ)
	: Area(nSampleX, nSampleZ, LengthX, LengthZ, nInstanceX, nInstanceZ)
{
	this->N = nSampleX;
	this->M = nSampleZ;
	this->LengthX = LengthX;
	this->LengthZ = LengthZ;

	gaussian = std::normal_distribution<float>(0, 1);
	gaussian2 = std::normal_distribution<float>(0, 1);
	int pos = 0;
	for (int m = 0; m < M; m++) for (int n = 0; n < N; n++)
	{
		vertices[pos].x -= N / 2.0;
		vertices[pos].z -= M / 2.0;

		glm::vec2 K;
		K.x = (2 * PI*n - PI*N) / LengthX;
		K.y = (2 * PI*m - PI*M) / LengthZ;

		vVar.h0.push_back(h_0(n, m));
		vVar.h0Conj.push_back(conj(h_0(-n, -m)));
		vVar.dispersion.push_back(dispersion_relation(n, m));

		pos++;
	}
}

std::complex<float> Ocean::h_(int n, int m, float t)
{
	int pos = n * N + m;
	std::complex<float> e_component, e_componentConj;
	
	e_component = exp(i *(float)(vVar.dispersion[pos] * t));
	e_componentConj = exp(-i *(float)(vVar.dispersion[pos] * t));

	return vVar.h0[pos] * e_component + vVar.h0Conj[pos] * e_componentConj;
}

float Ocean::dispersion_relation(int n, int m)
{
	float w0 = 2 * PI / 200.0;
	glm::vec2 K;
	K.x = (2 * PI*n - PI*N) / LengthX;
	K.y = (2 * PI*m - PI*M) / LengthZ;
	float dispersionTerm = sqrt(G * sqrt(K.x * K.x + K.y * K.y));
	return floor(dispersionTerm/w0) * w0;
}

std::complex<float> Ocean::h_0(int n, int m)
{
	std::complex<float> complexRand(gaussian(generator), gaussian2(generator));
	return complexRand * sqrt( philipsSpectrum(n, m) / 2.0f);
}

std::complex<float> Ocean::philipsSpectrum(int n, int m)
{
	glm::vec2 K;
	K.x = (2 * PI*n - PI*N) / LengthX;
	K.y = (2 * PI*m - PI*M) / LengthZ;
	
	float KLength = length(K);
	// Critical if n = N/2
	if (KLength == 0) return 0;

	float waveLength = windSpeed*windSpeed / G;
	std::complex<float> e_component = exp(-1.0 / pow((KLength * waveLength), 2));

	float wave_component = pow(dot(normalize(K), windDir), 4) / pow(KLength, 4);
	
	float damping = 0.001;
	std::complex<float> damp_compo = exp(-KLength * KLength * waveLength * waveLength * damping * damping);
	
	return amplitude * wave_component * e_component * damp_compo;
}


#endif