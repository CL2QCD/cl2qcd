//opencl_operations_spinor

//!!CP: many of the host-functions make no sense on the device and are deleted..

void su3matrix_times_colorvector(hmc_ocl_su3matrix* u, hmc_color_vector* in, hmc_color_vector* out){
#ifdef _RECONSTRUCT_TWELVE_
  for(int a=0; a<NC-1; a++) {
    out[a] = hmc_complex_zero;
    for(int b=0; b<NC; b++) {
      hmc_complex tmp = complexmult(&((*u)[ocl_su3matrix_element(a,b)]),&(in[b]));
      complexaccumulate(&out[a],&tmp);
    }
  }
  out[2] = hmc_complex_zero;
  for(int b=0; b<NC; b++) {
    hmc_complex rec = reconstruct_su3(u,b);
    hmc_complex tmp = complexmult(&rec,&(in[b]));
    complexaccumulate(&out[2],&tmp);
  }
#else
  for(int a=0; a<NC; a++) {
    out[a] = hmc_complex_zero;
    for(int b=0; b<NC; b++) {
      hmc_complex tmp = complexmult(&((u)[ocl_su3matrix_element(a,b)]),&(in[b]));
      complexaccumulate(&(out[a]),&tmp);
    }
  }
#endif
  return;
}

void set_local_zero_spinor(hmc_spinor* inout){
  for(int j=0; j<SPINORSIZE; j++) {
    inout[j].re = 0;
    inout[j].im = 0;
  }
  return;
}

hmc_float spinor_squarenorm(hmc_spinor* in){
  hmc_float res=0;
  for(int j=0; j<SPINORSIZE; j++) {
    hmc_complex tmp = complexconj(&in[j]);
    hmc_complex incr= complexmult(&tmp,&in[j]);
    res+=incr.re;
  }
  return res;
}

void real_multiply_spinor(hmc_spinor* inout, hmc_float factor){
  for(int j=0; j<SPINORSIZE; j++) {
		inout[j].re *=factor;
		inout[j].im *=factor;
	}
  return;
}

void spinprojectproduct_gamma0(hmc_ocl_su3matrix* u, hmc_spinor* spin,hmc_float sign){
  //out = u*(1+sign*gamma0)*in
  hmc_color_vector vec1[NC];
  hmc_color_vector vec2[NC];
  hmc_color_vector uvec1[NC];
  hmc_color_vector uvec2[NC];
  for(int c=0; c<NC; c++) {
    vec1[c].re = spin[spinor_element(0,c)].re - sign*spin[spinor_element(3,c)].im;
    vec1[c].im = spin[spinor_element(0,c)].im + sign*spin[spinor_element(3,c)].re;
    vec2[c].re = spin[spinor_element(1,c)].re - sign*spin[spinor_element(2,c)].im;
    vec2[c].im = spin[spinor_element(1,c)].im + sign*spin[spinor_element(2,c)].re;
  }
  su3matrix_times_colorvector(u,vec1,uvec1);
  su3matrix_times_colorvector(u,vec2,uvec2);
  for(int c=0; c<NC; c++) {
    spin[spinor_element(0,c)].re = uvec1[c].re;
    spin[spinor_element(0,c)].im = uvec1[c].im;
    spin[spinor_element(1,c)].re = uvec2[c].re;
    spin[spinor_element(1,c)].im = uvec2[c].im;
    spin[spinor_element(2,c)].re = sign*uvec2[c].im;
    spin[spinor_element(2,c)].im = -sign*uvec2[c].re;
    spin[spinor_element(3,c)].re = sign*uvec1[c].im;
    spin[spinor_element(3,c)].im = -sign*uvec1[c].re;
  }
  return;
}

void spinprojectproduct_gamma1(hmc_ocl_su3matrix* u, hmc_spinor* spin, hmc_float sign){
  //out = u*(1+sign*gamma1)*in
  hmc_color_vector vec1[NC];
  hmc_color_vector vec2[NC];
  hmc_color_vector uvec1[NC];
  hmc_color_vector uvec2[NC];
  for(int c=0; c<NC; c++) {
    vec1[c].re = spin[spinor_element(0,c)].re - sign*spin[spinor_element(3,c)].re;
    vec1[c].im = spin[spinor_element(0,c)].im - sign*spin[spinor_element(3,c)].im;
    vec2[c].re = spin[spinor_element(1,c)].re + sign*spin[spinor_element(2,c)].re;
    vec2[c].im = spin[spinor_element(1,c)].im + sign*spin[spinor_element(2,c)].im;
  }
  su3matrix_times_colorvector(u,vec1,uvec1);
  su3matrix_times_colorvector(u,vec2,uvec2);
  for(int c=0; c<NC; c++) {
    spin[spinor_element(0,c)].re = uvec1[c].re;
    spin[spinor_element(0,c)].im = uvec1[c].im;
    spin[spinor_element(1,c)].re = uvec2[c].re;
    spin[spinor_element(1,c)].im = uvec2[c].im;
    spin[spinor_element(2,c)].re = sign*uvec2[c].re;
    spin[spinor_element(2,c)].im = sign*uvec2[c].im;
    spin[spinor_element(3,c)].re = -sign*uvec1[c].re;
    spin[spinor_element(3,c)].im = -sign*uvec1[c].im;
  }
  return;
}

void spinprojectproduct_gamma2(hmc_ocl_su3matrix* u, hmc_spinor* spin, hmc_float sign){
  //out = u*(1+sign*gamma2)*in
  hmc_color_vector vec1[NC];
  hmc_color_vector vec2[NC];
  hmc_color_vector uvec1[NC];
  hmc_color_vector uvec2[NC];
  for(int c=0; c<NC; c++) {
    vec1[c].re = spin[spinor_element(0,c)].re - sign*spin[spinor_element(2,c)].im;
    vec1[c].im = spin[spinor_element(0,c)].im + sign*spin[spinor_element(2,c)].re;
    vec2[c].re = spin[spinor_element(1,c)].re + sign*spin[spinor_element(3,c)].im;
    vec2[c].im = spin[spinor_element(1,c)].im - sign*spin[spinor_element(3,c)].re;
  }
  su3matrix_times_colorvector(u,vec1,uvec1);
  su3matrix_times_colorvector(u,vec2,uvec2);
  for(int c=0; c<NC; c++) {
    spin[spinor_element(0,c)].re = uvec1[c].re;
    spin[spinor_element(0,c)].im = uvec1[c].im;
    spin[spinor_element(1,c)].re = uvec2[c].re;
    spin[spinor_element(1,c)].im = uvec2[c].im;
    spin[spinor_element(2,c)].re = sign*uvec1[c].im;
    spin[spinor_element(2,c)].im = -sign*uvec1[c].re;
    spin[spinor_element(3,c)].re = -sign*uvec2[c].im;
    spin[spinor_element(3,c)].im = sign*uvec2[c].re;
  }
  return;
}

void spinprojectproduct_gamma3(hmc_ocl_su3matrix* u, hmc_spinor* spin, hmc_float sign){
  //out = u*(1+sign*gamma3)*in
  hmc_color_vector vec1[NC];
  hmc_color_vector vec2[NC];
  hmc_color_vector uvec1[NC];
  hmc_color_vector uvec2[NC];
  for(int c=0; c<NC; c++) {
    vec1[c].re = spin[spinor_element(0,c)].re + sign*spin[spinor_element(2,c)].re;
    vec1[c].im = spin[spinor_element(0,c)].im + sign*spin[spinor_element(2,c)].im;
    vec2[c].re = spin[spinor_element(1,c)].re + sign*spin[spinor_element(3,c)].re;
    vec2[c].im = spin[spinor_element(1,c)].im + sign*spin[spinor_element(3,c)].im;
  }
  su3matrix_times_colorvector(u,vec1,uvec1);
  su3matrix_times_colorvector(u,vec2,uvec2);
  for(int c=0; c<NC; c++) {
    spin[spinor_element(0,c)].re = uvec1[c].re;
    spin[spinor_element(0,c)].im = uvec1[c].im;
    spin[spinor_element(1,c)].re = uvec2[c].re;
    spin[spinor_element(1,c)].im = uvec2[c].im;
    spin[spinor_element(2,c)].re = sign*uvec1[c].re;
    spin[spinor_element(2,c)].im = sign*uvec1[c].im;
    spin[spinor_element(3,c)].re = sign*uvec2[c].re;
    spin[spinor_element(3,c)].im = sign*uvec2[c].im;
  }
  return;
}

void spinors_accumulate(hmc_spinor* inout, hmc_spinor* incr){
  for(int j=0; j<SPINORSIZE; j++) {
    inout[j].re += incr[j].re;
    inout[j].im += incr[j].im;
  }
  return;
}

void multiply_spinor_i_factor_gamma5(hmc_spinor* in, hmc_spinor* out, hmc_float factor){
  for(int c=0; c<NC; c++) {
    out[spinor_element(0,c)].re = -factor*in[spinor_element(0,c)].im;
    out[spinor_element(1,c)].re = -factor*in[spinor_element(1,c)].im;
    out[spinor_element(2,c)].re = factor*in[spinor_element(2,c)].im;
    out[spinor_element(3,c)].re = factor*in[spinor_element(3,c)].im;
    out[spinor_element(0,c)].im = factor*in[spinor_element(0,c)].re;
    out[spinor_element(1,c)].im = factor*in[spinor_element(1,c)].re;
    out[spinor_element(2,c)].im = -factor*in[spinor_element(2,c)].re;
    out[spinor_element(3,c)].im = -factor*in[spinor_element(3,c)].re;
  }
  return;
}

void multiply_spinor_gamma0(hmc_spinor* in,hmc_spinor* out){
  for(int c=0; c<NC; c++) {
    out[spinor_element(0,c)].re = -in[spinor_element(3,c)].im;
    out[spinor_element(1,c)].re = -in[spinor_element(2,c)].im;
    out[spinor_element(2,c)].re = in[spinor_element(1,c)].im;
    out[spinor_element(3,c)].re = in[spinor_element(0,c)].im;
    out[spinor_element(0,c)].im = in[spinor_element(3,c)].re;
    out[spinor_element(1,c)].im = in[spinor_element(2,c)].re;
    out[spinor_element(2,c)].im = -in[spinor_element(1,c)].re;
    out[spinor_element(3,c)].im = -in[spinor_element(0,c)].re;
  }
  return;
}
void multiply_spinor_gamma1(hmc_spinor* in,hmc_spinor* out){
  for(int c=0; c<NC; c++) {
    out[spinor_element(0,c)].re = -in[spinor_element(3,c)].re;
    out[spinor_element(1,c)].re = in[spinor_element(2,c)].re;
    out[spinor_element(2,c)].re = in[spinor_element(1,c)].re;
    out[spinor_element(3,c)].re = -in[spinor_element(0,c)].re;
    out[spinor_element(0,c)].im = -in[spinor_element(3,c)].im;
    out[spinor_element(1,c)].im = in[spinor_element(2,c)].im;
    out[spinor_element(2,c)].im = in[spinor_element(1,c)].im;
    out[spinor_element(3,c)].im = -in[spinor_element(0,c)].im;
  }
  return;
}
void multiply_spinor_gamma2(hmc_spinor* in,hmc_spinor* out){
  for(int c=0; c<NC; c++) {
    out[spinor_element(0,c)].re = -in[spinor_element(2,c)].im;
    out[spinor_element(1,c)].re = in[spinor_element(3,c)].im;
    out[spinor_element(2,c)].re = in[spinor_element(0,c)].im;
    out[spinor_element(3,c)].re = -in[spinor_element(1,c)].im;
    out[spinor_element(0,c)].im = in[spinor_element(2,c)].re;
    out[spinor_element(1,c)].im = -in[spinor_element(3,c)].re;
    out[spinor_element(2,c)].im = -in[spinor_element(0,c)].re;
    out[spinor_element(3,c)].im = in[spinor_element(1,c)].re;
  }
  return;
}
void multiply_spinor_gamma3(hmc_spinor* in,hmc_spinor* out){
  for(int c=0; c<NC; c++) {
    out[spinor_element(0,c)].re = in[spinor_element(2,c)].re;
    out[spinor_element(1,c)].re = in[spinor_element(3,c)].re;
    out[spinor_element(2,c)].re = in[spinor_element(0,c)].re;
    out[spinor_element(3,c)].re = in[spinor_element(1,c)].re;
    out[spinor_element(0,c)].im = in[spinor_element(2,c)].im;
    out[spinor_element(1,c)].im = in[spinor_element(3,c)].im;
    out[spinor_element(2,c)].im = in[spinor_element(0,c)].im;
    out[spinor_element(3,c)].im = in[spinor_element(1,c)].im;
  }
  return;
}

void su3matrix_times_spinor(hmc_ocl_su3matrix* u, hmc_spinor* in, hmc_spinor* out){
  for (int alpha=0; alpha<NSPIN; alpha++) {
    hmc_color_vector vec_in[NC];
    hmc_color_vector vec_out[NC];
    for(int c=0; c<NC; c++) {
      vec_in[c].re = in[spinor_element(alpha,c)].re;
      vec_in[c].im = in[spinor_element(alpha,c)].im;
    }
    su3matrix_times_colorvector(u, vec_in, vec_out);
    for(int c=0; c<NC; c++) {
      out[spinor_element(alpha,c)].re = vec_out[c].re;
      out[spinor_element(alpha,c)].im = vec_out[c].im;
    }
  }
  return;
}

void spinor_apply_bc(hmc_spinor * in, hmc_float theta){
  for(int n = 0; n<SPINORSIZE; n++){
    hmc_float tmp1 = in[n].re;
    hmc_float tmp2 = in[n].im;
    in[n].re = cos(theta)*tmp1 - sin(theta)*tmp2;
    in[n].im = sin(theta)*tmp1 + cos(theta)*tmp2;
  }
  return; 
}

