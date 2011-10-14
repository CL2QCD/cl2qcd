#include "../opencl_module_fermions.h"
#include "../gaugefield_hybrid.h"

// use the boost test framework
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Fermionmatrix
#include <boost/test/unit_test.hpp>

Random rnd(15);
extern std::string const version;
std::string const version = "0.1";

class Device : public Opencl_Module_Fermions {

	cl_kernel testKernel;

public:
	Device(cl_command_queue queue, inputparameters* params, int maxcomp, string double_ext) : Opencl_Module_Fermions() {
		Opencl_Module_Fermions::init(queue, 0, params, maxcomp, double_ext); /* init in body for proper this-pointer */
	};
	~Device() {
		finalize();
	};

	void runTestKernel(cl_mem in, cl_mem out, cl_mem gf, int gs, int ls);
	void fill_kernels();
	void clear_kernels();
};

class Dummyfield : public Gaugefield_hybrid {

public:
	Dummyfield(cl_device_type device_type) : Gaugefield_hybrid() {
		std::stringstream tmp;
		tmp << SOURCEDIR << "/tests/m_eo_gpu_input_1";
		params.readfile(tmp.str().c_str());

		init(1, device_type, &params);
	};

	virtual void init_tasks();
	virtual void finalize_opencl();

	void verify(int which);
	void runTestKernel();

private:
	void verify(hmc_complex, hmc_complex);
	void fill_buffers();
	void clear_buffers();
	inputparameters params;
	cl_mem in, out;
	cl_mem sqnorm;
	spinor * sf_in;
	spinor * sf_out;
	Matrixsu3 * gf_in;

};


BOOST_AUTO_TEST_CASE( CPU )
{
	logger.info() << "Init dummy device";
	//params.print_info_inverter("m_gpu");
	Dummyfield dummy(CL_DEVICE_TYPE_CPU);
	logger.info() << "gaugeobservables: ";
	dummy.print_gaugeobservables_from_task(0, 0);
	logger.info() << "|phi|^2:";
	dummy.verify(0);
	dummy.runTestKernel();
	logger.info() << "|M phi|^2:";
	dummy.verify(1);
	BOOST_MESSAGE("Tested CPU");
}

BOOST_AUTO_TEST_CASE( GPU )
{
	logger.info() << "Init dummy device";
	//params.print_info_inverter("m_gpu");
	Dummyfield dummy(CL_DEVICE_TYPE_GPU);
	logger.info() << "gaugeobservables: ";
	dummy.print_gaugeobservables_from_task(0, 0);
	logger.info() << "|phi|^2:";
	dummy.verify(0);
	dummy.runTestKernel();
	logger.info() << "|M phi|^2:";
	dummy.verify(1);
	BOOST_MESSAGE("Tested GPU");
}

void Dummyfield::init_tasks()
{
	opencl_modules = new Opencl_Module* [get_num_tasks()];
	opencl_modules[0] = new Device(queue[0], get_parameters(), get_max_compute_units(0), get_double_ext(0));

	fill_buffers();
}

void Dummyfield::finalize_opencl()
{
	clear_buffers();
	Gaugefield_hybrid::finalize_opencl();
}

void fill_sf_with_one(spinor * sf_in, int size)
{
	for(int i = 0; i < size; ++i) {
		sf_in[i].e0.e0 = hmc_complex_one;
		sf_in[i].e0.e1 = hmc_complex_one;
		sf_in[i].e0.e2 = hmc_complex_one;
		sf_in[i].e1.e0 = hmc_complex_one;
		sf_in[i].e1.e1 = hmc_complex_one;
		sf_in[i].e1.e2 = hmc_complex_one;
		sf_in[i].e2.e0 = hmc_complex_one;
		sf_in[i].e2.e1 = hmc_complex_one;
		sf_in[i].e2.e2 = hmc_complex_one;
		sf_in[i].e3.e0 = hmc_complex_one;
		sf_in[i].e3.e1 = hmc_complex_one;
		sf_in[i].e3.e2 = hmc_complex_one;
	}
	return;
}

void fill_sf_with_random(spinor * sf_in, int size)
{
	Random rnd_loc(123456);
	for(int i = 0; i < size; ++i) {
		sf_in[i].e0.e0.re = rnd_loc.doub();
		sf_in[i].e0.e1.re = rnd_loc.doub();
		sf_in[i].e0.e2.re = rnd_loc.doub();
		sf_in[i].e1.e0.re = rnd_loc.doub();
		sf_in[i].e1.e1.re = rnd_loc.doub();
		sf_in[i].e1.e2.re = rnd_loc.doub();
		sf_in[i].e2.e0.re = rnd_loc.doub();
		sf_in[i].e2.e1.re = rnd_loc.doub();
		sf_in[i].e2.e2.re = rnd_loc.doub();
		sf_in[i].e3.e0.re = rnd_loc.doub();
		sf_in[i].e3.e1.re = rnd_loc.doub();
		sf_in[i].e3.e2.re = rnd_loc.doub();

		sf_in[i].e0.e0.im = rnd_loc.doub();
		sf_in[i].e0.e1.im = rnd_loc.doub();
		sf_in[i].e0.e2.im = rnd_loc.doub();
		sf_in[i].e1.e0.im = rnd_loc.doub();
		sf_in[i].e1.e1.im = rnd_loc.doub();
		sf_in[i].e1.e2.im = rnd_loc.doub();
		sf_in[i].e2.e0.im = rnd_loc.doub();
		sf_in[i].e2.e1.im = rnd_loc.doub();
		sf_in[i].e2.e2.im = rnd_loc.doub();
		sf_in[i].e3.e0.im = rnd_loc.doub();
		sf_in[i].e3.e1.im = rnd_loc.doub();
		sf_in[i].e3.e2.im = rnd_loc.doub();
	}
	return;
}


void Dummyfield::fill_buffers()
{
	// don't invoke parent function as we don't require the original buffers

	cl_int err;

	cl_context context = opencl_modules[0]->get_context();

	int NUM_ELEMENTS_SF;
	if(get_parameters()->get_use_eo() == true) NUM_ELEMENTS_SF =  params.get_eoprec_spinorfieldsize();
	else NUM_ELEMENTS_SF =  params.get_spinorfieldsize();

	sf_in = new spinor[NUM_ELEMENTS_SF];
	sf_out = new spinor[NUM_ELEMENTS_SF];

	//use the variable use_cg to switch between cold and random input sf
	if(get_parameters()->get_use_cg() == true) fill_sf_with_one(sf_in, NUM_ELEMENTS_SF);
	else fill_sf_with_random(sf_in, NUM_ELEMENTS_SF);
	BOOST_REQUIRE(sf_in);

	size_t sf_buf_size = get_parameters()->get_sf_buf_size();
	//create buffer for sf on device (and copy sf_in to both for convenience)
	in = clCreateBuffer(context, CL_MEM_READ_ONLY , sf_buf_size, 0, &err );
	BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
	err = clEnqueueWriteBuffer(static_cast<Device*>(opencl_modules[0])->get_queue(), in, CL_TRUE, 0, sf_buf_size, sf_in, 0, 0, NULL);
	BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
	out = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sf_buf_size, 0, &err );
	BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
	err = clEnqueueWriteBuffer(static_cast<Device*>(opencl_modules[0])->get_queue(), out, CL_TRUE, 0, sf_buf_size, sf_in, 0, 0, NULL);
	BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

	//create buffer for squarenorm on device
	sqnorm = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(hmc_float), 0, &err);
}

void Device::fill_kernels()
{
	//one only needs some kernels up to now. to save time during compiling they are put in here by hand
	Opencl_Module::fill_kernels();

	//to this end, one has to set the needed files by hand
	basic_opencl_code = ClSourcePackage() << "opencl_header.cl" << "opencl_geometry.cl" << "opencl_operations_complex.cl"
	                    << "operations_matrix_su3.cl" << "operations_matrix.cl" << "operations_gaugefield.cl";
	basic_fermion_code = basic_opencl_code << "types_fermions.h" << "operations_su3vec.cl" << "operations_spinor.cl" << "spinorfield.cl";

	global_squarenorm = createKernel("global_squarenorm_eoprec") << basic_fermion_code << "spinorfield_eo_squarenorm.cl";
	global_squarenorm_reduction = createKernel("global_squarenorm_reduction") << basic_fermion_code << "spinorfield_squarenorm.cl";

/*	if(get_device_type() == CL_DEVICE_TYPE_GPU)
		//    testKernel = createKernel("M_tm_plus") << basic_fermion_code  << "fermionmatrix_GPU.cl";
		testKernel = createKernel("M_tm_plus")  << "tests/m_tm_plus_GPU_full.cl";
	if(get_device_type() == CL_DEVICE_TYPE_CPU)
		// testKernel = createKernel("M_tm_plus") << basic_fermion_code  << "fermionmatrix.cl" << "fermionmatrix_m_tm_plus.cl";
		testKernel = createKernel("M_tm_plus")  << "tests/m_tm_plus_GPU_full.cl";
	}*/
	testKernel = createKernel("dslash_eoprec") << basic_fermion_code << "operations_spinorfield_eo.cl" << "fermionmatrix.cl" << "fermionmatrix_eo.cl" << "fermionmatrix_eo_dslash.cl";
		
}

void Dummyfield::clear_buffers()
{
	// don't invoke parent function as we don't require the original buffers

	clReleaseMemObject(in);
	clReleaseMemObject(out);
	clReleaseMemObject(sqnorm);

	delete[] sf_in;
	delete[] sf_out;
}

void Device::clear_kernels()
{
	clReleaseKernel(testKernel);
	Opencl_Module::clear_kernels();
}

void Device::runTestKernel(cl_mem out, cl_mem in, cl_mem gf, int gs, int ls)
{
	cl_int err;
	err = clSetKernelArg(testKernel, 0, sizeof(cl_mem), &in);
	BOOST_REQUIRE_EQUAL(CL_SUCCESS, err);
	err = clSetKernelArg(testKernel, 1, sizeof(cl_mem), &out);
	BOOST_REQUIRE_EQUAL(CL_SUCCESS, err);
	err = clSetKernelArg(testKernel, 2, sizeof(cl_mem), &gf);
	BOOST_REQUIRE_EQUAL(CL_SUCCESS, err);
	int odd = 0;
	err = clSetKernelArg(testKernel, 3, sizeof(int), &odd);
	BOOST_REQUIRE_EQUAL(CL_SUCCESS, err);
	
	enqueueKernel(testKernel, gs, ls);
}

void Dummyfield::verify(int which)
{
	//which controlls if the in or out-vector is looked at
	if(which == 0) static_cast<Device*>(opencl_modules[0])->set_float_to_global_squarenorm_device(in, sqnorm);
	if(which == 1) static_cast<Device*>(opencl_modules[0])->set_float_to_global_squarenorm_device(out, sqnorm);
	// get stuff from device
	hmc_float result;
	cl_int err = clEnqueueReadBuffer(*queue, sqnorm, CL_TRUE, 0, sizeof(hmc_float), &result, 0, 0, 0);
	BOOST_REQUIRE_EQUAL(CL_SUCCESS, err);
	logger.info() << result;
}

void Dummyfield::runTestKernel()
{
	int gs, ls;
	if(opencl_modules[0]->get_device_type() == CL_DEVICE_TYPE_GPU) {
		gs = get_parameters()->get_eoprec_spinorfieldsize();
		ls = 64;
	} else if(opencl_modules[0]->get_device_type() == CL_DEVICE_TYPE_CPU) {
		gs = opencl_modules[0]->get_max_compute_units();
		ls = 1;
	}
	static_cast<Device*>(opencl_modules[0])->runTestKernel(out, in, *(get_clmem_gaugefield()), gs, ls);
}

