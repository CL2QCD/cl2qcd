//alpha*x + beta*y + z
__kernel void saxsbypz(__global const spinor * const restrict x, __global const spinor * const restrict y, __global const spinor * const restrict z, __global const hmc_complex * const restrict alpha, __global const hmc_complex * const restrict beta, __global spinor * const restrict out)
{
	int id = get_global_id(0);
	int global_size = get_global_size(0);

	hmc_complex alpha_tmp = complexLoadHack(alpha);
	hmc_complex beta_tmp = complexLoadHack(beta);
	for(int id_mem = id; id_mem < SPINORFIELDSIZE_MEM; id_mem += global_size) {
		spinor x_tmp = x[id_mem];
		x_tmp = spinor_times_complex(x_tmp, alpha_tmp);
		spinor y_tmp = y[id_mem];
		y_tmp = spinor_times_complex(y_tmp, beta_tmp);
		spinor z_tmp = z[id_mem];

		out[id_mem] = spinor_acc_acc(y_tmp, x_tmp, z_tmp);
	}

	return;
}
