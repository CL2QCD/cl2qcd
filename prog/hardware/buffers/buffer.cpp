/** @file
 * Implementation of the hardware::buffers::Buffer class
 *
 * (c) 2012 Matthias Bach <bach@compeng.uni-frankfurt.de>
 */

#include "buffer.hpp"

#include "../system.hpp"
#include "../device.hpp"
#include "../../logger.hpp"
#include "../../crypto/md5.h"
#include "../code/buffer.hpp"

static cl_mem allocateBuffer(size_t bytes, cl_context context, bool place_on_host);
void memObjectReleased(cl_mem, void * user_data);
struct MemObjectAllocationTracer {
	size_t bytes;
	bool host;
	hardware::Device * device;

	MemObjectAllocationTracer(size_t bytes, bool host, hardware::Device * device)
	 : bytes(bytes), host(host), device(device) {
		device->markMemAllocated(host, bytes);
	 };

	~MemObjectAllocationTracer() {
		device->markMemReleased(host, bytes);
	}
};


hardware::buffers::Buffer::Buffer(size_t bytes, hardware::Device * device, bool place_on_host)
	: bytes(bytes), cl_buffer(allocateBuffer(bytes, device->context, place_on_host)), device(device)
{
	// notify device about allocation
	cl_int err = clSetMemObjectDestructorCallback(cl_buffer, memObjectReleased, new MemObjectAllocationTracer(bytes, place_on_host, device));
	if(err) {
		throw hardware::OpenclException(err, "clSetMemObjectDestructorCallback", __FILE__, __LINE__);
	}
}

hardware::buffers::Buffer::~Buffer()
{
	clReleaseMemObject(cl_buffer);
}

static cl_mem allocateBuffer(size_t bytes, cl_context context, const bool place_on_host)
{
	cl_int err;
	const cl_mem_flags mem_flags = place_on_host ? CL_MEM_ALLOC_HOST_PTR : 0;
	cl_mem cl_buffer = clCreateBuffer(context, mem_flags, bytes, 0, &err);
	if(err) {
		throw hardware::OpenclException(err, "clCreateBuffer", __FILE__, __LINE__);
	}
	return cl_buffer;
}

hardware::buffers::Buffer::operator const cl_mem*() const noexcept
{
	return &cl_buffer;
}

size_t hardware::buffers::Buffer::get_bytes() const noexcept
{
	return bytes;
}

void hardware::buffers::Buffer::load(const void * array, size_t bytes, size_t offset) const
{
	if(bytes == 0) {
		bytes = this->bytes;
	} else {
		if(bytes + offset > this->bytes) {
			logger.error() << "Writing outside buffer. Bytes: " << bytes << " Offset: " << offset << " Buffer size: " << this->bytes;
			throw std::out_of_range("You are loading to memory outside of the buffer.");
		}
	}
	logger.trace() << "clEnqueueWriteBuffer(...,...,...," << offset << ',' << bytes << ",...,0,nullptr,nullptr)";
	cl_int err = clEnqueueWriteBuffer(*device, cl_buffer, CL_TRUE, offset, bytes, array, 0, nullptr, nullptr);
	if(err) {
		throw hardware::OpenclException(err, "clEnqueueWriteBuffer", __FILE__, __LINE__);
	}
}

void hardware::buffers::Buffer::dump(void * array, size_t bytes, size_t offset) const
{
	if(bytes == 0) {
		bytes = this->bytes;
	} else {
		if(bytes + offset > this->bytes) {
			logger.error() << "Reading outside buffer. Bytes: " << bytes << " Offset: " << offset << " Buffer size: " << this->bytes;
			throw std::out_of_range("You are reading from outside of the buffer.");
		}
	}
	logger.trace() << "clEnqueueReadBuffer(...,...,...," << offset << ',' << bytes << ",...,0,nullptr,nullptr)";
	cl_int err = clEnqueueReadBuffer(*device, cl_buffer, CL_TRUE, offset, bytes, array, 0, nullptr, nullptr);
	if(err) {
		throw hardware::OpenclException(err, "clEnqueueReadBuffer", __FILE__, __LINE__);
	}
}

void hardware::buffers::Buffer::load_rect(const void* src, const size_t *buffer_origin, const size_t *host_origin, const size_t *region, size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch) const
{
	cl_int err = clEnqueueWriteBufferRect(*device, cl_buffer, CL_TRUE, buffer_origin, host_origin, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, src, 0, nullptr, nullptr);
	if(err) {
		throw hardware::OpenclException(err, "clEnqueueReadBufferRect", __FILE__, __LINE__);
	}
}

void hardware::buffers::Buffer::dump_rect(void* src, const size_t *buffer_origin, const size_t *host_origin, const size_t *region, size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch) const
{
	cl_int err = clEnqueueReadBufferRect(*device, cl_buffer, CL_TRUE, buffer_origin, host_origin, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, src, 0, nullptr, nullptr);
	if(err) {
		throw hardware::OpenclException(err, "clEnqueueReadBufferRect", __FILE__, __LINE__);
	}
}

hardware::SynchronizationEvent hardware::buffers::Buffer::load_rectAsync(const void* src, const size_t *buffer_origin, const size_t *host_origin, const size_t *region, size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch, const hardware::SynchronizationEvent& event) const
{
	const cl_event * wait_event = nullptr;
	cl_uint num_wait_events = 0;
	if(event.raw()) {
		wait_event = &event.raw();
		num_wait_events = 1;
	}

	cl_event event_cl;
	cl_int err = clEnqueueWriteBufferRect(*device, cl_buffer, CL_FALSE, buffer_origin, host_origin, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, src, num_wait_events, wait_event, &event_cl);
	if(err) {
		throw hardware::OpenclException(err, "clEnqueueReadBufferRect", __FILE__, __LINE__);
	}

	const hardware::SynchronizationEvent new_event(event_cl);
	err = clReleaseEvent(event_cl);
	if(err) {
		throw hardware::OpenclException(err, "clReleaseEvent", __FILE__, __LINE__);
	}
	return new_event;
}

hardware::SynchronizationEvent hardware::buffers::Buffer::dump_rectAsync(void* dest, const size_t *buffer_origin, const size_t *host_origin, const size_t *region, size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch, const hardware::SynchronizationEvent& event) const
{
	const cl_event * wait_event = nullptr;
	cl_uint num_wait_events = 0;
	if(event.raw()) {
		wait_event = &event.raw();
		num_wait_events = 1;
	}

	cl_event event_cl;
	cl_int err = clEnqueueReadBufferRect(*device, cl_buffer, CL_FALSE, buffer_origin, host_origin, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, dest, num_wait_events, wait_event, &event_cl);
	if(err) {
		throw hardware::OpenclException(err, "clEnqueueReadBufferRect", __FILE__, __LINE__);
	}

	const hardware::SynchronizationEvent new_event(event_cl);
	err = clReleaseEvent(event_cl);
	if(err) {
		throw hardware::OpenclException(err, "clReleaseEvent", __FILE__, __LINE__);
	}
	return new_event;
}

hardware::SynchronizationEvent hardware::buffers::Buffer::load_async(const void * array) const
{
	cl_event event_cl;
	cl_int err = clEnqueueWriteBuffer(*device, cl_buffer, CL_FALSE, 0, bytes, array, 0, nullptr, &event_cl);
	if(err) {
		throw hardware::OpenclException(err, "clEnqueueReadBuffer", __FILE__, __LINE__);
	}

	const hardware::SynchronizationEvent event(event_cl);
	err = clReleaseEvent(event_cl);
	if(err) {
		throw hardware::OpenclException(err, "clReleaseEvent", __FILE__, __LINE__);
	}
	return event;
}

hardware::SynchronizationEvent hardware::buffers::Buffer::dump_async(void * array) const
{
	cl_event event_cl;
	cl_int err = clEnqueueReadBuffer(*device, cl_buffer, CL_FALSE, 0, bytes, array, 0, nullptr, &event_cl);
	if(err) {
		throw hardware::OpenclException(err, "clEnqueueReadBuffer", __FILE__, __LINE__);
	}

	const hardware::SynchronizationEvent event(event_cl);
	err = clReleaseEvent(event_cl);
	if(err) {
		throw hardware::OpenclException(err, "clReleaseEvent", __FILE__, __LINE__);
	}
	return event;
}

const cl_mem* hardware::buffers::Buffer::get_cl_buffer() const noexcept
{
	return &cl_buffer;
}

hardware::Device * hardware::buffers::Buffer::get_device() const noexcept
{
	return device;
}

void hardware::buffers::Buffer::copyData(const Buffer* orig) const
{
	if(this->bytes != orig->bytes) {
		throw std::invalid_argument("The source and destination buffer must be of equal size!");
	} else {
		/*
		 * Now we have to play with the device a little.
		 * It seems on AMD hardware the buffer copy thing either pretty much sucks or I am using it wrong.
		 */
		const std::string dev_name = device->get_name();
		if(this->bytes == 16 && (dev_name == "Cypress" || dev_name == "Cayman")) {
			logger.debug() << "Using an OpenCL kernel to copy 16 bytes on " << dev_name << '.';
			device->get_buffer_code()->copy_16_bytes(this, orig);
		} else {
			logger.debug() << "Using default OpenCL buffer copy method for " << this->bytes << " bytes on " << dev_name << '.';
			int err = clEnqueueCopyBuffer(device->get_queue(), orig->cl_buffer, this->cl_buffer, 0, 0, this->bytes, 0, nullptr, nullptr);
			if(err) {
				throw hardware::OpenclException(err, "clEnqueueCopyBuffer", __FILE__, __LINE__);
			}
		}
	}
}
void hardware::buffers::Buffer::copyDataBlock(const Buffer* orig, const size_t dest_offset, const size_t src_offset, const size_t bytes) const
{
	logger.debug() << "Copying " << bytes << " bytes from offset " << src_offset << " to offset " << dest_offset;
	logger.debug() << "Source buffer size: " << orig->bytes;
	logger.debug() << "Dest buffer size:   " << this->bytes;
	if(this->bytes < dest_offset + bytes || orig->bytes < src_offset + bytes) {
		throw std::invalid_argument("Copy range exceeds buffer size!");
	} else {
		int err = clEnqueueCopyBuffer(device->get_queue(), orig->cl_buffer, this->cl_buffer, src_offset, dest_offset, bytes, 0, nullptr, nullptr);
		if(err) {
			throw hardware::OpenclException(err, "clEnqueueCopyBuffer", __FILE__, __LINE__);
		}
	}
}

void hardware::buffers::Buffer::clear() const
{
#ifdef CL_VERSION_1_2
	device->synchronize();
	if(sizeof(hmc_complex_zero) % bytes) {
		cl_char foo = 0;
		cl_int err = clEnqueueFillBuffer(*device, cl_buffer, &foo, sizeof(foo), 0, bytes, 0, nullptr, nullptr);
		if(err) {
			throw hardware::OpenclException(err, "clEnqueueFillBuffer", __FILE__, __LINE__);
		}
	} else {
		cl_int err = clEnqueueFillBuffer(*device, cl_buffer, &hmc_complex_zero, sizeof(hmc_complex_zero), 0, bytes, 0, nullptr, nullptr);
		if(err) {
			throw hardware::OpenclException(err, "clEnqueueFillBuffer", __FILE__, __LINE__);
		}
	}
#else
	device->get_buffer_code()->clear(this);
#endif
}

std::string hardware::buffers::md5(const Buffer* buf)
{
	md5_t md5_state;
	md5_init(&md5_state);

	char* data = new char[buf->bytes];
	buf->dump(data);

	md5_process(&md5_state, data, buf->bytes);

	delete[] data;

	char sig[MD5_SIZE];
	md5_finish(&md5_state, sig);

	char res[33];
	md5_sig_to_string(sig, res, 33);

	return std::string(res);
}

hardware::SynchronizationEvent hardware::buffers::copyDataRect(const hardware::Device* device, const hardware::buffers::Buffer* dest, const hardware::buffers::Buffer* orig, const size_t *dest_origin, const size_t *src_origin, const size_t *region, size_t dest_row_pitch, size_t dest_slice_pitch, size_t src_row_pitch, size_t src_slice_pitch, const std::vector<hardware::SynchronizationEvent> & events)
{
	auto const raw_events = get_raw_events(events);
	cl_event const * const raw_events_p = raw_events.size() > 0 ? raw_events.data() : nullptr;

	cl_event event_cl;
	cl_int err = clEnqueueCopyBufferRect(*device, *orig->get_cl_buffer(), *dest->get_cl_buffer(), src_origin, dest_origin, region, src_row_pitch, src_slice_pitch, dest_row_pitch, dest_slice_pitch, raw_events.size(), raw_events_p, &event_cl);
	if(err) {
		throw hardware::OpenclException(err, "clEnqueueCopyBufferRect", __FILE__, __LINE__);
	}

	const hardware::SynchronizationEvent new_event(event_cl);
	err = clReleaseEvent(event_cl);
	if(err) {
		throw hardware::OpenclException(err, "clReleaseEvent", __FILE__, __LINE__);
	}
	return new_event;
}

#ifdef CL_VERSION_1_2
void hardware::buffers::Buffer::migrate(hardware::Device * device, const std::vector<hardware::SynchronizationEvent>& events, cl_mem_migration_flags flags)
{
	size_t num_events = events.size();
	std::vector<cl_event> cl_events(num_events);
	for(size_t i = 0; i < num_events; ++i) {
		cl_events[i] = events[i].raw();
	}
	cl_event * events_p = (num_events > 0) ? &cl_events[0] : 0;

	cl_int err = clEnqueueMigrateMemObjects(*device, 1, this->get_cl_buffer(), flags, num_events, events_p, 0);
	if(err) {
		throw hardware::OpenclException(err, "clEnqueueMigrateMemoryObjects", __FILE__, __LINE__);
	}

	// update device used by this buffer
	this->device = device;
}
#endif

void memObjectReleased(cl_mem, void * user_data)
{
	MemObjectAllocationTracer * release_info = static_cast<MemObjectAllocationTracer *>(user_data);
	delete release_info;
}
