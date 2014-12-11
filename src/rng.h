#ifndef RNG_H
#define RNG_H

long rng(long val1, long val2);
double rng_float(double val1, double val2);
bool one_in(int chance);
bool one_in_improved(double chance);
bool x_in_y(double x, double y);
int dice(int number, int sides);

int djb2_hash(const unsigned char *input);

enum uid_spec {
    UID_NONE = 0,
    UID_SPARE_MAG_RELOAD,
    UID_MIN = 1000
};

typedef unsigned long UID;

UID generate_uid();

#endif
