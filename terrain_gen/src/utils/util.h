#pragma once

using ll = long long;

#pragma once

namespace rng_util {
using ll = long long;
class RNG {
   public:
	explicit RNG(ll seed = 17112005);

	int nextInt();
	float nextFloat();

   private:
	ll _state;
};

ll splitmix(ll &state);
}  // namespace rng_util