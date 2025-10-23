#include "util.h"

namespace rng_util {
RNG::RNG(ll seed) : _state(seed) {}

int RNG::nextInt() {
	ll x = _state;
	x ^= x >> 12;
	x ^= x << 25;
	x ^= x >> 27;
	_state = x;
	return static_cast<int>((x * 2685821657736338717LL) >> 32);
}

float RNG::nextFloat() { return (nextInt() / 4294967296.0f); }

ll splitmix(ll &state) {
	ll z = (state += 2654435769LL);
	z = (z ^ (z >> 30)) * 2246822507LL;
	z = (z ^ (z >> 27)) * 3255373325LL;
	return z ^ (z >> 31);
}

}  // namespace rng_util
