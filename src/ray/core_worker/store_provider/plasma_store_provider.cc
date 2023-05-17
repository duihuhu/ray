// Copyright 2017 The Ray Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ray/core_worker/store_provider/plasma_store_provider.h"

#include "ray/common/ray_config.h"
#include "ray/core_worker/context.h"
#include "ray/core_worker/core_worker.h"
#include "src/ray/protobuf/gcs.pb.h"

namespace ray {
namespace core {

void BufferTracker::Record(const ObjectID &object_id,
                           TrackedBuffer *buffer,
                           const std::string &call_site) {
  absl::MutexLock lock(&active_buffers_mutex_);
  active_buffers_[std::make_pair(object_id, buffer)] = call_site;
}

void BufferTracker::Release(const ObjectID &object_id, TrackedBuffer *buffer) {
  absl::MutexLock lock(&active_buffers_mutex_);
  auto key = std::make_pair(object_id, buffer);
  RAY_CHECK(active_buffers_.contains(key));
  active_buffers_.erase(key);
}

absl::flat_hash_map<ObjectID, std::pair<int64_t, std::string>>
BufferTracker::UsedObjects() const {
  absl::flat_hash_map<ObjectID, std::pair<int64_t, std::string>> used;
  absl::MutexLock lock(&active_buffers_mutex_);
  for (const auto &entry : active_buffers_) {
    auto it = used.find(entry.first.first);
    if (it != used.end()) {
      // Prefer to keep entries that have non-empty callsites.
      if (!it->second.second.empty()) {
        continue;
      }
    }
    used[entry.first.first] = std::make_pair(entry.first.second->Size(), entry.second);
  }
  return used;
}

CoreWorkerPlasmaStoreProvider::CoreWorkerPlasmaStoreProvider(
    const std::string &store_socket,
    const std::shared_ptr<raylet::RayletClient> raylet_client,
    const std::shared_ptr<ReferenceCounter> reference_counter,
    std::function<Status()> check_signals,
    bool warmup,
    std::function<std::string()> get_current_call_site)
    : raylet_client_(raylet_client),
      reference_counter_(reference_counter),
      check_signals_(check_signals) {
  if (get_current_call_site != nullptr) {
    get_current_call_site_ = get_current_call_site;
  } else {
    get_current_call_site_ = []() { return "<no callsite callback>"; };
  }
  object_store_full_delay_ms_ = RayConfig::instance().object_store_full_delay_ms();
  buffer_tracker_ = std::make_shared<BufferTracker>();
  RAY_CHECK_OK(store_client_.Connect(store_socket));
  if (warmup) {
    RAY_CHECK_OK(WarmupStore());
  }
}

CoreWorkerPlasmaStoreProvider::~CoreWorkerPlasmaStoreProvider() {
  RAY_IGNORE_EXPR(store_client_.Disconnect());
}

Status CoreWorkerPlasmaStoreProvider::Put(const RayObject &object,
                                          const ObjectID &object_id,
                                          const rpc::Address &owner_address,
                                          bool *object_exists) {
  RAY_CHECK(!object.IsInPlasmaError()) << object_id;
  std::shared_ptr<Buffer> data;
  RAY_RETURN_NOT_OK(Create(object.GetMetadata(),
                           object.HasData() ? object.GetData()->Size() : 0,
                           object_id,
                           owner_address,
                           &data,
                           /*created_by_worker=*/true));
  // data could be a nullptr if the ObjectID already existed, but this does
  // not throw an error.
  if (data != nullptr) {
    if (object.HasData()) {
      memcpy(data->Data(), object.GetData()->Data(), object.GetData()->Size());
    }
    RAY_RETURN_NOT_OK(Seal(object_id));
    if (object_exists) {
      *object_exists = false;
    }
  } else if (object_exists) {
    *object_exists = true;
  }
  return Status::OK();
}

Status CoreWorkerPlasmaStoreProvider::Create(const std::shared_ptr<Buffer> &metadata,
                                             const size_t data_size,
                                             const ObjectID &object_id,
                                             const rpc::Address &owner_address,
                                             std::shared_ptr<Buffer> *data,
                                             bool created_by_worker) {
  auto source = plasma::flatbuf::ObjectSource::CreatedByWorker;
  if (!created_by_worker) {
    source = plasma::flatbuf::ObjectSource::RestoredFromStorage;
  }
  Status status =
      store_client_.CreateAndSpillIfNeeded(object_id,
                                           owner_address,
                                           data_size,
                                           metadata ? metadata->Data() : nullptr,
                                           metadata ? metadata->Size() : 0,
                                           data,
                                           source,
                                           /*device_num=*/0);

  if (status.IsObjectStoreFull()) {
    RAY_LOG(ERROR) << "Failed to put object " << object_id
                   << " in object store because it "
                   << "is full. Object size is " << data_size << " bytes.\n"
                   << "Plasma store status:\n"
                   << MemoryUsageString() << "\n---\n"
                   << "--- Tip: Use the `ray memory` command to list active objects "
                      "in the cluster."
                   << "\n---\n";

    // Replace the status with a more helpful error message.
    std::ostringstream message;
    message << "Failed to put object " << object_id << " in object store because it "
            << "is full. Object size is " << data_size << " bytes.";
    status = Status::ObjectStoreFull(message.str());
  } else if (status.IsObjectExists()) {
    RAY_LOG(WARNING) << "Trying to put an object that already existed in plasma: "
                     << object_id << ".";
    status = Status::OK();
  } else {
    RAY_RETURN_NOT_OK(status);
  }
  return status;
}

Status CoreWorkerPlasmaStoreProvider::Seal(const ObjectID &object_id) {
  return store_client_.Seal(object_id);
}

Status CoreWorkerPlasmaStoreProvider::Release(const ObjectID &object_id) {
  return store_client_.Release(object_id);
}

Status CoreWorkerPlasmaStoreProvider::FetchAndGetFromPlasmaStore(
    absl::flat_hash_set<ObjectID> &remaining,
    const std::vector<ObjectID> &batch_ids,
    int64_t timeout_ms,
    bool fetch_only,
    bool in_direct_call,
    const TaskID &task_id,
    absl::flat_hash_map<ObjectID, std::shared_ptr<RayObject>> *results,
    bool *got_exception) {
  const auto owner_addresses = reference_counter_->GetOwnerAddresses(batch_ids);
  
  //hucc breakdown get_object
  // auto ts_breakdown_get_object = current_sys_time_us();
  // for (size_t i = 0; i < batch_ids.size(); i++) {
  //   const auto &object_id = batch_ids[i];
  //   RAY_LOG(WARNING) << "hucc breakdown get_object raylet: " << ts_breakdown_get_object << " object_id: "<< object_id << " task_id: " << task_id <<"\n";
  // }
  //hucc end 

  RAY_RETURN_NOT_OK(
      raylet_client_->FetchOrReconstruct(batch_ids,
                                         owner_addresses,
                                         fetch_only,
                                         /*mark_worker_blocked*/ !in_direct_call,
                                         task_id));

  std::vector<plasma::ObjectBuffer> plasma_results;
  RAY_RETURN_NOT_OK(store_client_.Get(batch_ids,
                                      timeout_ms,
                                      &plasma_results,
                                      /*is_from_worker=*/true));

  // Add successfully retrieved objects to the result map and remove them from
  // the set of IDs to get.
  for (size_t i = 0; i < plasma_results.size(); i++) {
    if (plasma_results[i].data != nullptr || plasma_results[i].metadata != nullptr) {
      const auto &object_id = batch_ids[i];
      std::shared_ptr<TrackedBuffer> data = nullptr;
      std::shared_ptr<Buffer> metadata = nullptr;
      if (plasma_results[i].data && plasma_results[i].data->Size()) {
        // We track the set of active data buffers in active_buffers_. On destruction,
        // the buffer entry will be removed from the set via callback.
        data = std::make_shared<TrackedBuffer>(
            plasma_results[i].data, buffer_tracker_, object_id);
        buffer_tracker_->Record(object_id, data.get(), get_current_call_site_());
      }
      if (plasma_results[i].metadata && plasma_results[i].metadata->Size()) {
        metadata = plasma_results[i].metadata;
      }
      const auto result_object = std::make_shared<RayObject>(
          data, metadata, std::vector<rpc::ObjectReference>());
      (*results)[object_id] = result_object;
      remaining.erase(object_id);
      if (result_object->IsException()) {
        RAY_CHECK(!result_object->IsInPlasmaError());
        *got_exception = true;
      }
    }
  }

  return Status::OK();
}


Status CoreWorkerPlasmaStoreProvider::FetchAndGetFromPlasmaStoreRDMA(
    absl::flat_hash_set<ObjectID> &remaining,
    const std::vector<ObjectID> &batch_ids,
    int64_t timeout_ms,
    bool fetch_only,
    bool in_direct_call,
    const TaskID &task_id,
    absl::flat_hash_map<ObjectID, std::shared_ptr<RayObject>> *results,
    bool *got_exception,
    const std::vector<unsigned long> &batch_virt_address,
    const std::vector<int> &batch_object_size,
    const std::vector<int> &batch_object_meta_size,
    const std::vector<ray::NodeID> &batch_owner_raylet_id,
    const std::vector<std::string> &batch_owner_ip_address,
    const std::vector<int> &batch_owner_port,
    const std::vector<ray::WorkerID> &batch_owner_worker_id,
    const std::vector<std::string> &batch_rem_ip_address) {
  const auto owner_addresses = reference_counter_->GetOwnerAddresses(batch_ids);
  
  auto t1 = current_sys_time_us();
  RAY_LOG(DEBUG) << " raylet client send " << t1 << " " << batch_ids[0];
  RAY_RETURN_NOT_OK(
      raylet_client_->FetchOrReconstructRDMA(batch_ids,
                                         owner_addresses,
                                         fetch_only,
                                         /*mark_worker_blocked*/ !in_direct_call,
                                         task_id,
                                         batch_virt_address,
                                         batch_object_size,
                                         batch_object_meta_size,
                                         batch_owner_raylet_id,
                                         batch_owner_ip_address,
                                         batch_owner_port,
                                         batch_owner_worker_id,
                                         batch_rem_ip_address));

  //hucc breakdown get_object
  auto t2 = current_sys_time_us();

  std::vector<plasma::ObjectBuffer> plasma_results;
  RAY_RETURN_NOT_OK(store_client_.Get(batch_ids,
                                      timeout_ms,
                                      &plasma_results,
                                      /*is_from_worker=*/true));
  auto t3 = current_sys_time_us();

  // Add successfully retrieved objects to the result map and remove them from
  // the set of IDs to get.
  for (size_t i = 0; i < plasma_results.size(); i++) {
    if (plasma_results[i].data != nullptr || plasma_results[i].metadata != nullptr) {
      const auto &object_id = batch_ids[i];
      std::shared_ptr<TrackedBuffer> data = nullptr;
      std::shared_ptr<Buffer> metadata = nullptr;
      if (plasma_results[i].data && plasma_results[i].data->Size()) {
        // We track the set of active data buffers in active_buffers_. On destruction,
        // the buffer entry will be removed from the set via callback.
        data = std::make_shared<TrackedBuffer>(
            plasma_results[i].data, buffer_tracker_, object_id);
        buffer_tracker_->Record(object_id, data.get(), get_current_call_site_());
      }
      if (plasma_results[i].metadata && plasma_results[i].metadata->Size()) {
        metadata = plasma_results[i].metadata;
        			// object info
        RAY_LOG(ERROR) << object_id <<  " " << object_id.Hash() << " " <<  metadata->Data();
        std::ofstream outfile1;
        outfile1.open("hutmp_" + std::to_string(object_id.Hash()) + ".txt");
        outfile1<<metadata->Data();
        // outfile1<<plasma_results[i].data->Size();
        // outfile1<<"\n";
        // outfile1<<plasma_results[i].metadata->Size();
        outfile1.close();
        
      }
      const auto result_object = std::make_shared<RayObject>(
          data, metadata, std::vector<rpc::ObjectReference>());
      (*results)[object_id] = result_object;
      remaining.erase(object_id);
      auto te_fetch_object_end = current_sys_time_us();
      RAY_LOG(DEBUG) << "plasma client fetch object id end: " << object_id << " " << te_fetch_object_end;
      if (result_object->IsException()) {
        RAY_CHECK(!result_object->IsInPlasmaError());
        *got_exception = true;
      }
    }
  }
  auto t4 = current_sys_time_us();

  RAY_LOG(DEBUG) << "FetchAndGetFromPlasmaStore: " << t4 - t3 << " " << t3 - t2 << " " << t2-t1 << " " <<batch_ids[0];

  return Status::OK();
}

Status CoreWorkerPlasmaStoreProvider::GetIfLocal(
    const std::vector<ObjectID> &object_ids,
    absl::flat_hash_map<ObjectID, std::shared_ptr<RayObject>> *results) {
  std::vector<plasma::ObjectBuffer> plasma_results;
  // Since this path is used only for spilling, we should set is_from_worker: false.
  RAY_RETURN_NOT_OK(store_client_.Get(object_ids,
                                      /*timeout_ms=*/0,
                                      &plasma_results,
                                      /*is_from_worker=*/false));

  for (size_t i = 0; i < object_ids.size(); i++) {
    if (plasma_results[i].data != nullptr || plasma_results[i].metadata != nullptr) {
      const auto &object_id = object_ids[i];
      std::shared_ptr<TrackedBuffer> data = nullptr;
      std::shared_ptr<Buffer> metadata = nullptr;
      if (plasma_results[i].data && plasma_results[i].data->Size()) {
        // We track the set of active data buffers in active_buffers_. On destruction,
        // the buffer entry will be removed from the set via callback.
        data = std::make_shared<TrackedBuffer>(
            plasma_results[i].data, buffer_tracker_, object_id);
        buffer_tracker_->Record(object_id, data.get(), get_current_call_site_());
      }
      if (plasma_results[i].metadata && plasma_results[i].metadata->Size()) {
        metadata = plasma_results[i].metadata;
      }
      const auto result_object = std::make_shared<RayObject>(
          data, metadata, std::vector<rpc::ObjectReference>());
      (*results)[object_id] = result_object;
    }
  }
  return Status::OK();
}

Status CoreWorkerPlasmaStoreProvider::GetObjectMetaFromPlasma(const ObjectID &object_id, unsigned long *address, int64_t *object_size, int *device_num, ray::ObjectInfo *object_info) {
  RAY_RETURN_NOT_OK(store_client_.GetObjectMeta(object_id, address, object_size, device_num, object_info));
  return Status::OK();
}

Status UnblockIfNeeded(const std::shared_ptr<raylet::RayletClient> &client,
                       const WorkerContext &ctx) {
  if (ctx.CurrentTaskIsDirectCall()) {
    // NOTE: for direct call actors, we still need to issue an unblock IPC to release
    // get subscriptions, even if the worker isn't blocked.
    if (ctx.ShouldReleaseResourcesOnBlockingCalls() || ctx.CurrentActorIsDirectCall()) {
      return client->NotifyDirectCallTaskUnblocked();
    } else {
      return Status::OK();  // We don't need to release resources.
    }
  } else {
    return client->NotifyUnblocked(ctx.GetCurrentTaskID());
  }
}


Status CoreWorkerPlasmaStoreProvider::Get(
    const absl::flat_hash_set<ObjectID> &object_ids,
    int64_t timeout_ms,
    const WorkerContext &ctx,
    absl::flat_hash_map<ObjectID, std::shared_ptr<RayObject>> *results,
    bool *got_exception) {
  int64_t batch_size = RayConfig::instance().worker_fetch_request_size();
  //hucc time for get obj from local plasma
  auto t1_out = current_sys_time_us();


  std::vector<ObjectID> batch_ids;
  absl::flat_hash_set<ObjectID> remaining(object_ids.begin(), object_ids.end());

  // First, attempt to fetch all of the required objects once without reconstructing.
  std::vector<ObjectID> id_vector(object_ids.begin(), object_ids.end());
  // RAY_LOG(ERROR) << "ref object get start ";

  int64_t total_size = static_cast<int64_t>(object_ids.size());
  //hucc time for get obj from local plasma
  auto ts_get_obj_local_plasma = current_sys_time_us();
  
  for (int64_t start = 0; start < total_size; start += batch_size) {
    batch_ids.clear();
    for (int64_t i = start; i < batch_size && i < total_size; i++) {
      batch_ids.push_back(id_vector[start + i]);
    }
    RAY_RETURN_NOT_OK(FetchAndGetFromPlasmaStore(remaining,
                                                 batch_ids,
                                                 /*timeout_ms=*/0,
                                                 /*fetch_only=*/true,
                                                 ctx.CurrentTaskIsDirectCall(),
                                                 ctx.GetCurrentTaskID(),
                                                 results,
                                                 got_exception));
  }
  auto t2_out = current_sys_time_us();
  // auto te_get_obj_local_plasma = current_sys_time_us();
  // RAY_LOG(WARNING) << "hucc time for get obj from local plasma total time: " << te_get_obj_local_plasma << "," << ts_get_obj_local_plasma << " empty: " << remaining.empty() << "\n";
  // If all objects were fetched already, return. Note that we always need to
  // call UnblockIfNeeded() to cancel the get request.
  if (remaining.empty() || *got_exception) {
    return UnblockIfNeeded(raylet_client_, ctx);
  }
  auto t3_out = current_sys_time_us();

  // If not all objects were successfully fetched, repeatedly call FetchOrReconstruct
  // and Get from the local object store in batches. This loop will run indefinitely
  // until the objects are all fetched if timeout is -1.
  bool should_break = false;
  bool timed_out = false;
  int64_t remaining_timeout = timeout_ms;
  auto fetch_start_time_ms = current_time_ms();
  while (!remaining.empty() && !should_break) {
    auto t1 = current_sys_time_us();
    batch_ids.clear();
    for (const auto &id : remaining) {
      if (int64_t(batch_ids.size()) == batch_size) {
        break;
      }
      batch_ids.push_back(id);
    }

    int64_t batch_timeout = std::max(RayConfig::instance().get_timeout_milliseconds(),
                                     int64_t(10 * batch_ids.size()));
    if (remaining_timeout >= 0) {
      batch_timeout = std::min(remaining_timeout, batch_timeout);
      remaining_timeout -= batch_timeout;
      timed_out = remaining_timeout <= 0;
    }
    auto t2 = current_sys_time_us();

    size_t previous_size = remaining.size();
    // This is a separate IPC from the FetchAndGet in direct call mode.
    if (ctx.CurrentTaskIsDirectCall() && ctx.ShouldReleaseResourcesOnBlockingCalls()) {
      RAY_RETURN_NOT_OK(raylet_client_->NotifyDirectCallTaskBlocked(
          /*release_resources_during_plasma_fetch=*/false));
    }
    auto t3 = current_sys_time_us();
    RAY_LOG(DEBUG) << "CurrentTaskIsDirectCall " << t3 -t2 << " " << ctx.CurrentTaskIsDirectCall() << " " << ctx.ShouldReleaseResourcesOnBlockingCalls();
    //hucc time for get obj from remote plasma
    auto ts_get_obj_remote_plasma = current_sys_time_us();
    RAY_RETURN_NOT_OK(FetchAndGetFromPlasmaStore(remaining,
                                                 batch_ids,
                                                 batch_timeout,
                                                 /*fetch_only=*/false,
                                                 ctx.CurrentTaskIsDirectCall(),
                                                 ctx.GetCurrentTaskID(),
                                                 results,
                                                 got_exception));
    should_break = timed_out || *got_exception;
    auto ts_get_obj_remote_plasma_median = current_sys_time_us();

    if ((previous_size - remaining.size()) < batch_ids.size()) {
      WarnIfFetchHanging(fetch_start_time_ms, remaining);
    }
    //hucc time for get obj from remote plasma
    auto te_get_obj_remote_plasma = current_sys_time_us();
    RAY_LOG(DEBUG) << "hucc time for get obj from local plasma total time in while: " << te_get_obj_remote_plasma - ts_get_obj_remote_plasma << " " << te_get_obj_remote_plasma - ts_get_obj_remote_plasma_median << " empty: " << remaining.empty() << "\n";
    if (check_signals_) {
      Status status = check_signals_();
      if (!status.ok()) {
        // TODO(edoakes): in this case which status should we return?
        RAY_RETURN_NOT_OK(UnblockIfNeeded(raylet_client_, ctx));
        return status;
      }
    }
    auto t4 = current_sys_time_us();

    if (RayConfig::instance().yield_plasma_lock_workaround() && !should_break &&
        remaining.size() > 0) {
      // Yield the plasma lock to other threads. This is a temporary workaround since we
      // are holding the lock for a long time, so it can easily starve inbound RPC
      // requests to Release() buffers which only require holding the lock for brief
      // periods. See https://github.com/ray-project/ray/pull/16402 for more context.
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    auto t5 = current_sys_time_us();
    RAY_LOG(DEBUG) << "hucc time for break while remain "  << t2-t1  << " , " << t3-t2  << " , " <<  t4-t3  << " , "  << t5-t4;

  }

  if (!remaining.empty() && timed_out) {
    RAY_RETURN_NOT_OK(UnblockIfNeeded(raylet_client_, ctx));
    return Status::TimedOut("Get timed out: some object(s) not ready.");
  }
  auto t4_out = current_sys_time_us();

  Status status  = UnblockIfNeeded(raylet_client_, ctx);

  auto end_out = current_sys_time_us();
  RAY_LOG(DEBUG) << "get object once time "  << end_out - t4_out << " " << t4_out - t3_out << " " << t3_out - t2_out << " " << t2_out - t1_out;

  return status;
  // Notify unblocked because we blocked when calling FetchOrReconstruct with
  // fetch_only=false.
  // return UnblockIfNeeded(raylet_client_, ctx);
}



Status CoreWorkerPlasmaStoreProvider::GetRDMA(
    const absl::flat_hash_set<ObjectID> &object_ids,
    int64_t timeout_ms,
    const WorkerContext &ctx,
    absl::flat_hash_map<ObjectID, std::shared_ptr<RayObject>> *results,
    bool *got_exception, 
    absl::flat_hash_map<ObjectID, std::pair<std::pair<unsigned long, std::string>, ray::ObjectInfo>> &plasma_node_virt_info_) {
  int64_t batch_size = RayConfig::instance().worker_fetch_request_size();
  //hucc time for get obj from local plasma

  auto t1_out = current_sys_time_us();

  // auto ts_get_obj_local_plasma = current_sys_time_us();
  std::vector<ObjectID> batch_ids;
  std::vector<unsigned long> batch_virt_address;
  std::vector<int> batch_object_size;
  std::vector<int> batch_object_meta_size;

  std::vector<ray::NodeID> batch_owner_raylet_id;
  std::vector<std::string> batch_owner_ip_address;
  std::vector<int> batch_owner_port;
  std::vector<ray::WorkerID> batch_owner_worker_id;
  
  std::vector<std::string> batch_rem_ip_address;

  absl::flat_hash_set<ObjectID> remaining(object_ids.begin(), object_ids.end());

  // First, attempt to fetch all of the required objects once without reconstructing.
  std::vector<ObjectID> id_vector(object_ids.begin(), object_ids.end());
  // RAY_LOG(ERROR) << " object get start " << id_vector[0];

  int64_t total_size = static_cast<int64_t>(object_ids.size());
  
  std::vector<unsigned long> virt_address_vector;
  std::vector<int> object_size_vector;
  std::vector<int> object_meta_size_vector;

  std::vector<ray::NodeID> owner_raylet_id_vector;
  std::vector<std::string> owner_ip_address_vector;
  std::vector<int> owner_port_vector;
  std::vector<ray::WorkerID> owner_worker_id_vector;
  
  std::vector<std::string> rem_ip_address_vector;
  // absl::flat_hash_set<ObjectID> waiting_info;

  // RAY_LOG(ERROR) << " object info time after find ";
  absl::flat_hash_set<ObjectID> wait_info;
  for (auto entry: id_vector) {
    auto it = plasma_node_virt_info_.find(entry);
    if (it == plasma_node_virt_info_.end()) {
      // RAY_LOG(ERROR)<<"not found " << entry;
      wait_info.insert(entry);
      remaining.erase(entry);
      continue;
    }
    virt_address_vector.push_back(it->second.first.first);
    object_size_vector.push_back(it->second.second.data_size);
    object_meta_size_vector.push_back(it->second.second.metadata_size);
    owner_raylet_id_vector.push_back(it->second.second.owner_raylet_id);
    owner_ip_address_vector.push_back(it->second.second.owner_ip_address);
    owner_port_vector.push_back(it->second.second.owner_port);
    owner_worker_id_vector.push_back(it->second.second.owner_worker_id);
    rem_ip_address_vector.push_back(it->second.first.second);

  }
  // RAY_LOG(ERROR) << " object info time after find 1";
  if(!remaining.empty()) {
  for (int64_t start = 0; start < total_size; start += batch_size) {
    batch_ids.clear();
    batch_virt_address.clear();
    batch_object_size.clear();
    batch_object_meta_size.clear();
    batch_owner_raylet_id.clear();
    batch_owner_ip_address.clear();
    batch_owner_port.clear();
    batch_owner_worker_id.clear();
    batch_rem_ip_address.clear();

    for (int64_t i = start; i < batch_size && i < total_size; i++) {
      batch_ids.push_back(id_vector[start + i]);
      batch_virt_address.push_back(virt_address_vector[start + i]);
      batch_object_size.push_back(object_size_vector[start + i]);
      batch_object_meta_size.push_back(object_meta_size_vector[start + i]);

      batch_owner_raylet_id.push_back(owner_raylet_id_vector[start + i]);
      batch_owner_ip_address.push_back(owner_ip_address_vector[start + i]);
      batch_owner_port.push_back(owner_port_vector[start + i]);
      batch_owner_worker_id.push_back(owner_worker_id_vector[start + i]);
      batch_rem_ip_address.push_back(rem_ip_address_vector[start + i]);

    }
    // auto ts_fetch_plasma = current_sys_time_us();
    // RAY_LOG(DEBUG) << " first fetch and get plasma 1 " << batch_ids[0] << " " << ts_fetch_plasma;

    RAY_RETURN_NOT_OK(FetchAndGetFromPlasmaStoreRDMA(remaining,
                                                 batch_ids,
                                                 /*timeout_ms=*/0,
                                                 /*fetch_only=*/true,
                                                 ctx.CurrentTaskIsDirectCall(),
                                                 ctx.GetCurrentTaskID(),
                                                 results,
                                                 got_exception, 
                                                 batch_virt_address,
                                                 batch_object_size,
                                                 batch_object_meta_size,
                                                 batch_owner_raylet_id,
                                                 batch_owner_ip_address,
                                                 batch_owner_port,
                                                 batch_owner_worker_id,
                                                 batch_rem_ip_address));
  }
  }
  auto t2_out = current_sys_time_us();
  // RAY_LOG(DEBUG) << " first fetch and get plasma 2 " << id_vector[0] << " " << t2_out;

  for (auto entry: wait_info) {
    remaining.insert(entry);
  }
  wait_info.clear();
  // auto te_get_obj_local_plasma = current_sys_time_us();
  // RAY_LOG(WARNING) << "hucc time for get obj from local plasma total time: " << te_get_obj_local_plasma - ts_get_obj_local_plasma << " empty: " << remaining.empty() << "\n";
  // If all objects were fetched already, return. Note that we always need to
  // call UnblockIfNeeded() to cancel the get request.
  if (remaining.empty() || *got_exception) {
    return UnblockIfNeeded(raylet_client_, ctx);
  }

  auto t3_out = current_sys_time_us();
  // RAY_LOG(DEBUG) << " first fetch and get plasma 3 " << id_vector[0] << " " << t3_out;

  // If not all objects were successfully fetched, repeatedly call FetchOrReconstruct
  // and Get from the local object store in batches. This loop will run indefinitely
  // until the objects are all fetched if timeout is -1.
  bool should_break = false;
  bool timed_out = false;
  int64_t remaining_timeout = timeout_ms;
  auto fetch_start_time_ms = current_time_ms();
  // RAY_LOG(ERROR) << " object info time after find 2";


  while (!remaining.empty() && !should_break) {
    auto t1 = current_sys_time_us();
    batch_ids.clear();
    batch_virt_address.clear();
    batch_object_size.clear();
    batch_object_meta_size.clear();
    batch_owner_raylet_id.clear();
    batch_owner_ip_address.clear();
    batch_owner_port.clear();
    batch_owner_worker_id.clear();
    batch_rem_ip_address.clear();
    // RAY_LOG(ERROR) << " object info time after find 3";

    for (const auto &id : remaining) {
      if (int64_t(batch_ids.size()) == batch_size) {
        break;
      }
      auto it = plasma_node_virt_info_.find(id);
      if (it == plasma_node_virt_info_.end()) {
        // RAY_LOG(ERROR)<<"not found " << id;
        wait_info.insert(id);
        remaining.erase(id);
        continue;
      }
      batch_ids.push_back(id);
      batch_virt_address.push_back(it->second.first.first);
      batch_object_size.push_back(it->second.second.data_size);
      batch_object_meta_size.push_back(it->second.second.metadata_size);
      batch_owner_raylet_id.push_back(it->second.second.owner_raylet_id);
      batch_owner_ip_address.push_back(it->second.second.owner_ip_address);
      batch_owner_port.push_back(it->second.second.owner_port);
      batch_owner_worker_id.push_back(it->second.second.owner_worker_id);
      batch_rem_ip_address.push_back(it->second.first.second);

    }
    // RAY_LOG(ERROR) << " object info time after find 4";
    // if (remaining.empty()){
    //   continue;
    // }
    int64_t batch_timeout = std::max(RayConfig::instance().get_timeout_milliseconds(),
                                     int64_t(10 * batch_ids.size()));
    if (remaining_timeout >= 0) {
      batch_timeout = std::min(remaining_timeout, batch_timeout);
      remaining_timeout -= batch_timeout;
      timed_out = remaining_timeout <= 0;
    }
    auto t2 = current_sys_time_us();

    size_t previous_size = remaining.size();
    // This is a separate IPC from the FetchAndGet in direct call mode.
    // if (ctx.CurrentTaskIsDirectCall() && ctx.ShouldReleaseResourcesOnBlockingCalls()) {
    //   RAY_RETURN_NOT_OK(raylet_client_->NotifyDirectCallTaskBlocked(
    //       /*release_resources_during_plasma_fetch=*/false));
    // }
    auto t3 = current_sys_time_us();
    RAY_LOG(DEBUG) << "CurrentTaskIsDirectCall " << t3 -t2 << " " << ctx.CurrentTaskIsDirectCall() << " " << ctx.ShouldReleaseResourcesOnBlockingCalls();

    //hucc time for get obj from remote plasma
    auto ts_get_obj_remote_plasma = current_sys_time_us();
    // RAY_LOG(WARNING) << "CoreWorkerPlasmaStoreProvider Get remaining empty" << remaining.empty() << " should_break " << should_break;
    if(!remaining.empty()) {
    RAY_RETURN_NOT_OK(FetchAndGetFromPlasmaStoreRDMA(remaining,
                                                 batch_ids,
                                                 10,
                                                 /*fetch_only=*/false,
                                                 ctx.CurrentTaskIsDirectCall(),
                                                 ctx.GetCurrentTaskID(),
                                                 results,
                                                 got_exception,
                                                 batch_virt_address,
                                                 batch_object_size,
                                                 batch_object_meta_size,
                                                 batch_owner_raylet_id,
                                                 batch_owner_ip_address,
                                                 batch_owner_port,
                                                 batch_owner_worker_id,
                                                 batch_rem_ip_address));

    should_break = timed_out || *got_exception;
    }
    auto ts_get_obj_remote_plasma_median = current_sys_time_us();

    for (auto entry: wait_info) {
      remaining.insert(entry);
    }
    wait_info.clear();
    if ((previous_size - remaining.size()) < batch_ids.size()) {
      WarnIfFetchHanging(fetch_start_time_ms, remaining);
    }
    //hucc time for get obj from remote plasma
    auto te_get_obj_remote_plasma = current_sys_time_us();
    RAY_LOG(DEBUG) << "hucc time for get obj from local plasma total time in while: " << te_get_obj_remote_plasma - ts_get_obj_remote_plasma << " " << te_get_obj_remote_plasma - ts_get_obj_remote_plasma_median << " empty: " << remaining.empty() << "\n";

    if (check_signals_) {
      Status status = check_signals_();
      if (!status.ok()) {
        // TODO(edoakes): in this case which status should we return?
        // RAY_RETURN_NOT_OK(UnblockIfNeeded(raylet_client_, ctx));
        return status;
      }
    }
    auto t4 = current_sys_time_us();

    if (RayConfig::instance().yield_plasma_lock_workaround() && !should_break &&
        remaining.size() > 0) {
      // Yield the plasma lock to other threads. This is a temporary workaround since we
      // are holding the lock for a long time, so it can easily starve inbound RPC
      // requests to Release() buffers which only require holding the lock for brief
      // periods. See https://github.com/ray-project/ray/pull/16402 for more context.
      // std::this_thread::sleep_for(std::chrono::milliseconds(10));
      std::this_thread::sleep_for(std::chrono::microseconds(100));

    }
    auto t5 = current_sys_time_us();
    RAY_LOG(DEBUG) << "hucc time for break while remain "  << t2-t1  << " , " << t3-t2  << " , " <<  t4-t3  << " , "  << t5-t4;

  }

  if (!remaining.empty() && timed_out) {
    RAY_RETURN_NOT_OK(UnblockIfNeeded(raylet_client_, ctx));
    return Status::TimedOut("Get timed out: some object(s) not ready.");
  }
  auto end_out = current_sys_time_us();
  RAY_LOG(DEBUG) << "get object once time "  << end_out - t3_out << " " << t3_out - t2_out << " " << t2_out - t1_out;

  // Notify unblocked because we blocked when calling FetchOrReconstruct with
  // fetch_only=false.
  Status status = UnblockIfNeeded(raylet_client_, ctx);
  // return UnblockIfNeeded(raylet_client_, ctx);
  return status;  // We don't need to release resources.

}

Status CoreWorkerPlasmaStoreProvider::Contains(const ObjectID &object_id,
                                               bool *has_object) {
  return store_client_.Contains(object_id, has_object);
}

Status CoreWorkerPlasmaStoreProvider::Wait(
    const absl::flat_hash_set<ObjectID> &object_ids,
    int num_objects,
    int64_t timeout_ms,
    const WorkerContext &ctx,
    absl::flat_hash_set<ObjectID> *ready) {
  std::vector<ObjectID> id_vector(object_ids.begin(), object_ids.end());

  bool should_break = false;
  int64_t remaining_timeout = timeout_ms;
  while (!should_break) {
    WaitResultPair result_pair;
    int64_t call_timeout = RayConfig::instance().get_timeout_milliseconds();
    if (remaining_timeout >= 0) {
      call_timeout = std::min(remaining_timeout, call_timeout);
      remaining_timeout -= call_timeout;
      should_break = remaining_timeout <= 0;
    }

    // This is a separate IPC from the Wait in direct call mode.
    if (ctx.CurrentTaskIsDirectCall() && ctx.ShouldReleaseResourcesOnBlockingCalls()) {
      RAY_RETURN_NOT_OK(raylet_client_->NotifyDirectCallTaskBlocked(
          /*release_resources_during_plasma_fetch=*/false));
    }
    const auto owner_addresses = reference_counter_->GetOwnerAddresses(id_vector);
    RAY_RETURN_NOT_OK(
        raylet_client_->Wait(id_vector,
                             owner_addresses,
                             num_objects,
                             call_timeout,
                             /*mark_worker_blocked*/ !ctx.CurrentTaskIsDirectCall(),
                             ctx.GetCurrentTaskID(),
                             &result_pair));

    if (result_pair.first.size() >= static_cast<size_t>(num_objects)) {
      should_break = true;
    }
    for (const auto &entry : result_pair.first) {
      ready->insert(entry);
    }
    if (check_signals_) {
      RAY_RETURN_NOT_OK(check_signals_());
    }
  }
  if (ctx.CurrentTaskIsDirectCall() && ctx.ShouldReleaseResourcesOnBlockingCalls()) {
    RAY_RETURN_NOT_OK(raylet_client_->NotifyDirectCallTaskUnblocked());
  }
  return Status::OK();
}

Status CoreWorkerPlasmaStoreProvider::Delete(
    const absl::flat_hash_set<ObjectID> &object_ids, bool local_only) {
  std::vector<ObjectID> object_id_vector(object_ids.begin(), object_ids.end());
  return raylet_client_->FreeObjects(object_id_vector, local_only);
}

std::string CoreWorkerPlasmaStoreProvider::MemoryUsageString() {
  return store_client_.DebugString();
}

absl::flat_hash_map<ObjectID, std::pair<int64_t, std::string>>
CoreWorkerPlasmaStoreProvider::UsedObjectsList() const {
  return buffer_tracker_->UsedObjects();
}

void CoreWorkerPlasmaStoreProvider::WarnIfFetchHanging(
    int64_t fetch_start_time_ms, const absl::flat_hash_set<ObjectID> &remaining) {
  int64_t duration_ms = current_time_ms() - fetch_start_time_ms;
  if (duration_ms > RayConfig::instance().fetch_warn_timeout_milliseconds()) {
    std::ostringstream oss;
    size_t printed = 0;
    for (auto &id : remaining) {
      if (printed >=
          RayConfig::instance().object_store_get_max_ids_to_print_in_warning()) {
        break;
      }
      if (printed > 0) {
        oss << ", ";
      }
      oss << id.Hex();
      printed++;
    }
    if (printed < remaining.size()) {
      oss << ", etc";
    }
    RAY_LOG(WARNING)
        << "Objects " << oss.str() << " are still not local after "
        << (duration_ms / 1000) << "s. "
        << "If this message continues to print, ray.get() is likely hung. Please file an "
           "issue at https://github.com/ray-project/ray/issues/.";
  }
}

Status CoreWorkerPlasmaStoreProvider::WarmupStore() {
  ObjectID object_id = ObjectID::FromRandom();
  std::shared_ptr<Buffer> data;
  RAY_RETURN_NOT_OK(Create(nullptr,
                           8,
                           object_id,
                           rpc::Address(),
                           &data,
                           /*created_by_worker=*/true));
  RAY_RETURN_NOT_OK(Seal(object_id));
  RAY_RETURN_NOT_OK(Release(object_id));
  RAY_RETURN_NOT_OK(Delete({object_id}, true));
  return Status::OK();
}

}  // namespace core
}  // namespace ray
