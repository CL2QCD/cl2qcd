//opencl_fermionobservables.cl

//!!CP: LT should update this...
void simple_correlator(__global hmc_spinor_field * in, __global hmc_spinor_field * spinor_out, __global hmc_ocl_gaugefield* gaugefield, __global hmc_complex * out, hmc_float kappa, hmc_float mu, hmc_float theta, int cgmax){

  //pseudo scalar, flavour multiplet
  for(int z=0; z<NSPACE; z++) {
    out[z].re = 0;
    out[z].im = 0;
  }

  hmc_spinor_field b[SPINORFIELDSIZE];

  for(int k=0; k<NC*NSPIN; k++) {
    create_point_source(b,k,0,0,kappa,mu,gaugefield);
/*
for(int i = 0; i<SPINORFIELDSIZE; i++){
b[i].re= 0;
b[i].im = 0.;
}
*/
    solver(in, spinor_out, b, gaugefield, kappa, mu, theta, 10000);

    for(int timepos = 0; timepos<NTIME; timepos++) {
      for(int spacepos = 0; spacepos<VOLSPACE; spacepos++) {
	for(int alpha = 0; alpha<NSPIN; alpha++) {
	  for(int c = 0; c<NC; c++) {
	    //	    int j = spinor_element(alpha,c);
	    int n = spinor_field_element(alpha, c, spacepos, timepos);
	    int z = get_spacecoord(spacepos, 3);
	    hmc_complex tmp = spinor_out[n];
	    hmc_complex ctmp = complexconj(&tmp);
	    hmc_complex incr = complexmult(&ctmp,&tmp);
	//    if(timepos == 0 && spacepos == 0){
 	    out[z].re += incr.re;
 	    out[z].im += incr.im;
//}
	  }
	}
      }
    }
  }



  return;
}
