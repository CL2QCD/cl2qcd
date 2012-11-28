/** @file
 * Implementation of the hardware::buffers::Matrix3x3 class
 *
 * (c) 2012 Matthias Bach <bach@compeng.uni-frankfurt.de>
 */

#include "3x3.hpp"
#include "../device.hpp"

#include <stdexcept>

typedef hmc_complex soa_storage_t;
const size_t soa_storage_lanes = 9;

static size_t calculate_3x3_buffer_size(size_t elems, hardware::Device * device);

hardware::buffers::Matrix3x3::Matrix3x3(size_t elems, hardware::Device * device)
	: Buffer(calculate_3x3_buffer_size(elems, device), device),
	  elems(elems),
	  soa(check_Matrix3x3_for_SOA(device))
{
	// nothing to do
}

size_t hardware::buffers::check_Matrix3x3_for_SOA(hardware::Device * device)
{
	return device->get_prefers_soa();
}

static size_t calculate_3x3_buffer_size(size_t elems, hardware::Device * device)
{
	using namespace hardware::buffers;
	if(check_Matrix3x3_for_SOA(device)) {
		size_t stride = get_Matrix3x3_buffer_stride(elems, device);
		return stride * soa_storage_lanes * sizeof(soa_storage_t);
	} else {
		return elems * sizeof(::Matrix3x3);
	}
}

size_t hardware::buffers::get_Matrix3x3_buffer_stride(size_t elems, Device * device)
{
	return device->recommend_stride(elems, sizeof(soa_storage_t), soa_storage_lanes);
}

size_t hardware::buffers::Matrix3x3::get_elements() const noexcept
{
	return elems;
}

bool hardware::buffers::Matrix3x3::is_soa() const noexcept
{
	return soa;
}
