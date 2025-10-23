#pragma once

using ll = long long;

struct ErosionParams {
	ll worldSeed = 424242;
	int numDroplets = 200000;  // keep this under 1e7 so lap doesnt go boom
	int maxSteps = 45;
	float stepSize = 1.0f;

	float initSpeed = 1.0f;
	float initWater = 1.0f;
	float inertia = 0.3f;
	float gravity = 9.81f;
	float evaporateRate = 0.015f;
	float minWater = 0.01f;
	float minSpeed = 0.01f;

	float capacityFactor = 8.0f;
	float erodeRate = 0.5f;
	float depositRate = 0.3f;
	float maxErodePerStep = 0.1f;
	int erosionRadius = 3;

	bool usePerThreadBuffers = true;
};
