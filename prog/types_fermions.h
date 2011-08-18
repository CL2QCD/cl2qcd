/** @file
 * Common fermion types used by HMC, both on host and OpenCL device.
 */

#ifndef _TYPES_FERMIONSH_
#define _TYPES_FERMIONSH_

#include "types.h"

//CP: new defs for spinors
typedef struct {
	hmc_complex e0;
	hmc_complex e1;
	hmc_complex e2;
	
} su3vec;

typedef struct {
	su3vec e0;
	su3vec e1;
	su3vec e2;
	su3vec e3;
} spinor;

typedef struct {
	su3vec e0;
	su3vec e1;
} halfspinor;

typedef spinor spinorfield;
typedef spinor spinorfield_eoprec;

//CP: this an algebraelement
typedef struct {
  hmc_float e0;
 	hmc_float e1;
  hmc_float e2;
  hmc_float e3;
  hmc_float e4;
  hmc_float e5;
  hmc_float e6;
  hmc_float e7;
} ae;

#endif /* _TYPES_FERMIONSH_ */

