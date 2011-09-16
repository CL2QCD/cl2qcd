/** @file
 * Basic OpenCL functionality
 */
#ifndef _OPENCLMODULEH_
#define _OPENCLMODULEH_

#include <cmath>
#include <cstdlib>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include "host_geometry.h"
#include "host_operations_complex.h"
#include "host_operations_gaugefield.h"
#include "globaldefs.h"
#include "types.h"
#include "host_use_timer.h"
#include "inputparameters.h"
#include "opencl_compiler.hpp"

#include "exceptions.h"

/**
 * An OpenCL device
 *
 * This class wraps all operations on a device. Operations are always specific, e.g. each kernel and copy operation
 * has it's own wrapper function.
 *
 * @todo Everything is public to faciliate inheritance. Actually, more parts should be private.
 */
class Opencl_Module {
public:
	/**
	 * Empty constructor.
	 *
	 */
	Opencl_Module() {};
	/**
	 * Destructor, calls finalize().
	 *
	 */
	~Opencl_Module() {
		finalize();
	}

	/**
	 * Free variables. Called by destructor.
	 */
	void finalize();

	/**
	 * Initialize everything. First method to be called.
	 *
	 * @param[in] queue OpenCL command queue
	 * @param[in] clmem_gaugefield OpenCL buffer that contains the gaugefield
	 * @param[in] params instance of inputparameters
	 * @param[in] maxcomp maximum_compute_units for device
	 * @param[in] double_ext OpenCL double extension for device (AMD or KHR)
	 */
	void init(cl_command_queue queue, cl_mem* clmem_gaugefield, inputparameters* params, int maxcomp, string double_ext);

	// set and get methods
	/**
	 * Get the context
	 * @return cl_context
	 */
	cl_context get_context();
	/**
	 * Set queue
	 * @param[in] queue OpenCL command queue
	 */
	void set_queue(cl_command_queue queue);
	/**
	 * Return the OpenCL command queue
	 * @return ocl_queue
	 */
	cl_command_queue get_queue();
	/**
	 * Set the gaugefield buffer
	 * @param[in] clmem_gaugefield pointer to OpenCL buffer
	 */
	void set_gaugefield(cl_mem* clmem_gaugefield);
	/**
	 * Get a pointer to the gaugefield buffer
	 * @return ocl_gaugefield OpenCL buffer with gaugefield
	 */
	cl_mem* get_gaugefield();
	/**
	 * Set inputparameters
	 * @param params Pointer to inputparameters
	 */
	void set_parameters(inputparameters* params);
	/**
	 * Get a pointer to inputparameters
	 * @return parameters
	 */
	inputparameters* get_parameters();
	/**
	 * Get OpenCL device
	 * @return device
	 */
	cl_device_id get_device();
	/**
	 * Get OpenCL device_type
	 * @return device_type
	 */
	cl_device_type get_device_type();
	/**
	 * Get platform_id
	 * @return platform
	 */
	cl_platform_id get_platform();
	/**
	 * Set device_double_extension
	 * @param double_ext "AMD" or "KHR"
	 */
	void set_device_double_extension(string double_ext);
	/**
	 * Get the device_double_extension
	 * @return double_extension
	 */
	string get_device_double_extension();
	/**
	 * Get the maximum_compute_units
	 * @return max_compute_units
	 */
	int get_max_compute_units();
	/**
	 * Set the maximum_compute_units
	 * @return max_compute_units
	 */
	void set_max_compute_units(int maxcomp);

	// methods which actually calculate something
	/**
	 * Calculate plaquette and polyakov of a specific gaugefield (on device).
	 *
	 * @param[in] gf gaugefield to measure on
	 * @param[out] plaq Storage for result of plaquette calculation
	 * @param[out] tplaq Storage for result of plaquette calculation
	 * @param[out] splaq Storage for result of plaquette calculation
	 * @param[out] pol Storage for result of polyakov calculation
	 */
	void gaugeobservables(cl_mem gf, hmc_float * const plaq, hmc_float * const tplaq, hmc_float * const splaq, hmc_complex * const pol);
	/**
	 * Calculate plaquette for a specific gaugefield (on device).
	 *
	 * @param[in] gf gaugefield to measure on
	 */
	void plaquette_device(cl_mem gf);
	/**
	 * Calculate Polyakov loop for a specific gaugefield (on device).
	 *
	 * @param[in] gf gaugefield to measure on
	 */
	void polyakov_device(cl_mem gf);

	// OpenCL specific methods needed for building/compiling the OpenCL program
	/**
	 * Collect the compiler options for OpenCL.
	 * Virtual method, allows to include more options in inherited classes.
	 */
	virtual void fill_collect_options(stringstream* collect_options);
	/**
	 * Collect the buffers to generate for OpenCL.
	 * Virtual method, allows to include more buffers in inherited classes.
	 */
	virtual void fill_buffers();
	/**
	 * Collect the kernels for OpenCL.
	 * Virtual method, allows to include more kernels in inherited classes.
	 */
	virtual void fill_kernels();
	/**
	 * Clear out the kernels,
	 * Virtual method, allows to clear additional kernels in inherited classes.
	 */
	virtual void clear_kernels();
	/**
	 * Clear out the buffers,
	 * Virtual method, allows to clear additional buffers in inherited classes.
	 */
	virtual void clear_buffers();
	/**
	 * Contains the list of kernel files after call to fill_kernels_file().
	 */
	std::vector<std::string> cl_kernels_file;

	/**
	 *  This calls
	 *    clCreateBuffer(context, CL_MEM_READ_WRITE, size, 0, &clerr)
	 *  and returns a pointer to a read-write cl_mem-object if clerr is HMC_SUCCESS
	 *  @param size size of buffer
	 */
	cl_mem create_rw_buffer(size_t size);

	/**
	 *  This calls
	 *    clCreateBuffer(context, CL_MEM_READ_ONLY, size, 0, &clerr)
	 *  and returns a pointer to a read-only cl_mem-object if clerr is HMC_SUCCESS
	 *  @param size size of buffer
	 */
	cl_mem create_wo_buffer(size_t size);

	/**
	 *  This calls
	 *    clCreateBuffer(context, CL_MEM_WRITE_ONLY, size, 0, &clerr)
	 *  and returns a pointer to a write-only cl_mem-object if clerr is HMC_SUCCESS
	 *  @param size size of buffer
	 */
	cl_mem create_ro_buffer(size_t size);

	/**
	 *  This calls
	 *    clCreateBuffer(context, CL_MEM_USE_HOST_PTR, size, host_pointer, &clerr);
	 *  and returns a pointer to a cl_mem-object located on the host if clerr is HMC_SUCCESS
	 *  @param size size of buffer
	 *  @param host_pointer pointer to memory on host
	 */
	cl_mem create_uhp_buffer(size_t size, void *host_pointer);

	/**
	 *  This calls
	 *    clCreateBuffer(context, CL_MEM_ALLOC_HOST_PTR, size, host_pointer, &clerr);
	 *  and returns a pointer to a cl_mem-object located on the host and allocats memory on the host
	 *  if clerr is HMC_SUCCESS
	 *  @param size size of buffer
	 *  @param host_pointer pointer to memory on host
	 */
	cl_mem create_ahp_buffer(size_t size, void *host_pointer);

	/**
	 *  This calls
	 *    clCreateBuffer(context, CL_MEM_USE_HOST_PTR, size, host_pointer, &clerr);
	 *  and returns a pointer to a cl_mem-object located on the device and
	 *  then copies host-memory pointed to by host-pointer to the device
	 *  if clerr is HMC_SUCCESS
	 *  @param size size of buffer
	 *  @param host_pointer pointer to memory on host
	 */
	cl_mem create_chp_buffer(size_t size, void *host_pointer);

	/**
	 * comutes work-sizes for a kernel
	 * @todo autotune
	 * @param ls local-work-size
	 * @param gs global-work-size
	 * @param num_groups number of work groups
	 * @param dev_type type of device on which the kernel should be executed
	 * @param name name of the kernel for possible autotune-usage, not yet used!!
	 */
	virtual void get_work_sizes(const cl_kernel kernel, cl_device_type dev_type, size_t * ls, size_t * gs, cl_uint * num_groups);

	/**
	 * Return the kernel name as a string
	 * @param[in] kernel
	 * @return kernel_name
	 */
	string get_kernel_name(const cl_kernel kernel);

#ifdef _PROFILING_
	//CP: if PROFILING is activated, one needs a timer for each kernel
	usetimer timer_plaquette;
	usetimer timer_plaquette_reduction;
	usetimer timer_polyakov;
	usetimer timer_polyakov_reduction;

	usetimer timer_stout_smear;
	/**
	 * Return the timer connected to a specific kernel.
	 *
	 * @param in Name of the kernel under consideration.
	 */
	virtual usetimer* get_timer(char * in);

	/**
	 * Return amount of bytes read and written by a specific kernel per call.
	 *
	 * @param in Name of the kernel under consideration.
	 */
	virtual int get_read_write_size(char * in, inputparameters * parameters);

	/**
	 * Print the profiling information of a specific kernel to a file.
	 *
	 * @param filename Name of file where data is appended.
	 * @param kernelName Name of specific kernel.
	 * @param time_total total execution time
	 * @param calls_total total number of kernel calls
	 * @param read_write_size number of bytes read and written by the kernel
	 */
	void print_profiling(std::string filename, const char * kernelName, uint64_t time_total, int calls_total, int read_write_size);

	/**
	 * Print the profiling information to a file.
	 *
	 * @param filename Name of file where data is appended.
	 */
	void virtual print_profiling(std::string filename);

#endif

	/**
	 * Enqueue the given kernel on the device. Local work size will be determined
	 * automatically from device and kernel properties.
	 *
	 * @param kernel The kernel to execute.
	 * @param global_work_size The number of threads to run.
	 *
	 * @todo local work size decision might need ot become less automatic
	 * @todo global work size will also depend on device ...
	 */
	void enqueueKernel(const cl_kernel kernel, const size_t global_work_size);

	/**
	 * Enqueue the given kernel on the device. Local work size will be determined
	 * automatically from device and kernel properties.
	 *
	 * @param kernel The kernel to execute.
	 * @param global_work_size The number of threads to run.
	 *
	 * @todo local work size decision might need ot become less automatic
	 * @todo global work size will also depend on device ...
	 */
	void enqueueKernel(const cl_kernel kernel, const size_t global_work_size, const size_t local_work_size);

	/**
	 * Print resource requirements of a kernel object.
	 *
	 * All information is dumped to the trace.
	 *
	 * @param kernel The kernel of which to query the information.
	 */
	void printResourceRequirements(const cl_kernel kernel);


	/**
	 * Copy content of a buffer to another buffer inside a queue using
	 *     clEnqueueCopyBuffer(queue, in, out, 0, 0, size , 0, 0, NULL);
	 * @param in source
	 * @param out destination
	 * @param size size of data (out must be equal or bigger than size)
	 */
	void copy_buffer_on_device(cl_mem in, cl_mem out, size_t size);
	/**
	 * Copy content of a buffer on host to a buffer on device inside a queue using
	 *     clEnqueueWriteBuffer(queue, dest, CL_TRUE, 0, size, source, 0, 0, NULL);
	 * This call is a blocking write.
	 * @param source
	 * @param dest
	 * @param size size of data (out must be equal or bigger than size)
	 */
	void copy_buffer_to_device(void * source, cl_mem dest, size_t size);
	/**
	 * Copy content of a buffer on device to a buffer on host inside a queue using
	 *    clEnqueueReadBuffer(queue, source, CL_TRUE, 0, size, dest, 0, NULL, NULL);
	 * This call is a blocking read.
	 * @param source
	 * @param dest
	 * @param size size of data (out must be equal or bigger than size)
	 */
	void get_buffer_from_device(cl_mem source, void * dest, size_t size);

	/**
	 * This applies stout smearing to a gaugefield
	 */
	void smear_gaugefield(cl_mem gf);
	void stout_smear_device();

	/**
	 * This replaces the stout smeared gaugefield with the unsmeared one
	 */
	void unsmear_gaugefield(cl_mem gf);
	
	usetimer * get_copy_to();
	usetimer * get_copy_on();

	/**
	 * Prints the copy-to/from/on-device-times
	 */
	void print_copy_times(uint64_t totaltime);

protected:
	/**
	 * A set of source files used by all kernels.
	 */
	ClSourcePackage basic_opencl_code;

	/**
	 * Create a kernel from source files.
	 *
	 * Usage:
	 * @code
	 * cl_kernel dummy = createKernel("dummy") << "dummy.cl";
	 * @endcode
	 *
	 * @param kernel_name The name of the kernel to create.
	 */
	TmpClKernel createKernel(const char * const kernel_name);

	/**
	 * Get number of threads
	 * @return int numthreads
	 *
	 */
	int get_numthreads();

protected:
	inputparameters* parameters;

private:

	cl_platform_id platform;
	cl_context ocl_context;
	cl_command_queue ocl_queue;
	cl_mem* ocl_gaugefield;

	cl_uint max_compute_units;
	string device_double_extension;

	cl_device_id device;
	cl_device_type device_type;

	cl_mem clmem_plaq;
	cl_mem clmem_plaq_buf_glob;
	cl_mem clmem_splaq_buf_glob;
	cl_mem clmem_tplaq_buf_glob;
	cl_mem clmem_splaq;
	cl_mem clmem_tplaq;
	cl_mem clmem_polyakov;
	cl_mem clmem_polyakov_buf_glob;

	//this is used to save the unsmeared gaugefield if smearing is used
	cl_mem gf_unsmeared;
	
	//since this is only applicated to the gaugefield, this should be here...
	cl_kernel stout_smear;

	cl_kernel plaquette;
	cl_kernel plaquette_reduction;
	cl_kernel polyakov;
	cl_kernel polyakov_reduction;

	//bunch of timers
	//this is used to measure data-transfer to and from the device
	usetimer copy_to;
	//this is used to measure data-transfer on the device
	usetimer copy_on;

	int numthreads;

};

#endif //OPENCLMODULEH
