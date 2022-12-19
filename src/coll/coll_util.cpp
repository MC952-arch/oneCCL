/*
 Copyright 2016-2020 Intel Corporation
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
     http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/
#include "coll/coll_util.hpp"
#include "coll/selection/selection.hpp"
#include "sched/entry/factory/entry_factory.hpp"
#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)
#include "sched/entry/ze/ze_event_signal_entry.hpp"
#include "sched/entry/ze/ze_event_wait_entry.hpp"
#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL

namespace ccl {

void add_coll_entry(ccl_sched* sched, const ccl_coll_entry_param& param) {
    ccl_selector_param selector_param;

    selector_param.ctype = param.ctype;
    selector_param.count = param.count;
    if (param.ctype == ccl_coll_allgatherv) {
        selector_param.count = param.send_count;
    }
    selector_param.recv_counts = param.recv_counts;
    selector_param.dtype = param.dtype;
    selector_param.comm = param.comm;
    selector_param.stream = param.stream;
    selector_param.buf = (param.send_buf) ? param.send_buf.get_ptr() : param.recv_buf.get_ptr();
    selector_param.is_vector_buf = sched->coll_attr.is_vector_buf;
#ifdef CCL_ENABLE_SYCL
    selector_param.is_sycl_buf = sched->coll_attr.is_sycl_buf;
#endif // CCL_ENABLE_SYCL
    selector_param.hint_algo = param.hint_algo;
    selector_param.is_scaleout = param.is_scaleout;

    if (ccl_is_device_side_algo(selector_param)) {
        sched->strict_order = true;
    }

    if ((ccl::global_data::env().atl_transport == ccl_atl_mpi) &&
        ccl_is_direct_algo(selector_param)) {
        /* entry directly into schedule due to performance reasons */
        coll_entry::build_sched(sched, param);
    }
    else {
        entry_factory::create<coll_entry>(sched, param);
    }
}

#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)

void add_wait_events(ccl_sched* sched, const std::vector<ze_event_handle_t>& wait_events) {
    if (wait_events.size() > 0) {
        entry_factory::create<ze_event_wait_entry>(sched, wait_events);
        sched->add_barrier();
    }
}

void add_signal_event(ccl_sched* sched, ze_event_handle_t signal_event) {
    if (signal_event) {
        entry_factory::create<ze_event_signal_entry>(sched, signal_event);
        sched->add_barrier();
    }
}

ze_event_handle_t add_signal_event(ccl_sched* sched) {
    auto signal_event = sched->get_memory().event_manager->create();
    add_signal_event(sched, signal_event);
    return signal_event;
}

void add_comm_barrier(ccl_sched* sched,
                      ccl_comm* comm,
                      ze_event_pool_handle_t ipc_pool,
                      size_t ipc_event_idx) {
    sched->add_barrier();
    if (ipc_pool && global_data::env().enable_ze_barrier) {
        entry_factory::create<ze_barrier_entry>(sched, comm, ipc_pool, ipc_event_idx);
    }
    else {
        ccl_coll_entry_param barrier_param{};
        barrier_param.ctype = ccl_coll_barrier;
        barrier_param.comm = comm;

        /* TODO: optimize p2p based barrier */
        //barrier_param.hint_algo.barrier = ccl_coll_barrier_ring;

        add_coll_entry(sched, barrier_param);
    }
    sched->add_barrier();
}

void add_comm_barrier(ccl_sched* sched,
                      ccl_comm* comm,
                      std::vector<ze_event_handle_t>& wait_events,
                      ze_event_pool_handle_t ipc_pool,
                      size_t ipc_event_idx) {
    sched->add_barrier();
    auto signal_event = sched->get_memory().event_manager->create();
    if (sched->use_single_list) {
        add_wait_events(sched, wait_events);
        add_comm_barrier(sched, comm, ipc_pool, ipc_event_idx);
        add_signal_event(sched, signal_event);
    }
    else {
        add_comm_barrier(sched, comm, ipc_pool, ipc_event_idx);
        add_signal_event(sched, signal_event);
    }
    sched->add_barrier();
    wait_events.push_back(signal_event);
}

void add_handle_exchange(ccl_sched* sched,
                         ccl_comm* comm,
                         const std::vector<ze_handle_exchange_entry::mem_desc_t>& in_buffers,
                         int skip_rank,
                         ze_event_pool_handle_t pool,
                         size_t event_idx) {
    if (sched->coll_attr.to_cache) {
        sched->set_entry_exec_mode(ccl_sched_entry_exec_once);
        entry_factory::create<ze_handle_exchange_entry>(sched, comm, in_buffers, skip_rank);
        sched->add_barrier();
        sched->set_entry_exec_mode(ccl_sched_entry_exec_regular);

        // TODO: no need barrier for the first iteration where ze_handle_exchange_entry exists
        add_comm_barrier(sched, comm, pool, event_idx);
    }
    else {
        entry_factory::create<ze_handle_exchange_entry>(sched, comm, in_buffers, skip_rank);
        sched->add_barrier();
    }
}

void add_coll(ccl_sched* sched,
              const ccl_coll_entry_param& param,
              std::vector<ze_event_handle_t>& wait_events) {
    if (sched->use_single_list) {
        ccl::add_wait_events(sched, wait_events);
    }
    if (ccl::global_data::env().ze_multi_workers) {
        ccl_coll_attr attr{};
        ccl_coll_param coll_param;
        switch (param.ctype) {
            case ccl_coll_allreduce: {
                coll_param = ccl_coll_param::create_allreduce_param(param.send_buf.get_src(),
                                                                    param.recv_buf.get_src(),
                                                                    param.count,
                                                                    param.dtype.idx(),
                                                                    param.reduction,
                                                                    attr,
                                                                    param.comm,
                                                                    param.stream);
                break;
            }
            case ccl_coll_reduce: {
                coll_param = ccl_coll_param::create_reduce_param(param.send_buf.get_src(),
                                                                 param.recv_buf.get_src(),
                                                                 param.count,
                                                                 param.dtype.idx(),
                                                                 param.reduction,
                                                                 param.root,
                                                                 attr,
                                                                 param.comm,
                                                                 param.stream);
                break;
            }
            case ccl_coll_alltoallv: {
                coll_param = ccl_coll_param::create_alltoallv_param(param.send_buf.get_src(),
                                                                    param.send_counts,
                                                                    param.recv_buf.get_src(),
                                                                    param.recv_counts,
                                                                    param.dtype.idx(),
                                                                    attr,
                                                                    param.comm,
                                                                    param.stream);

                break;
            }
            case ccl_coll_allgatherv: {
                coll_param = ccl_coll_param::create_allgatherv_param(param.send_buf.get_src(),
                                                                     param.send_count,
                                                                     param.recv_buf.get_src(),
                                                                     param.recv_counts,
                                                                     param.dtype.idx(),
                                                                     attr,
                                                                     param.comm,
                                                                     param.stream);
                break;
            }
            default: CCL_THROW("unexpected coll type", ccl_coll_type_to_str(param.ctype));
        }
        LOG_DEBUG("scaleout/multi_workers: created params for: ",
                  ccl_coll_type_to_str(param.ctype),
                  " coll");
        // pass the scale-out selection param through factory
        coll_param.is_scaleout = param.is_scaleout;
        ccl_sched_create_param sched_param(sched->sched_id, coll_param);
        entry_factory::create<subsched_entry>(sched, 0, sched_param, "SCALEOUT");
    }
    else {
        coll_entry::build_sched(sched, param);
    }
    sched->add_barrier();

    if (sched->use_single_list) {
        auto signal_event = ccl::add_signal_event(sched);
        wait_events.push_back(signal_event);
    }
}

void add_scaleout(ccl_sched* sched,
                  const ccl_coll_entry_param& in_coll_param,
                  const bool is_single_node,
                  std::vector<ze_event_handle_t>& wait_events,
                  const copy_attr& h2d_copy_attr,
                  ccl_comm* global_comm,
                  ccl_buffer global_recv_buf,
                  int global_root) {
    ccl_coll_entry_param coll_param(in_coll_param);

    bool multi_node = (!is_single_node && (coll_param.count || coll_param.recv_counts));
    bool enable_hmem = (ccl::global_data::env().use_hmem && atl_base_comm::attr.out.enable_hmem);
    bool do_h2d_copy =
        ((coll_param.ctype == ccl_coll_allreduce || coll_param.ctype == ccl_coll_alltoallv ||
          coll_param.ctype == ccl_coll_alltoall || coll_param.ctype == ccl_coll_allgatherv) &&
         multi_node && !enable_hmem) ||
        (coll_param.ctype == ccl_coll_reduce && coll_param.comm->rank() == coll_param.root);

    auto copy_entry =
        [&](ccl_buffer src, ccl_buffer dst, const size_t count, const copy_attr& copy_attr) {
            LOG_DEBUG("topo/scale_out/intra: use ze_copy_entry");
            auto entry = entry_factory::create<ze_copy_entry>(
                sched, src, dst, count, coll_param.dtype, copy_attr, wait_events);
            wait_events.push_back(entry->entry_event);
        };

    auto copy_entry_with_offset = [&](std::vector<ccl_buffer> bufs,
                                      ccl_buffer buf,
                                      const size_t* counts,
                                      const copy_attr& copy_attr) {
        size_t offset = 0;
        // number of not skipped s/r_counts, helps calculate the offset
        for (int idx = 0; idx < coll_param.comm->size(); idx++) {
            if (counts[idx] == 0) {
                continue;
            }

            ccl_buffer src = bufs[idx];
            ccl_buffer dst = buf + offset;
            if (copy_attr.direction == copy_direction::h2d) {
                src = buf + offset;
                dst = bufs[idx];
            }
            copy_entry(src, dst, counts[idx], copy_attr);
            offset += counts[idx] * coll_param.dtype.size();
        }
        LOG_DEBUG("copy_entry_with_offset done");
    };

    if (multi_node) {
        if (!enable_hmem) {
            LOG_DEBUG("topo/scale_out: use host_", ccl_coll_type_to_str(coll_param.ctype));

            size_t host_buf_size = 0;
            if (coll_param.ctype == ccl_coll_alltoallv || coll_param.ctype == ccl_coll_alltoall ||
                coll_param.ctype == ccl_coll_allgatherv) {
                // assume sum of send_counts and recv_counts are equal for alltoallv
                host_buf_size = std::accumulate(coll_param.recv_counts,
                                                coll_param.recv_counts + coll_param.comm->size(),
                                                0) *
                                coll_param.dtype.size();
                LOG_DEBUG("alltoall(v) scale_out host buf size: ", host_buf_size);
            }
            else {
                host_buf_size = coll_param.count * coll_param.dtype.size();
            }

            CCL_THROW_IF_NOT(host_buf_size != invalid_host_buf_size,
                             "unexpected the size of buffer in scaleout phase");
            ccl::alloc_param alloc_param(
                host_buf_size, ccl::buffer_type::regular, ccl::buffer_place::host);
            coll_param.send_buf = sched->alloc_buffer(alloc_param);
            coll_param.recv_buf = coll_param.send_buf;

            if (coll_param.ctype == ccl_coll_alltoallv || coll_param.ctype == ccl_coll_alltoall) {
                copy_entry_with_offset(in_coll_param.send_bufs,
                                       coll_param.send_buf,
                                       coll_param.send_counts,
                                       copy_attr(copy_direction::d2h));
            }
            else if (coll_param.ctype == ccl_coll_allgatherv) {
                size_t offset = std::accumulate(coll_param.recv_counts,
                                                coll_param.recv_counts + coll_param.comm->rank(),
                                                0) *
                                coll_param.dtype.size();
                copy_entry(in_coll_param.send_buf,
                           coll_param.send_buf + offset,
                           coll_param.send_count,
                           copy_attr(copy_direction::d2h));
            }
            else {
                copy_entry(in_coll_param.send_buf,
                           coll_param.send_buf,
                           coll_param.count,
                           copy_attr(copy_direction::d2h));
            }
            sched->add_barrier();

            LOG_DEBUG("topo/scale_out: ze_copy_entry of D2H for ",
                      ccl_coll_type_to_str(coll_param.ctype),
                      " done");
        }
        // pass the scale-out selection param directly
        coll_param.is_scaleout = true;
        // do inplace collective
        ccl::add_coll(sched, coll_param, wait_events);
    }

    if (!do_h2d_copy)
        return;

    ccl_buffer src_copy_buf = coll_param.recv_buf;
    ccl_buffer dst_copy_buf = in_coll_param.recv_buf;

    if (in_coll_param.ctype == ccl_coll_reduce) {
        if (!multi_node)
            src_copy_buf = in_coll_param.recv_buf;
        dst_copy_buf = (global_comm->rank() == global_root) ? global_recv_buf : ccl_buffer();
    }

    if (coll_param.ctype == ccl_coll_alltoallv || coll_param.ctype == ccl_coll_alltoall ||
        coll_param.ctype == ccl_coll_allgatherv) {
        copy_entry_with_offset(
            in_coll_param.recv_bufs, coll_param.recv_buf, coll_param.recv_counts, h2d_copy_attr);
    }
    else {
        copy_entry(src_copy_buf, dst_copy_buf, coll_param.count, h2d_copy_attr);
    }
    sched->add_barrier();

    LOG_DEBUG("topo/scale_out: ze_copy_entry of H2D for ",
              ccl_coll_type_to_str(coll_param.ctype),
              " done");
}
#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL

} // namespace ccl
