#ifndef PERLIN_H
#define PERLIN_H

typedef unsigned int perlin_seed_t;

typedef struct {
    float scale;
    perlin_seed_t seed;
} perlin3d_t;

int perlin3d_init(perlin3d_t *perlin3d, float scale, perlin_seed_t seed);

void perlin3d_destroy(perlin3d_t *perlin3d);

// Returns NaN on error.
float perlin3d_get(perlin3d_t *perlin3d, const float point[3]);

#endif
