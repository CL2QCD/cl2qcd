/** @file
 * Implementation of the algorithm to find min and max eigenvalues of an operator
 * 
 * (c) 2013 Alessandro Sciarra <sciarra@th.physik.uni-frankfurt.de>
 */

#include "find_minmax_eigenvalue.hpp"
#include "../lattices/staggeredfield_eo.hpp"
#include "../lattices/util.hpp"
#include "../lattices/scalar.hpp"
#include "../../logger.hpp"
#include "cmath"
#include "sstream"
#include "solver.hpp" //For exceptions


static std::string create_log_prefix_find_max(int number) noexcept;

hmc_float physics::algorithms::find_max_eigenvalue(const physics::fermionmatrix::Fermionmatrix_stagg_eo& A, const physics::lattices::Gaugefield& gf, const hardware::System& system, hmc_float prec)
{
	using namespace physics::lattices;
	using namespace physics::algorithms;
	
	if(!(A.is_hermitian()))
		throw std::invalid_argument("Unable to deal with non-hermitian matrices in find_max_eigenvalue!");
		
	Scalar<hmc_complex> max(system);
	hmc_float resid;
	
	//This field is the starting point and it must be random (we have to be sure
	//to have a non zero component along the eigenvectors referring to the biggest eigenvalue)
	Staggeredfield_eo v1(system);
	pseudo_randomize<Staggeredfield_eo, su3vec>(&v1, 123);
	sax(&v1, {1./sqrt(squarenorm(v1)), 0.}, v1); //v1 is now normalized
	//Auxiliary field
	Staggeredfield_eo v2(system);
	//How often to check resid
	auto params = system.get_inputparameters();
	const int RESID_CHECK_FREQUENCY = params.get_find_minmax_iteration_block_size();
	
	log_squarenorm(create_log_prefix_find_max(0) + "v1 (initial): ", v1);
	log_squarenorm(create_log_prefix_find_max(0) + "v2 (initial) [not-initialized]: ", v2);
	
	for(int i=0; i < params.get_findminmax_max(); i++){
		//Apply A onto v1
		A(&v2, gf, v1);
		if(i%100 == 0) log_squarenorm(create_log_prefix_find_max(i) + "v2: ", v2);
		//Normalize v2
		sax(&v2, {1./sqrt(squarenorm(v2)), 0.}, v2);
		if(i%100 == 0) log_squarenorm(create_log_prefix_find_max(i) + "v2: ", v2);
		//Check whether the algorithm converged
		if(i % RESID_CHECK_FREQUENCY == 0) {
			saxpy(&v1, {-1.,0.}, v1, v2); //Here I can use v1 as container, the important is not
			                              //to modify v2 that will be copied in v1
			log_squarenorm(create_log_prefix_find_max(i) + "v1: ", v1);
			resid = sqrt(squarenorm(v1));
			
			logger.debug() << create_log_prefix_find_max(i) << "resid: " << std::setprecision(8)<< resid;
			
			if(resid < prec){
				A(&v1, gf, v2);
				scalar_product(&max, v2, v1);
				hmc_complex result = max.get();
				logger.debug() << "max.im = " << result.im;
				if((result.im) > 1.e-12){
					logger.fatal() << "Power Method found complex eigenvalue!";
					throw solvers::SolverStuck(i, __FILE__, __LINE__);
				}
				logger.debug() << "Find max_eig converged in " << i << " iterations! resid = " << resid;
				return result.re;
			}
		}
		copyData(&v1, v2);
	}
	
	logger.fatal() << "Power Method failed in finding max_eig in " << params.get_findminmax_max() << " iterations. Last resid: " << resid;
	throw solvers::SolverDidNotSolve(params.get_findminmax_max(), __FILE__, __LINE__);
	
	
}


hmc_float physics::algorithms::find_min_eigenvalue(const physics::fermionmatrix::Fermionmatrix_stagg_eo& A, const physics::lattices::Gaugefield& gf, const hardware::System& system, hmc_float prec, const bool conservative)
{
	if(conservative){
		hmc_float mass = A.get_mass();
		return mass*mass;
	}
	
	using namespace physics::lattices;
	using namespace physics::algorithms;
	
	if(!(A.is_hermitian()))
		throw std::invalid_argument("Unable to deal with non-hermitian matrices in find_max_eigenvalue!");
	
	Scalar<hmc_complex> min(system);
	hmc_float max  = find_max_eigenvalue(A, gf, system, prec);
	hmc_float resid;
	logger.info() << "max = " << std::setprecision(16) << max;
	
	//This field is the starting point and it must be random (we have to be sure
	//to have a non zero component along the eigenvectors referring to the smallest eigenvalue)
	Staggeredfield_eo v1(system);
	pseudo_randomize<Staggeredfield_eo, su3vec>(&v1, 321);
	log_squarenorm(create_log_prefix_find_max(0) + "v1 (initial): ", v1);
	sax(&v1, {1./sqrt(squarenorm(v1)), 0.}, v1); //v1 is now normalized
	//Auxiliary field
	Staggeredfield_eo v2(system);
	//How often to check resid
	auto params = system.get_inputparameters();
	const int RESID_CHECK_FREQUENCY = params.get_find_minmax_iteration_block_size();
	
	log_squarenorm(create_log_prefix_find_max(0) + "v1 (initial): ", v1);
	log_squarenorm(create_log_prefix_find_max(0) + "v2 (initial) [not-initialized]: ", v2);
	
	for(int i=0; i < params.get_findminmax_max(); i++){
		//Apply (max-A) onto v1
		A(&v2, gf, v1);
		saxpby(&v2, {max,0.}, v1, {-1.,0.}, v2); //Now in v2 there is (max-A)*v1
		if(i%100 == 0) log_squarenorm(create_log_prefix_find_max(i) + "v2: ", v2);
		//Normalize v2
		sax(&v2, {1./sqrt(squarenorm(v2)), 0.}, v2);
		if(i%100 == 0) log_squarenorm(create_log_prefix_find_max(i) + "v2: ", v2);
		//Check whether the algorithm converged
		if(i % RESID_CHECK_FREQUENCY == 0) {
			saxpy(&v1, {-1.,0.}, v1, v2);//Here I can use v1 as container, the important is not
			                              //to modify v2 that will be copied in v1
			log_squarenorm(create_log_prefix_find_max(i) + "v1: ", v1);
			resid = sqrt(squarenorm(v1));
			
			logger.debug() << create_log_prefix_find_max(i) << "resid: " << std::setprecision(8)<< resid;
			
			if(resid < prec){
				//Apply (max-A) onto v2
				A(&v1, gf, v2);
				saxpby(&v1, {max,0.}, v2, {-1.,0.}, v1); //Now in v1 there is (max-A)*v2
				scalar_product(&min, v2, v1);
				hmc_complex result = min.get();
				logger.debug() << "min.im = " << result.im;
				if(abs(result.im) > 1.e-12){
					logger.fatal() << "Power Method found complex eigenvalue!";
					throw solvers::SolverStuck(i, __FILE__, __LINE__);
				}
				logger.debug() << "Find min_eig converged in " << i << " iterations! resid = " << resid;
				return max-result.re;
			}
		}
		copyData(&v1, v2);
	}
	
	logger.fatal() << "Power Method failed in finding min_eig in " << params.get_findminmax_max() << " iterations. Last resid: " << resid;
	throw solvers::SolverDidNotSolve(params.get_findminmax_max(), __FILE__, __LINE__);
	
	
}






static std::string create_log_prefix_find(std::string name, int number) noexcept
{
  using namespace std;
  string separator_big = "\t";
  string separator_small = " ";
  string label = "FIND";

  stringstream strnumber;
  strnumber.fill('0');
  /// @todo this should be length(cgmax)
  strnumber.width(6);
  strnumber << right << number;
  stringstream outfilename;
  outfilename << separator_big << label << separator_small << "[" << name << "]" << separator_small << "[" << strnumber.str() << "]:" << separator_big;
  string outputfile = outfilename.str();
  return outputfile;
}

static std::string create_log_prefix_find_max(int number) noexcept
{
  return create_log_prefix_find("MAX", number);
}




