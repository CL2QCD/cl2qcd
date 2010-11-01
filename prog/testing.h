#ifndef _TESTINGH_
#define _TESTINGH_

#include <cstdlib>
#include <cstdio>

#include "hmcerrs.h"
#include "globals.h"
#include "types.h"
#include "operations.h"
#include "geometry.h"

void testing_spinor();
void print_su3mat(hmc_su3matrix A);
void testing_su3mat();
void testing_gaugefield();
void testing_geometry();

#endif
