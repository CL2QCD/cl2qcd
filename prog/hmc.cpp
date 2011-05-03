#include "hmc.h"

int main(int argc, char* argv[])
{
	char* progname = argv[0];
	print_hello(progname);

#ifdef _PERFORM_BENCHMARKS_
	if( argc != 3 ) {
		std::cerr << "Please specify the input file and the benchmark ID." << std::endl;
		return HMC_FILEERROR;
	}
#else /* _PERFORM_BENCHMARKS_ */
	if( argc != 2 ) {
		std::cerr << "Please specify one input file." << std::endl;
		return HMC_FILEERROR;
	}
#endif /* _PERFORM_BENCHMARKS_ */

	char* inputfile = argv[1];
	inputparameters parameters;
	parameters.readfile(inputfile);
	print_info(&parameters);

	stringstream gaugeout_name;
	gaugeout_name << "gaugeobservables_beta" << parameters.get_beta();

#ifdef _PERFORM_BENCHMARKS_
	benchmark_id = argv[2];
	int tmp = 0;
	//CP: this is done in order to have a time-file in any case
	totaltime.add();
	time_output(
	  &totaltime, &inittime, &polytime, &plaqtime, &updatetime, &overrelaxtime, &copytime
#ifdef _FERMIONS_
	  , &inittimer, &singletimer, &Mtimer, &copytimer, &scalarprodtimer, &latimer, &solvertimer, &dslashtimer, &Mdiagtimer
#endif /* _USE_FERMIONS_ */
	  , tmp
	);
	totaltime.reset();
#endif	/* _PERFORM_BENCHMARKS_ */

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Initialization
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	sourcefileparameters parameters_source;
	hmc_gaugefield * gaugefield;
	gaugefield = (hmc_gaugefield*) malloc(sizeof(hmc_gaugefield));
	hmc_rndarray rndarray;

	//init gaugefield according to the inputfile settings
	init_gaugefield(gaugefield, &parameters, &inittime);
	int err = init_random_seeds(rndarray, "rand_seeds", &inittime);
	if( err )
		return err;

	//TODO add a security function that ends the OpenCL-init if it takes too long (this is apparently necessary on the loewe)
#ifdef _USEGPU_
	opencl device(CL_DEVICE_TYPE_GPU, local_work_size, global_work_size, &inittime, &parameters);
#else /* _USEGPU_ */
	opencl device(CL_DEVICE_TYPE_CPU, local_work_size, global_work_size, &inittime, &parameters);
#endif /* _USEGPU_ */
	cout << endl << "OpenCL initialisaton time:\t" << inittime.getTime() << " [mus]" << endl;

	cout << "initial values of observables:\n\t" ;
	print_gaugeobservables(gaugefield, &polytime, &plaqtime);

	device.copy_gaugefield_to_device(gaugefield, &copytime);
	device.copy_rndarray_to_device(rndarray, &copytime);

#ifdef _TESTING_
	device.testing(gaugefield);
#endif /* _TESTING_ */

#ifndef _PERFORM_BENCHMARKS_

#ifdef _USEHMC_

	//TODO CP: port to OpenCL *g*
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Hybrid Monte Carlo
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//init section
	//CP: here, one has to get all the parameters necessary from the inputparameters
	// suppose that in parameters the number of hmc_iterations is given, store them in a variable here...
	int hmc_iter; //= ...
	int iter;
	int err = 0;
	//beta has to be saved here to give it to the metropolis step, all other parameters can be given via parameters
	hmc_float beta;
	// TODO give seed meaningful value, perhaps read it in from parameters
	int seed = 89343894543895;
	Random hmc_rnd_gen (seed);
	hmc_float rnd_number;

	hmc_spinor_field* phi = new hmc_spinor_field[SPINORFIELDSIZE];
	hmc_spinor_field* phi_inv = new hmc_spinor_field[SPINORFIELDSIZE];
	hmc_spinor_field* chi = new hmc_spinor_field[SPINORFIELDSIZE];
	hmc_gauge_momentum* p = new hmc_gauge_momentum[GAUGEMOMENTASIZE];
	hmc_gauge_momentum* new_p = new hmc_gauge_momentum[GAUGEMOMENTASIZE];
	//the "old" field is the "gaugefield introduced above
	hmc_gaugefield new_field;

	//TODO add implicit measurements(plaquette, deltaH, ...)
	//main hmc-loop
	for(iter = 0; iter < hmc_iter; iter ++) {
		//init gauge_momenta
		//TODO perhaps write a wrapper that automatically evaluates the err's
		err = generate_gaussian_gauge_momenta(p);

		//init/update spinorfield phi
		err = generate_gaussian_spinorfield(chi);
		err = md_update_spinorfield(chi, phi, &gaugefield, parameters);

		//update gaugefield and gauge_momenta via leapfrog
		//here, phi is inverted several times and stored in phi_inv
		err = leapfrog(parameters, &gaugefield, p, phi, &new_field, new_p, phi_inv);

		//metropolis step: afterwards, the updated config is again in gaugefield and p
		//generate new random-number
		rnd_number = hmc_rnd_gen.doub();
		err = metropolis(rnd_number, beta, phi, phi_inv, &gaugefield, p, &new_field, new_p);

		//TODO if wanted, measurements can be added here...
	}

	delete [] phi;
	delete [] phi_inv;
	delete [] chi;
	delete [] p;
	delete [] new_p;

#else /* _USEHMC_ */

#ifdef _FERMIONS_

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Fermions
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	cout<<"calculate simple_correlator on host..."<<endl;
	simple_correlator(gaugefield, parameters.get_kappa(), parameters.get_mu(), parameters.get_theta_fermion(), parameters.get_chem_pot_re(), parameters.get_chem_pot_im(), 1000);

	cout <<"calculate simple_correlator on device..." << endl;
	usetimer noop;
	device.init_fermion_variables(&parameters, local_work_size, global_work_size, &inittimer);
	device.simple_correlator_device(&copytimer, &singletimer, &Mtimer, &scalarprodtimer, &latimer, &solvertimer, &dslashtimer, &Mdiagtimer,  local_work_size, global_work_size, 1000);
	device.finalize_fermions();

#endif /* _FERMIONS_ */

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Heatbath
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	int nsteps = parameters.get_heatbathsteps();
	int overrelaxsteps = parameters.get_overrelaxsteps();
	cout<<"perform "<<nsteps<<" heatbath steps on OpenCL device..."<<endl;
	for(int i = 0; i<nsteps; i++) {
		device.run_heatbath(parameters.get_beta(), local_work_size, global_work_size, &updatetime);
		for (int j = 0; j < overrelaxsteps; j++)
			device.run_overrelax(parameters.get_beta(), local_work_size, global_work_size, &overrelaxtime);
		if( ( (i+1) % parameters.get_writefrequency() ) == 0 ) {
			device.gaugeobservables(local_work_size, global_work_size, &plaq, &tplaq, &splaq, &pol, &plaqtime, &polytime);
			print_gaugeobservables(plaq, tplaq, splaq, pol, i, gaugeout_name.str());
		}
		if( parameters.get_saveconfigs()==TRUE && ( (i+1) % parameters.get_savefrequency() ) == 0 ) {
			device.get_gaugefield_from_device(gaugefield, &copytime);
			save_gaugefield(gaugefield, &parameters, i);
			print_gaugeobservables(gaugefield, &plaqtime, &polytime, i, gaugeout_name.str());
		}
	}

	device.get_gaugefield_from_device(gaugefield, &copytime);

#endif /* _USEHMC_ */

#else /* _PERFORM_BENCHMARKS_ */

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Benchmarking
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _FERMIONS_

	int benchmarksteps1 = parameters.get_heatbathsteps();
	cout<<"perform HEATBATH-BENCHMARK with "<<benchmarksteps1<<" steps off each device operation..."<<endl;
	for(int i = 0; i<benchmarksteps1; i++) {
		device.run_heatbath(parameters.get_beta(), local_work_size, global_work_size, &updatetime);
		device.run_overrelax(parameters.get_beta(), local_work_size, global_work_size, &overrelaxtime);
		device.gaugeobservables(local_work_size, global_work_size, &plaq, &tplaq, &splaq, &pol, &plaqtime, &polytime);
		device.get_gaugefield_from_device(gaugefield, &copytime);
		if(i%100==0) {
			cout << "time at iteration " << i << endl;
			totaltime.add();
			time_output(&totaltime, &inittime, &polytime, &plaqtime, &updatetime, &overrelaxtime, &copytime, i);
			totaltime.reset();
		}
	}

#else /* _FERMIONS_ */

	int benchmarksteps2 = parameters.get_thermalizationsteps();
	int cgmax = parameters.get_cgmax();
	cout<<"perform FERMION-BENCHMARK with "<<benchmarksteps2<<" steps off each device operation..."<<endl;

	//CP: set up testing field
	hmc_spinor_field in[SPINORFIELDSIZE];
	if(!use_eo) {
		init_spinorfield_cold(in);
		device.copy_spinorfield_to_device(in, &copytimer);
	} else {
		//!!CP: this should be fine since only half the field is used but of course it is not nice...
		init_spinorfield_cold_eoprec(in);
		device.copy_eoprec_spinorfield_to_device(in, &copytimer);
	}
	for(int i = 0; i<benchmarksteps2; i++) {
		device.init_fermion_variables(&parameters, local_work_size, global_work_size, &inittimer);
		device.perform_benchmark(cgmax, local_work_size, global_work_size, &copytimer, &singletimer, &Mtimer, &scalarprodtimer, &latimer, &solvertimer, &dslashtimer, &Mdiagtimer);
		if(i%100==0) {
			cout << "time at iteration " << i << endl;
			totaltime.add();
			time_output( &totaltime, &inittime, &polytime, &plaqtime, &updatetime, &overrelaxtime, &copytime, &inittimer, &singletimer, &Mtimer, &copytimer, &scalarprodtimer, &latimer, &solvertimer, &dslashtimer, &Mdiagtimer, i );
			totaltime.reset();
		}
	}
	device.finalize_fermions();

#endif /* _FERMIONS_ */

#endif /* _PERFORM_BENCHMARKS_ */

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Final Output
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	totaltime.add();
#ifndef _PERFORM_BENCHMARKS_
	save_gaugefield(gaugefield, &parameters, nsteps);
#endif /* _PERFORM_BENCHMARKS_ */
	time_output(
	  &totaltime, &inittime, &polytime, &plaqtime, &updatetime, &overrelaxtime, &copytime
#ifdef _FERMIONS_
	  , &inittimer, &singletimer, &Mtimer, &copytimer, &scalarprodtimer, &latimer, &solvertimer, &dslashtimer, &Mdiagtimer
#endif /* _FERMIONS_ */
#ifdef _PERFORM_BENCHMARKS_
#ifndef _FERMIONS_
	  , benchmarksteps1
#else /* _FERMIONS_ */
	  , benchmarksteps2
#endif /* _FERMIONS_ */
#endif /* _PERFORM_BENCHMARKS_ */
	);
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// free variables
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	free(gaugefield);
	device.finalize();

	return HMC_SUCCESS;
}
