/** @file
 * Device code for the heatbath update
 */

//opencl_update_heatbath.cl

Matrixsu2_pauli SU2Update(const hmc_float alpha, __global hmc_ocl_ran * rnd)
{
	Matrixsu2_pauli out;
  
	hmc_float delta;
	hmc_float a0 ;
	hmc_float eta ;
	do {
		delta = -log(ocl_new_ran(rnd))/alpha*pow(cos(2. * PI * ocl_new_ran(rnd)), 2.) -log(ocl_new_ran(rnd))/alpha;
		a0 = 1.-delta;
		eta = ocl_new_ran(rnd);
	} while ( (1.-0.5*delta) < eta*eta);
	hmc_float phi = 2.*PI*ocl_new_ran(rnd);
	hmc_float theta = asin(2.*ocl_new_ran(rnd) - 1.);
	out.e00 = a0;
	out.e01 = sqrt(1.-a0 * a0)*cos(theta) * cos(phi);
	out.e10 = sqrt(1.-a0 * a0)*cos(theta) * sin(phi);
	out.e11 = sqrt(1.-a0 * a0)*sin(theta);
	
	return out;
}

void inline perform_heatbath(__global ocl_s_gaugefield* gaugefield, const hmc_float beta, const int mu, __global hmc_ocl_ran * rnd, int pos, int t, int id)
{
  
	Matrixsu3 U;
	Matrix3x3 W;
	Matrix3x3 staplematrix;
	int order[3];
	Matrixsu2 w;
	Matrixsu2_pauli w_pauli;
	Matrixsu2_pauli r_pauli;
	hmc_float beta_new;
	hmc_float k;

	random_1_2_3(order, &rnd[id]);
	
	U = get_matrixsu3(gaugefield, pos, t, mu);
	U = project_su3(U);

	staplematrix = calc_staple(gaugefield, pos, t, mu);
	
 	for(int i=0; i<NC; i++) {

	  	
		W = matrix_su3to3x3 (U);
		
		W = multiply_matrix3x3 (W, staplematrix);

		w = reduction(W, order[i]);

		w_pauli.e00 = 0.5*(w.e00.re + w.e11.re);
		w_pauli.e01 = 0.5*(w.e01.im + w.e10.im);
		w_pauli.e10 = 0.5*(w.e01.re - w.e10.re);
		w_pauli.e11 = 0.5*(w.e00.im - w.e11.im);
		k = sqrt(  w_pauli.e00*w_pauli.e00 +  w_pauli.e01*w_pauli.e01 + w_pauli.e10*w_pauli.e10 + w_pauli.e11*w_pauli.e11  );
		
		beta_new =  2.*beta / NC*k;

		r_pauli = SU2Update(beta_new, &rnd[id]);

		w.e00.re = (r_pauli.e00*w_pauli.e00 + r_pauli.e01*w_pauli.e01 + r_pauli.e10*w_pauli.e10 + r_pauli.e11*w_pauli.e11 )/k;
		w.e00.im = (w_pauli.e00*r_pauli.e11 - w_pauli.e11*r_pauli.e00 + r_pauli.e01*w_pauli.e10 - r_pauli.e10*w_pauli.e01 )/k;
		w.e01.re = (w_pauli.e00*r_pauli.e10 - w_pauli.e10*r_pauli.e00 + r_pauli.e11*w_pauli.e01 - r_pauli.e01*w_pauli.e11 )/k;
		w.e01.im = (w_pauli.e00*r_pauli.e01 - w_pauli.e01*r_pauli.e00 + r_pauli.e10*w_pauli.e11 - r_pauli.e11*w_pauli.e10 )/k;
		w.e10.re = -(w_pauli.e00*r_pauli.e10 - w_pauli.e10*r_pauli.e00 + r_pauli.e11*w_pauli.e01 - r_pauli.e01*w_pauli.e11 )/k;
		w.e10.im = (w_pauli.e00*r_pauli.e01 - w_pauli.e01*r_pauli.e00 + r_pauli.e10*w_pauli.e11 - r_pauli.e11*w_pauli.e10 )/k;
		w.e11.re = (r_pauli.e00*w_pauli.e00 + r_pauli.e01*w_pauli.e01 + r_pauli.e10*w_pauli.e10 + r_pauli.e11*w_pauli.e11 )/k;
		w.e11.im = -(w_pauli.e00*r_pauli.e11 - w_pauli.e11*r_pauli.e00 + r_pauli.e01*w_pauli.e10 - r_pauli.e10*w_pauli.e01 )/k;

		//old and without structs
		/*
		w_pauli[0] = w_pauli[0]/k;
		w_pauli[1] = -w_pauli[1]/k;
		w_pauli[2] = -w_pauli[2]/k;
		w_pauli[3] = -w_pauli[3]/k;

		hmc_float su2_tmp[su2_entries];
		su2_tmp[0] = r_pauli[0]*w_pauli[0] - r_pauli[1]*w_pauli[1] - r_pauli[2]*w_pauli[2] - r_pauli[3]*w_pauli[3] ;
		su2_tmp[1] = w_pauli[0]*r_pauli[1] + w_pauli[1]*r_pauli[0] - r_pauli[2]*w_pauli[3] + r_pauli[3]*w_pauli[2] ;
		su2_tmp[2] = w_pauli[0]*r_pauli[2] + w_pauli[2]*r_pauli[0] - r_pauli[3]*w_pauli[1] + r_pauli[1]*w_pauli[3] ;
		su2_tmp[3] = w_pauli[0]*r_pauli[3] + w_pauli[3]*r_pauli[0] - r_pauli[1]*w_pauli[2] + r_pauli[2]*w_pauli[1] ;
		r_pauli[0] = su2_tmp[0];
		r_pauli[1] = su2_tmp[1];
		r_pauli[2] = su2_tmp[2];
		r_pauli[3] = su2_tmp[3];

		//go back to a su2 matrix in standard basis
		w[0].re = r_pauli[0];
		w[0].im = r_pauli[3];
		w[1].re = r_pauli[2];
		w[1].im = r_pauli[1];
		w[2].re = -r_pauli[2];
		w[2].im = r_pauli[1];
		w[3].re = r_pauli[0];
		w[3].im = -r_pauli[3];
		*/
		
		Matrixsu3 extW;
		extW = extend (order[i], w);
		
		U = multiply_matrixsu3 (extW, U);
	}
	
 	put_matrixsu3(gaugefield, U, pos, t, mu);

	//Überprüft, ob die erzeugte Matrix unitär ist
	//Ja, falls trace.re = 3.0 und trace.im = 0.0
	/*	
	hmc_complex detu = det_matrixsu3 (U);
	printf("det: %f \t %f \n", detu.re, detu.im);
	
 	Matrixsu3 blubb;
 	Matrixsu3 adjU;
	adjU = adjoint_matrixsu3(U);
 	blubb = multiply_matrixsu3 (adjU, U);
	printf("%f %f \t %f %f \t %f %f \n",blubb.e00.re, blubb.e00.im, blubb.e01.re, blubb.e01.im, blubb.e02.re, blubb.e02.im);
	printf("%f %f \t %f %f \t %f %f \n",blubb.e10.re, blubb.e10.im, blubb.e11.re, blubb.e11.im, blubb.e12.re, blubb.e12.im);
// 	printf("%f %f \t %f %f \t %f %f \n",blubb.e20.re, blubb.e20.im, blubb.e21.re, blubb.e21.im, blubb.e22.re, blubb.e22.im);
	hmc_complex u0 = reconstruct_su3 (blubb, 0);
	hmc_complex u1 = reconstruct_su3 (blubb, 1);
	hmc_complex u2 = reconstruct_su3 (blubb, 2);
	printf("%f %f \t %f %f \t %f %f \n",u0.re, u0.im, u1.re, u1.im, u2.re, u2.im);
	printf("\n");
	hmc_complex trace;
 	trace = trace_matrixsu3 (blubb);
 	printf (" U * adj(u) %f \n", trace.re);
 	printf (" U * adj(u) %f \n", trace.im);
	*/
}



__kernel void heatbath_even(__global ocl_s_gaugefield * gaugefield, const hmc_float beta, const int mu, __global hmc_ocl_ran * rnd)
{
	int t, pos, id, id_tmp, size;
	id_tmp = get_global_id(0);
	size = get_global_size(0);
	for(id = id_tmp; id<VOLSPACE*NTIME/2; id+=size) {
		get_even_site(id, &pos, &t);
		perform_heatbath(gaugefield, beta, mu, rnd, pos, t, id_tmp);
	}
}

__kernel void heatbath_odd(__global ocl_s_gaugefield* gaugefield, const hmc_float beta, const int mu, __global hmc_ocl_ran * rnd)
{
	int t, pos, id, id_tmp, size;
	id_tmp = get_global_id(0);
	size = get_global_size(0);
	for(id = id_tmp; id<VOLSPACE*NTIME/2; id+=size) {
		get_odd_site(id, &pos, &t);
		perform_heatbath(gaugefield, beta, mu, rnd, pos, t, id_tmp);
	}
}


void inline perform_overrelaxing(__global ocl_s_gaugefield* gaugefield, const hmc_float beta, const int mu, __global hmc_ocl_ran * rnd, int pos, int t, int id)
{
  
	Matrixsu3 U;
	Matrix3x3 W;
	Matrix3x3 staplematrix;

	Matrixsu2 w;
	Matrixsu2_pauli w_pauli;
	hmc_float k;
	int order[3];

	random_1_2_3(order, &rnd[id]);
	U = get_matrixsu3(gaugefield, pos, t, mu);
	U=project_su3(U);
	
	staplematrix = calc_staple(gaugefield, pos, t, mu);

	Matrixsu3 extW;

	for(int i=0; i<NC; i++) {
		W = matrix_su3to3x3 (U);
		W = multiply_matrix3x3 (W, staplematrix);
	  
		w = reduction(W, order[i]);
		
		w_pauli.e00 = 0.5*(w.e00.re + w.e11.re);
		w_pauli.e01 = 0.5*(w.e01.im + w.e10.im);
		w_pauli.e10 = 0.5*(w.e01.re - w.e10.re);
		w_pauli.e11 = 0.5*(w.e00.im - w.e11.im);
		k = sqrt(  w_pauli.e00*w_pauli.e00 +  w_pauli.e01*w_pauli.e01 + w_pauli.e10*w_pauli.e10 + w_pauli.e11*w_pauli.e11  );

		w.e00.re = (w_pauli.e00*w_pauli.e00 - w_pauli.e01*w_pauli.e01 - w_pauli.e10*w_pauli.e10 - w_pauli.e11*w_pauli.e11)/k/k;
		w.e00.im = (-2.*w_pauli.e00*w_pauli.e11)/k/k;
		w.e01.re = (-2.*w_pauli.e00*w_pauli.e10)/k/k;
		w.e01.im = (-2.*w_pauli.e00*w_pauli.e01)/k/k;
		w.e10.re = (2.*w_pauli.e00*w_pauli.e10)/k/k;
		w.e10.im = (-2.*w_pauli.e00*w_pauli.e01)/k/k;
		w.e11.re = (w_pauli.e00*w_pauli.e00 - w_pauli.e01*w_pauli.e01 - w_pauli.e10*w_pauli.e10 - w_pauli.e11*w_pauli.e11)/k/k;
		w.e11.im = (2.*w_pauli.e00*w_pauli.e11)/k/k;

		extW = extend (order[i], w);
		U = multiply_matrixsu3(extW, U);
	}
	
	put_matrixsu3(gaugefield, U, pos, t, mu);
}

__kernel void overrelax_even(__global ocl_s_gaugefield* gaugefield, const hmc_float beta, const int mu, __global hmc_ocl_ran * rnd)
{
	int t, pos, id, id_tmp, size;
	id_tmp = get_global_id(0);
	size = get_global_size(0);
	for(id = id_tmp; id<VOLSPACE*NTIME/2; id+=size) {
		get_even_site(id, &pos, &t);
		perform_overrelaxing(gaugefield, beta, mu, rnd, pos, t, id_tmp);
	}
}

__kernel void overrelax_odd(__global ocl_s_gaugefield* gaugefield, const hmc_float beta, const int mu, __global hmc_ocl_ran * rnd)
{
	int t, pos, id, id_tmp, size;
	id_tmp = get_global_id(0);
	size = get_global_size(0);
	for(id = id_tmp; id<VOLSPACE*NTIME/2; id+=size) {
		get_odd_site(id, &pos, &t);
		perform_overrelaxing(gaugefield, beta, mu, rnd, pos, t, id_tmp);
	}
}

