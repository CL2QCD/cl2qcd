#include "gaugefield_inverter.h"

#include "meta/util.hpp"


hardware::code::Fermions* Gaugefield_inverter::get_task_solver()
{
	return (hardware::code::Fermions*)opencl_modules[task_solver];
}

hardware::code::Correlator* Gaugefield_inverter::get_task_correlator()
{
	return (hardware::code::Correlator*)opencl_modules[task_correlator];
}

void Gaugefield_inverter::init_tasks()
{
	//allocate host-memory for solution- and source-buffer
  size_t bufsize = get_parameters().get_num_sources() * meta::get_spinorfieldsize(get_parameters()) * sizeof(spinor);
	logger.debug() << "allocate memory for solution-buffer on host of size " << bufsize / 1024. / 1024. / 1024. << " GByte";
	solution_buffer = new spinor [get_parameters().get_num_sources() *  meta::get_spinorfieldsize(get_parameters())];
	source_buffer = new spinor [get_parameters().get_num_sources() * meta::get_spinorfieldsize(get_parameters())];

	task_solver = 0;
	task_correlator = 1;

	opencl_modules = new hardware::code::Opencl_Module* [get_num_tasks()];
	opencl_modules[task_solver] = get_device_for_task(task_solver)->get_fermion_code();
	opencl_modules[task_correlator] = get_device_for_task(task_correlator)->get_correlator_code();
}

void Gaugefield_inverter::delete_variables()
{
	Gaugefield_hybrid::delete_variables();
}

void Gaugefield_inverter::finalize_opencl()
{
	Gaugefield_hybrid::finalize_opencl();

	logger.debug() << "free solution buffer";
	delete [] solution_buffer;
	logger.debug() << "free source buffer";
	delete [] source_buffer;
}

void Gaugefield_inverter::invert_M_nf2_upperflavour(const hardware::buffers::Plain<spinor> * inout, const hardware::buffers::Plain<spinor> * source, const hardware::buffers::SU3 * gf, usetimer * solvertimer)
{
	/** This solves the sparse-matrix system
	 *  A x = b
	 *  with  x == inout
	 *        A == f
	 *        b == source
	 * using a Krylov-Solver (BiCGStab or CG)
	 */
	int converged = -1;
	hardware::code::Fermions * solver = get_task_solver();
	auto spinor_code = solver->get_device()->get_spinor_code();

	if(get_parameters().get_profile_solver() ) (*solvertimer).reset();

	if( !get_parameters().get_use_eo() ){
	  //noneo case
	  //Trial solution
	  ///@todo this should go into a more general function
	  spinor_code->set_spinorfield_cold_device(inout);
	  if(get_parameters().get_solver() == meta::Inputparameters::cg) {
	    //to use cg, one needs an hermitian matrix, which is QplusQminus
	    //the source must now be gamma5 b, to obtain the desired solution in the end
	    solver->gamma5_device(source);
		hardware::code::QplusQminus f_neo(solver);
	    converged = solver->cg(f_neo, inout, source, gf, get_parameters().get_solver_prec());
	    //now, calc Qminus inout to obtain x = A^⁻1 b
	    //therefore, use source as an intermediate buffer
	    solver->Qminus(inout, source, gf, get_parameters().get_kappa(), meta::get_mubar(get_parameters() ));
	    //save the result to inout
	    hardware::buffers::copyData(inout, source);
	  } else {
	    hardware::code::M f_neo(solver);
	    converged = solver->bicgstab(f_neo, inout, source, gf, get_parameters().get_solver_prec());
	  }
	}
	else{
	  /**
	   * If even-odd-preconditioning is used, the inversion is split up
	   * into even and odd parts using Schur decomposition, assigning the
	   * non-trivial inversion to the even sites (see DeGran/DeTar p 174ff).
	   */
	  //init some helping buffers
	  const hardware::buffers::Spinor clmem_source_even  (meta::get_eoprec_spinorfieldsize(get_parameters()), solver->get_device());
	  const hardware::buffers::Spinor clmem_source_odd  (meta::get_eoprec_spinorfieldsize(get_parameters()), solver->get_device());
	  const hardware::buffers::Spinor clmem_tmp_eo_1  (meta::get_eoprec_spinorfieldsize(get_parameters()), solver->get_device());
	  const hardware::buffers::Spinor clmem_tmp_eo_2  (meta::get_eoprec_spinorfieldsize(get_parameters()), solver->get_device());
	  const hardware::buffers::Plain<hmc_complex> clmem_one (1, solver->get_device());
	  hmc_complex one = hmc_complex_one;
	  clmem_one.load(&one);
	  
	  //convert source and input-vector to eoprec-format
	  spinor_code->convert_to_eoprec_device(&clmem_source_even, &clmem_source_odd, source);
	  //prepare sources
	  /**
	   * This changes the even source according to (with A = M + D):
	   *  b_e = b_e - D_eo M_inv b_o
	   */
	  
	  if(get_parameters().get_fermact() == meta::Inputparameters::wilson) {
	    //in this case, the diagonal matrix is just 1 and falls away.
	    solver->dslash_eo_device(&clmem_source_odd, &clmem_tmp_eo_1, gf, EVEN);
	    spinor_code->saxpy_eoprec_device(&clmem_source_even, &clmem_tmp_eo_1, &clmem_one, &clmem_source_even);
	  } else if(get_parameters().get_fermact() == meta::Inputparameters::twistedmass) {
	    solver->M_tm_inverse_sitediagonal_device(&clmem_source_odd, &clmem_tmp_eo_1);
	    solver->dslash_eo_device(&clmem_tmp_eo_1, &clmem_tmp_eo_2, gf, EVEN);
	    spinor_code->saxpy_eoprec_device(&clmem_source_even, &clmem_tmp_eo_2, &clmem_one, &clmem_source_even);
	  }
	  
	  //Trial solution
	  ///@todo this should go into a more general function
	  spinor_code->set_eoprec_spinorfield_cold_device(solver->get_inout_eo());
	  logger.debug() << "start eoprec-inversion";
	  //even solution
	  if(get_parameters().get_solver() == meta::Inputparameters::cg){
	    //to use cg, one needs an hermitian matrix, which is QplusQminus
	    //the source must now be gamma5 b, to obtain the desired solution in the end
	    solver->gamma5_eo_device(&clmem_source_even);
	    hardware::code::QplusQminus_eo f_eo(solver);
	    converged = solver->cg_eo(f_eo, solver->get_inout_eo(), &clmem_source_even, gf, get_parameters().get_solver_prec());
	    //now, calc Qminus inout to obtain x = A^⁻1 b
	    //therefore, use source as an intermediate buffer
	    solver->Qminus_eo(solver->get_inout_eo(), &clmem_source_even, gf, get_parameters().get_kappa(), meta::get_mubar(get_parameters() ));
	    //save the result to inout
	    hardware::buffers::copyData(solver->get_inout_eo(), &clmem_source_even);
	  } else{
	    hardware::code::Aee f_eo(solver);
	    converged = solver->bicgstab_eo(f_eo, solver->get_inout_eo(), &clmem_source_even, gf, get_parameters().get_solver_prec());
	  }
	  
	  //odd solution
	  /** The odd solution is obtained from the even one according to:
	   *  x_o = M_inv D x_e - M_inv b_o
	   */
	  if(get_parameters().get_fermact() == meta::Inputparameters::wilson) {
	    //in this case, the diagonal matrix is just 1 and falls away.
	    solver->dslash_eo_device(solver->get_inout_eo(), &clmem_tmp_eo_1, gf, ODD);
	    spinor_code->saxpy_eoprec_device(&clmem_tmp_eo_1, &clmem_source_odd, &clmem_one, &clmem_tmp_eo_1);
	  } else if(get_parameters().get_fermact() == meta::Inputparameters::twistedmass) {
	    solver->dslash_eo_device(solver->get_inout_eo(), &clmem_tmp_eo_2, gf, ODD);
	    solver->M_tm_inverse_sitediagonal_device(&clmem_tmp_eo_2, &clmem_tmp_eo_1);
	    solver->M_tm_inverse_sitediagonal_device(&clmem_source_odd, &clmem_tmp_eo_2);
	    spinor_code->saxpy_eoprec_device(&clmem_tmp_eo_1, &clmem_tmp_eo_2, &clmem_one, &clmem_tmp_eo_1);
	  }
	  //CP: whole solution
	  //CP: suppose the even sol is saved in inout_eoprec, the odd one in clmem_tmp_eo_1
	  spinor_code->convert_from_eoprec_device(solver->get_inout_eo(), &clmem_tmp_eo_1, inout);
	}

	if(get_parameters().get_profile_solver() ) {
		solver->get_device()->synchronize();
		(*solvertimer).add();
	}

	if (converged < 0) {
		if(converged == -1) logger.fatal() << "\t\t\tsolver did not solve!!";
		else logger.fatal() << "\t\t\tsolver got stuck after " << abs(converged) << " iterations!!";
	} else logger.debug() << "\t\t\tsolver solved in " << converged << " iterations!";
}

void Gaugefield_inverter::perform_inversion(usetimer* solver_timer)
{
  int num_sources = get_parameters().get_num_sources();

	hardware::code::Fermions * solver = get_task_solver();
	const hardware::buffers::Plain<spinor> clmem_res(meta::get_spinorfieldsize(get_parameters()), solver->get_device());
	const hardware::buffers::Plain<spinor> clmem_source(meta::get_spinorfieldsize(get_parameters()), solver->get_device());
	auto gf_code = solver->get_device()->get_gaugefield_code();

	//apply stout smearing if wanted
	if(get_parameters().get_use_smearing() == true)
		gf_code->smear_gaugefield(gf_code->get_gaugefield(), std::vector<const hardware::buffers::SU3 *>());

	for(int k = 0; k < num_sources; k++) {
		//copy source from to device
		//NOTE: this is a blocking call!
		logger.debug() << "copy pointsource between devices";
		clmem_source.load(&source_buffer[k * meta::get_vol4d(get_parameters())]);
		logger.debug() << "calling solver..";
		invert_M_nf2_upperflavour( &clmem_res, &clmem_source, gf_code->get_gaugefield(), solver_timer);
		//add solution to solution-buffer
		//NOTE: this is a blocking call!
		logger.debug() << "add solution...";
		clmem_res.dump(&solution_buffer[k * meta::get_vol4d(get_parameters())]);
	}

	if(get_parameters().get_use_smearing() == true) 
		gf_code->unsmear_gaugefield(gf_code->get_gaugefield());
}

void Gaugefield_inverter::flavour_doublet_correlators(std::string corr_fn)
{
	using namespace std;
	using namespace hardware::buffers;

	logger.debug() << "init buffers for correlator calculation...";
	const hardware::buffers::Plain<spinor> clmem_corr(get_parameters().get_num_sources() * meta::get_spinorfieldsize(get_parameters()), get_task_correlator()->get_device());
	//for now, make sure clmem_corr is properly filled; maybe later we can increase performance a bit by playing with this...
	clmem_corr.load(solution_buffer);

	//this buffer is only needed if sources other than pointsources have been used.
	hardware::buffers::Plain<spinor> * clmem_sources;
	if(get_parameters().get_sourcetype() != meta::Inputparameters::point ){
	  clmem_sources = new hardware::buffers::Plain<spinor>(get_parameters().get_num_sources() * meta::get_vol4d(get_parameters()), get_task_correlator()->get_device());
	  clmem_sources->load(source_buffer);
	}

	int num_corr_entries =  0;
	switch (get_parameters().get_corr_dir()) {
		case 0 :
			num_corr_entries = get_parameters().get_ntime();
			break;
		case 3 :
			num_corr_entries = get_parameters().get_nspace();
			break;
		default :
			stringstream errmsg;
			errmsg << "Correlator direction " << get_parameters().get_corr_dir() << " has not been implemented.";
			throw Print_Error_Message(errmsg.str());
	}

	const Plain<hmc_float> result_ps(num_corr_entries, get_task_correlator()->get_device());
	const Plain<hmc_float> result_sc(num_corr_entries, get_task_correlator()->get_device());
	const Plain<hmc_float> result_vx(num_corr_entries, get_task_correlator()->get_device());
	const Plain<hmc_float> result_vy(num_corr_entries, get_task_correlator()->get_device());
	const Plain<hmc_float> result_vz(num_corr_entries, get_task_correlator()->get_device());
	const Plain<hmc_float> result_ax(num_corr_entries, get_task_correlator()->get_device());
	const Plain<hmc_float> result_ay(num_corr_entries, get_task_correlator()->get_device());
	const Plain<hmc_float> result_az(num_corr_entries, get_task_correlator()->get_device());

	logger.info() << "calculate correlators..." ;
	if(get_parameters().get_sourcetype() == meta::Inputparameters::point){
	  get_task_correlator()->correlator_device(get_task_correlator()->get_correlator_kernel("ps"), &clmem_corr, &result_ps);
	  get_task_correlator()->correlator_device(get_task_correlator()->get_correlator_kernel("sc"), &clmem_corr, &result_sc);
	  get_task_correlator()->correlator_device(get_task_correlator()->get_correlator_kernel("vx"), &clmem_corr, &result_vx);
	  get_task_correlator()->correlator_device(get_task_correlator()->get_correlator_kernel("vy"), &clmem_corr, &result_vy);
	  get_task_correlator()->correlator_device(get_task_correlator()->get_correlator_kernel("vz"), &clmem_corr, &result_vz);
	  get_task_correlator()->correlator_device(get_task_correlator()->get_correlator_kernel("ax"), &clmem_corr, &result_ax);
	  get_task_correlator()->correlator_device(get_task_correlator()->get_correlator_kernel("ay"), &clmem_corr, &result_ay);
	  get_task_correlator()->correlator_device(get_task_correlator()->get_correlator_kernel("az"), &clmem_corr, &result_az);
	} else {
	  get_task_correlator()->correlator_device(get_task_correlator()->get_correlator_kernel("ps"), &clmem_corr, clmem_sources, &result_ps);
	  get_task_correlator()->correlator_device(get_task_correlator()->get_correlator_kernel("sc"), &clmem_corr, clmem_sources, &result_sc);
	  get_task_correlator()->correlator_device(get_task_correlator()->get_correlator_kernel("vx"), &clmem_corr, clmem_sources, &result_vx);
	  get_task_correlator()->correlator_device(get_task_correlator()->get_correlator_kernel("vy"), &clmem_corr, clmem_sources, &result_vy);
	  get_task_correlator()->correlator_device(get_task_correlator()->get_correlator_kernel("vz"), &clmem_corr, clmem_sources, &result_vz);
	  get_task_correlator()->correlator_device(get_task_correlator()->get_correlator_kernel("ax"), &clmem_corr, clmem_sources, &result_ax);
	  get_task_correlator()->correlator_device(get_task_correlator()->get_correlator_kernel("ay"), &clmem_corr, clmem_sources, &result_ay);
	  get_task_correlator()->correlator_device(get_task_correlator()->get_correlator_kernel("az"), &clmem_corr, clmem_sources, &result_az);
	}

	hmc_float* host_result_ps = new hmc_float [num_corr_entries];
	hmc_float* host_result_sc = new hmc_float [num_corr_entries];
	hmc_float* host_result_vx = new hmc_float [num_corr_entries];
	hmc_float* host_result_vy = new hmc_float [num_corr_entries];
	hmc_float* host_result_vz = new hmc_float [num_corr_entries];
	hmc_float* host_result_ax = new hmc_float [num_corr_entries];
	hmc_float* host_result_ay = new hmc_float [num_corr_entries];
	hmc_float* host_result_az = new hmc_float [num_corr_entries];

	if(get_parameters().get_print_to_screen() )
	  meta::print_info_flavour_doublet_correlators(get_parameters());

	ofstream of;
	of.open(corr_fn.c_str(), ios_base::app);
	if(of.is_open()) {
	  meta::print_info_flavour_doublet_correlators(&of, get_parameters());
	} else {
	  throw File_Exception(corr_fn);
	}

	// @todo One could also implement to write all results on screen if wanted
	//the pseudo-scalar (J=0, P=1)
	logger.info() << "pseudo scalar correlator:" ;
	result_ps.dump(host_result_ps);
	for(int j = 0; j < num_corr_entries; j++) {
		logger.info() << j << "\t" << scientific << setprecision(14) << host_result_ps[j];
		of << scientific << setprecision(14) << "0 1\t" << j << "\t" << host_result_ps[j] << endl;
	}

	//the scalar (J=0, P=0)
	result_sc.dump(host_result_sc);
	for(int j = 0; j < num_corr_entries; j++) {
		of << scientific << setprecision(14) << "0 0\t" << j << "\t" << host_result_sc[j] << endl;
	}

	//the vector (J=1, P=1)
	result_vx.dump(host_result_vx);
	result_vy.dump(host_result_vy);
	result_vz.dump(host_result_vz);
	for(int j = 0; j < num_corr_entries; j++) {
		of << scientific << setprecision(14) << "1 1\t" << j << "\t" << (host_result_vx[j] + host_result_vy[j] + host_result_vz[j]) / 3. << "\t" << host_result_vx[j] << "\t" << host_result_vy[j] << "\t" << host_result_vz[j] << endl;
	}

	//the axial vector (J=1, P=0)
	result_ax.dump(host_result_ax);
	result_ay.dump(host_result_ay);
	result_az.dump(host_result_az);
	for(int j = 0; j < num_corr_entries; j++) {
		of << scientific << setprecision(14) << "1 0\t" << j << "\t" << (host_result_ax[j] + host_result_ay[j] + host_result_az[j]) / 3. << "\t" << host_result_ax[j] << "\t" << host_result_ay[j] << "\t" << host_result_az[j] << endl;
	}

	of << endl;
	of.close();
	delete [] host_result_ps;
	delete [] host_result_sc;
	delete [] host_result_vx;
	delete [] host_result_vy;
	delete [] host_result_vz;
	delete [] host_result_ax;
	delete [] host_result_ay;
	delete [] host_result_az;
}

void Gaugefield_inverter::create_sources()
{
  //create sources on the correlator-device and save them on the host
  const hardware::buffers::Plain<spinor> clmem_source(meta::get_spinorfieldsize(get_parameters()), get_task_correlator()->get_device());
  auto prng = &get_device_for_task(task_correlator)->get_prng_code()->get_prng_buffer();

  for(int k = 0; k < get_parameters().get_num_sources(); k++) {
    if(get_parameters().get_sourcetype() == meta::Inputparameters::sourcetypes::point) {
      logger.debug() << "start creating point-source..."; 
      //CP: k has to be between 0..12 (corresponds to spin-color index)
      int k_tmp = k%12;
      get_task_correlator()->create_point_source_device(&clmem_source, k_tmp, get_source_pos_spatial(get_parameters()), get_parameters().get_source_t());
    } else if (get_parameters().get_sourcetype() == meta::Inputparameters::sourcetypes::volume) {
      logger.debug() << "start creating volume-source...";
      get_task_correlator()->create_volume_source_device(&clmem_source, prng);
    }  else if (get_parameters().get_sourcetype() == meta::Inputparameters::sourcetypes::timeslice) {
      logger.debug() << "start creating timeslice-source...";
      get_task_correlator()->create_timeslice_source_device(&clmem_source, prng, get_parameters().get_source_t());
    }  else if (get_parameters().get_sourcetype() == meta::Inputparameters::sourcetypes::zslice) {
      logger.debug() << "start creating zslice-source...";
      get_task_correlator()->create_zslice_source_device(&clmem_source, prng, get_parameters().get_source_z());
    }
    logger.debug() << "copy source to host";
    clmem_source.dump(&source_buffer[k * meta::get_vol4d(get_parameters())]);
  }
}

void Gaugefield_inverter::flavour_doublet_chiral_condensate(std::string pbp_fn){
	using namespace std;
	using namespace hardware::buffers;

	hmc_complex host_result = {0., 0.};
	if(get_parameters().get_pbp_version() == meta::Inputparameters::std){
	  /**
	   * In the pure Wilson case one can evaluate <pbp> with stochastic estimators according to:
	   * <pbp> = <ubu + dbd> = 2<ubu> = 2 Tr_(space, colour, dirac) ( D^-1 )
	   auto spinor_code = solver->get_device()->get_spinor_code(); *       = lim_r->inf 2/r (Xi_r, Phi_r)
	   * where the estimators satisfy
	   * D^-1(x,y)_(a,b, A,B) = lim_r->inf Phi_r(x)_a,A (Xi_r(y)_b,B)^dagger
	   * and Phi fulfills
	   * D Phi = Xi
	   * (X,Y) denotes the normal scalar product
	   * In the twisted-mass case one can evaluate <pbp> with stochastic estimators similar to the pure wilson case.
	   * However, one first has to switch to the twisted basis:
	   * <pbp> -> <chibar i gamma_5 tau_3 chi>
	   *       = <ub i gamma_5 u> - <db i gamma_5 d>
	   *       = Tr( i gamma_5 (D^-1_u - D^-1_d ) )
	   *       = Tr( i gamma_5 (D^-1_u - gamma_5 D^-1_u^dagger gamma_5) )
	   *       = Tr( i gamma_5 (D^-1_u -  D^-1_u^dagger ) )
	   *       = 2 Im Tr ( gamma_5 D^-1_u)
	   *       = lim_r->inf 2/r  (gamma_5 Xi_r, Phi_r)
	   * NOTE: The basic difference compared to the pure Wilson case is only the gamma_5 and that one takes the imaginary part!
	   */
	  auto spinor_code = get_task_solver()->get_device()->get_spinor_code();
	  auto fermion_code = get_task_solver()->get_device()->get_fermion_code();
	  // Need 2 spinors at once..
	  logger.debug() << "init buffers for chiral condensate calculation...";
	  const hardware::buffers::Plain<spinor> clmem_phi(meta::get_spinorfieldsize(get_parameters()), get_task_correlator()->get_device());
	  const hardware::buffers::Plain<spinor> clmem_xi(meta::get_spinorfieldsize(get_parameters()), get_task_correlator()->get_device());
	  const Plain<hmc_complex> result_pbp2(1, get_task_correlator()->get_device());
	  for(int i = 0; i < get_parameters().get_num_sources(); i++){
	    clmem_phi.load(&solution_buffer[i* meta::get_spinorfieldsize(get_parameters())]);
	    clmem_xi.load(&source_buffer[i* meta::get_spinorfieldsize(get_parameters())]);
	    if(get_parameters().get_fermact() == meta::Inputparameters::twistedmass){
	      fermion_code->gamma5_device(&clmem_xi);
	    }
	    spinor_code->set_complex_to_scalar_product_device(&clmem_xi, &clmem_phi, &result_pbp2);
	    hmc_complex host_tmp;
	    result_pbp2.dump(&host_tmp);
	    if(get_parameters().get_fermact() == meta::Inputparameters::wilson){
	      host_result.re += host_tmp.re;
              host_result.im += host_tmp.im;
	    }
	    else if(get_parameters().get_fermact() == meta::Inputparameters::twistedmass){
	      //NOTE: Here I just swap the assignments
	      host_result.re += host_tmp.im;
	      host_result.im += host_tmp.re;
	    }
	  }
	  //Normalization: The 2/r from above plus 1/VOL4D
	  hmc_float norm = 2./get_parameters().get_num_sources() / meta::get_vol4d(get_parameters());
	  host_result.re *= norm;
	  host_result.im *= norm;
	}
	if(get_parameters().get_fermact() == meta::Inputparameters::twistedmass && get_parameters().get_pbp_version() == meta::Inputparameters::tm_one_end_trick ){
	  /**
	   * For twisted-mass fermions one can also employ the one-end trick, which origins from
	   * D_d - D_u = - 4 i kappa amu gamma_5 <-> D^-1_u - D^-1_d = - 4 i kappa amu gamma_5 (D^-1_u)^dagger D^-1_u
	   * With this, the chiral condensate is:
	   * <pbp> = ... = Tr( i gamma_5 (D^-1_u - D^-1_d ) )
	   *       = - 4 kappa amu lim_r->inf 1/R (Phi_r, Phi_r)
	   * NOTE: Here one only needs Phi...
	   */
	  auto spinor_code = get_task_solver()->get_device()->get_spinor_code();
	  logger.debug() << "init buffers for chiral condensate calculation...";
	  const hardware::buffers::Plain<spinor> clmem_phi(meta::get_spinorfieldsize(get_parameters()), get_task_correlator()->get_device());
	  const Plain<hmc_float> result_pbp2(1, get_task_correlator()->get_device());
	  for(int i = 0; i < get_parameters().get_num_sources(); i++){
	    clmem_phi.load(&solution_buffer[i* meta::get_spinorfieldsize(get_parameters())]);
	    spinor_code->set_float_to_global_squarenorm_device(&clmem_phi, &result_pbp2);
	    hmc_float host_tmp;
	    result_pbp2.dump(&host_tmp);
	    host_result.re += host_tmp;
	    host_result.im += 0;
	  }
	  //Normalization: The 1/r from above plus 1/VOL4D and the additional factor - 4 kappa amu ( = 2 mubar )
	  hmc_float norm = 1./get_parameters().get_num_sources() / meta::get_vol4d(get_parameters()) * (-2.) * meta::get_mubar(get_parameters() );
	  host_result.re *= norm;
	  host_result.im *= norm;

	}
	ofstream of;
	of.open(pbp_fn.c_str(), ios_base::app);
	if(!of.is_open()) {
	  throw File_Exception(pbp_fn);
	}

	// @todo One could also implement to write all results on screen if wanted
	//the pseudo-scalar (J=0, P=1)
	logger.info() << "chiral condensate:" ;
	logger.info() << scientific << setprecision(14) << host_result.re << "\t" << host_result.im;
	of << scientific << setprecision(14) << host_result.re << "\t" << host_result.im << endl;


}

