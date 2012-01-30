//alpha*x + beta*y + z
__kernel void saxsbypz_eoprec(__global const hmc_complex * const restrict x, __global const hmc_complex * const restrict y, __global const hmc_complex * const restrict z, __global const hmc_complex * const restrict alpha, __global hmc_complex * beta, __global hmc_complex * const restrict out)
{
	const int id = get_global_id(0);
	const int global_size = get_global_size(0);

	const hmc_complex alpha_tmp = complexLoadHack(alpha);
	const hmc_complex beta_tmp = complexLoadHack(beta);
	for(int id_tmp = id; id_tmp < EOPREC_SPINORFIELDSIZE; id_tmp += global_size) {
		spinor x_tmp = getSpinorSOA_eo(x, id_tmp);
		x_tmp = spinor_times_complex(x_tmp, alpha_tmp);
		spinor y_tmp = getSpinorSOA_eo(y, id_tmp);
		y_tmp = spinor_times_complex(y_tmp, beta_tmp);
		spinor z_tmp = getSpinorSOA_eo(z, id_tmp);

		spinor out_tmp = spinor_acc_acc(y_tmp, x_tmp, z_tmp);
		putSpinorSOA_eo(out, id_tmp, out_tmp);
	}

	return;
}
