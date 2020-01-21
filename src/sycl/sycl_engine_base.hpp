/*******************************************************************************
* Copyright 2019 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#ifndef SYCL_ENGINE_BASE_HPP
#define SYCL_ENGINE_BASE_HPP

#include <memory>

#include "common/c_types_map.hpp"
#include "common/engine.hpp"
#include "common/memory_storage.hpp"
#include "common/stream.hpp"
#include "compute/compute.hpp"
#include "ocl/ocl_engine.hpp"
#include "ocl/ocl_gpu_engine.hpp"
#include "ocl/ocl_utils.hpp"
#include "sycl/sycl_device_info.hpp"
#include "sycl/sycl_ocl_gpu_kernel.hpp"

#include <CL/sycl.hpp>

namespace dnnl {
namespace impl {
namespace sycl {

class sycl_engine_base_t : public compute::compute_engine_t {
public:
    sycl_engine_base_t(engine_kind_t kind, const cl::sycl::device &dev,
            const cl::sycl::context &ctx)
        : compute::compute_engine_t(
                kind, runtime_kind::sycl, new sycl_device_info_t(dev))
        , device_(dev)
        , context_(ctx) {}

    status_t init() {
        CHECK(compute::compute_engine_t::init());
        stream_t *service_stream_ptr;
        status_t status = create_stream(
                &service_stream_ptr, stream_flags::default_flags);
        if (status != status::success) return status;
        service_stream_.reset(service_stream_ptr);
        return status::success;
    }

    virtual status_t create_memory_storage(memory_storage_t **storage,
            unsigned flags, size_t size, void *handle) override;

    virtual status_t create_stream(stream_t **stream, unsigned flags) override;
    status_t create_stream(stream_t **stream, cl::sycl::queue &queue);

    virtual status_t create_kernels(std::vector<compute::kernel_t> *kernels,
            const std::vector<const char *> &kernel_names,
            const compute::kernel_ctx_t &kernel_ctx) const override {
        if (kind() != engine_kind::gpu) {
            assert("not expected");
            return status::invalid_arguments;
        }
        ocl::ocl_engine_factory_t f(engine_kind::gpu);
        engine_t *ocl_engine_ptr;
        CHECK(f.engine_create(&ocl_engine_ptr, ocl_device(), ocl_context()));
        auto ocl_engine = std::unique_ptr<ocl::ocl_gpu_engine_t>(
                utils::downcast<ocl::ocl_gpu_engine_t *>(ocl_engine_ptr));

        std::vector<compute::kernel_t> ocl_kernels;
        CHECK(ocl_engine->create_kernels(
                &ocl_kernels, kernel_names, kernel_ctx));
        *kernels = std::vector<compute::kernel_t>(kernel_names.size());
        for (size_t i = 0; i < ocl_kernels.size(); ++i) {
            if (!ocl_kernels[i]) continue;

            auto *k = utils::downcast<ocl::ocl_gpu_kernel_t *>(
                    ocl_kernels[i].impl());
            auto ocl_kernel = k->ocl_kernel();
            OCL_CHECK(clRetainKernel(ocl_kernel));
            (*kernels)[i]
                    = compute::kernel_t(new sycl_ocl_gpu_kernel_t(ocl_kernel));
        }
        return status::success;
    }

    const cl::sycl::device &device() const { return device_; }
    const cl::sycl::context &context() const { return context_; }

    stream_t *service_stream() const { return service_stream_.get(); }

    cl_device_id ocl_device() const {
        assert(device_.is_cpu() || device_.is_gpu());
        return ocl::ocl_utils::make_ocl_wrapper(device().get());
    }
    cl_context ocl_context() const {
        assert(device_.is_cpu() || device_.is_gpu());
        return ocl::ocl_utils::make_ocl_wrapper(context().get());
    }

private:
    cl::sycl::device device_;
    cl::sycl::context context_;

    std::unique_ptr<stream_t> service_stream_;
};

} // namespace sycl
} // namespace impl
} // namespace dnnl

#endif