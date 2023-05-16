#include "perlin.h"
#include "util.h"

#include <math.h>
#include <stdlib.h>

int perlin3d_init(perlin3d_t *perlin3d, float scale, perlin_seed_t seed) {
    perlin3d->scale = scale;
    perlin3d->seed = seed;
    return 0;
}

void perlin3d_destroy(perlin3d_t *perlin3d) {
}

static unsigned int generate_lattice_point_seed(float lattice_point[3], perlin_seed_t seed) {
    unsigned total_bit_count = sizeof(unsigned int) * 8;
    unsigned bits_per_part = total_bit_count / 4;

    unsigned mask = (1 << bits_per_part) - 1;

    return ((int) lattice_point[0] & mask)
        + (((int) lattice_point[1] & mask) << bits_per_part)
        + (((int) lattice_point[2] & mask) << (2 * bits_per_part))
	+ ((seed & mask) << (3 * bits_per_part));
}

static void gradient_for_lattice_point(float gradient[3], float lattice_point[3], perlin_seed_t seed) {
    unsigned int lattice_seed = generate_lattice_point_seed(lattice_point, seed);

    float sum = 0;
    for (int i = 0;  i < 3;  i++) {
	gradient[i] = rand_r_in_range(&lattice_seed, -1.0, 1.0);
	sum += gradient[i] * gradient[i];
    }

    // normalize
    float length = sqrtf(sum);
    for (int i = 0;  i < 3;  i++) {
	gradient[i] /= length;
    }
}

static float gradient_dot_product(perlin_seed_t seed, float corner[3], float point[3]) {
    float gradient[3];
    gradient_for_lattice_point(gradient, corner, seed);

    float sum = 0;
    for (int i = 0;  i < 3;  i++) {
	sum += gradient[i] * (point[i] - corner[i]);
    }

    return sum;
}

static float get_recurse(perlin_seed_t seed, float corner[3], float point[3], unsigned axis) {
    if (axis > 2) {
	return gradient_dot_product(seed, corner, point);
    }

    float low = get_recurse(seed, corner, point, axis + 1);
    corner[axis] += 1;
    float high = get_recurse(seed, corner, point, axis + 1);
    corner[axis] -= 1;

    float distance = point[axis] - corner[axis];
    float eased = 3 * distance * distance - 2 * distance * distance * distance;

    return eased * high + (1 - eased) * low;
}

float perlin3d_get(perlin3d_t *perlin3d, const float point[3]) {
    float mypoint[3];
    float corner[3];
    for (int i = 0;  i < 3;  i++) {
	mypoint[i] = point[i] / perlin3d->scale;
        corner[i] = floorf(mypoint[i]);
    }

    return get_recurse(perlin3d->seed, corner, mypoint, 0);
}
