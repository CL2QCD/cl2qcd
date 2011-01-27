#include "hmc.h"

int main(int argc, char* argv[]) {

  char* progname = argv[0];
  print_hello(progname);

  char* inputfile = argv[1];
  inputparameters parameters;
  parameters.readfile(inputfile);
  print_info(&parameters);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Initialization
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////


  stringstream gaugeout_name;
  gaugeout_name<<"gaugeobservables_beta"<<parameters.get_beta();
    
  sourcefileparameters parameters_source;
  hmc_gaugefield * gaugefield;
  gaugefield = (hmc_gaugefield*) malloc(sizeof(hmc_gaugefield));
  ildg_gaugefield * gaugefield_buf;
  gaugefield_buf = (ildg_gaugefield*) malloc(sizeof(ildg_gaugefield));
  int gaugefield_buf_size = sizeof(ildg_gaugefield)/sizeof(hmc_float);
  hmc_rndarray rndarray;

  init_gaugefield(gaugefield,&parameters,&inittime);
  
  //this needs optimization
  const size_t local_work_size  = VOL4D/2;
  const size_t global_work_size = local_work_size;
  //one should define a definite number of threads and use this here
  init_random_seeds(rnd, rndarray, VOL4D/2, &inittime);

  opencl gpu(CL_DEVICE_TYPE_GPU, &inittime);

  cout << "initial values of observables:\n\t" ;
  print_gaugeobservables(gaugefield, &polytime, &plaqtime);

  gpu.copy_gaugefield_to_device(gaugefield, &copytime);
  gpu.copy_rndarray_to_device(rndarray, &copytime);


  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Heatbath
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////


  //this has to go into a function later
  int nsteps = parameters.get_heatbathsteps();
  cout<<"perform "<<nsteps<<" heatbath steps on OpenCL device..."<<endl;
  for(int i = 0; i<nsteps; i++){
    gpu.run_heatbath(parameters.get_beta(), local_work_size, global_work_size, &updatetime);
    gpu.run_overrelax(parameters.get_beta(), local_work_size, global_work_size, &overrelaxtime);
    if( ( (i+1)%parameters.get_writefrequency() ) == 0 ) {
       gpu.gaugeobservables(local_work_size, global_work_size, &plaq, &tplaq, &splaq, &pol, &plaqtime, &polytime);
       print_gaugeobservables(plaq, tplaq, splaq, pol, i, gaugeout_name.str());
    }
  }

  gpu.get_gaugefield_from_device(gaugefield, &copytime);



  totaltime.add();
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Final Output
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////

  //these are not yet used...
  hmc_float c2_rec = 0, epsilonbar = 0, mubar = 0;
  //this is only in the makefile??
  string outputfile ("conf.");
  outputfile.append(parameters.sourcefilenumber);
  
  copy_gaugefield_to_ildg_format(gaugefield_buf, gaugefield);

  plaq = plaquette(gaugefield, &tplaq, &splaq);
  pol = polyakov(gaugefield);
  print_gaugeobservables(plaq, tplaq, splaq, pol, nsteps);
  
  write_gaugefield ( gaugefield_buf, gaugefield_buf_size , NSPACE, NSPACE, NSPACE, NSPACE, parameters.get_prec(), nsteps, plaq, parameters.get_beta(), parameters.get_kappa(), parameters.get_mu(), c2_rec, epsilonbar, mubar, version.c_str(), outputfile.c_str());

  time_output(&totaltime, &inittime, &polytime, &plaqtime, &updatetime, &overrelaxtime, &copytime);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // free variables
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  free(gaugefield);
  free(gaugefield_buf);
  gpu.finalize();
  
  return HMC_SUCCESS;
}
