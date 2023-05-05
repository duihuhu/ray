// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include "ray/object_manager/plasma/object_store.h"

namespace plasma {

ObjectStore::ObjectStore(IAllocator &allocator)
    : allocator_(allocator), object_table_() {}

const LocalObject *ObjectStore::CreateObject(const ray::ObjectInfo &object_info,
                                             plasma::flatbuf::ObjectSource source,
                                             bool fallback_allocate, bool rdma) {
                                              
  RAY_LOG(DEBUG) << "attempting to create object " << object_info.object_id << " size "
                 << object_info.data_size;
  // RAY_LOG(WARNING) << "attempting to create object " << object_info.object_id << " size "
  //               << object_info.data_size;
  RAY_CHECK(object_table_.count(object_info.object_id) == 0)
      << object_info.object_id << " already exists!";

  // if (object_table_rdma_.find(object_info.object_id) == object_table_rdma_.end() ){
  //   return nullptr;
  // }
  auto object_size = object_info.GetObjectSize();
  auto allocation = fallback_allocate ? allocator_.FallbackAllocate(object_size)
                                      : allocator_.Allocate(object_size);
  RAY_LOG_EVERY_MS(INFO, 10 * 60 * 1000)
      << "Object store current usage " << (allocator_.Allocated() / 1e9) << " / "
      << (allocator_.GetFootprintLimit() / 1e9) << " GB.";
  if (!allocation.has_value()) {
    return nullptr;
  }
  if (rdma == true) {
    LocalObject *ptr = new LocalObject(allocation.value());
    RAY_LOG(DEBUG) << "create object " << object_info.object_id << " succeeded" << " address " << ptr->GetAllocation().address ;
    ptr->object_info = object_info;
    ptr->state = ObjectState::PLASMA_CREATED;
    ptr->create_time = std::time(nullptr);
    ptr->construct_duration = -1;
    ptr->source = source;
    object_table_rdma_.insert(object_info.object_id);
    return ptr;
  }
  auto ptr = std::make_unique<LocalObject>(std::move(allocation.value()));
  auto entry =
      object_table_.emplace(object_info.object_id, std::move(ptr)).first->second.get();
  entry->object_info = object_info;
  entry->state = ObjectState::PLASMA_CREATED;
  entry->create_time = std::time(nullptr);
  entry->construct_duration = -1;
  entry->source = source;

  RAY_LOG(DEBUG) << "create object " << object_info.object_id << " succeeded";
  return entry;
}

const LocalObject *ObjectStore::GetObject(const ObjectID &object_id) const {
  RAY_LOG(DEBUG) << "ObjectStore GetObject " << object_id;
  auto it = object_table_.find(object_id);
  if (it == object_table_.end()) {
    RAY_LOG(DEBUG) << "ObjectStore GetObject nullptr " << object_id ;
    return nullptr;
  }
  return it->second.get();
}

bool ObjectStore::GetObjectExist(const ObjectID &object_id) {
  auto it = object_table_rdma_.find(object_id);
  if (it == object_table_rdma_.end())
    return false;
  return true;
}

const LocalObject *ObjectStore::SealObject(const ObjectID &object_id) {
  auto entry = GetMutableObject(object_id);
  if (entry == nullptr || entry->state == ObjectState::PLASMA_SEALED) {
    return nullptr;
  }
  entry->state = ObjectState::PLASMA_SEALED;
  entry->construct_duration = std::time(nullptr) - entry->create_time;
  return entry;
}

bool ObjectStore::DeleteObject(const ObjectID &object_id) {
  auto entry = GetMutableObject(object_id);
  if (entry == nullptr) {
    return false;
  }
  allocator_.Free(std::move(entry->allocation));
  object_table_.erase(object_id);
  object_table_rdma_.erase(object_id);
  return true;
}

LocalObject *ObjectStore::GetMutableObject(const ObjectID &object_id) {
  auto it = object_table_.find(object_id);
  if (it == object_table_.end()) {
    return nullptr;
  }
  return it->second.get();
}

absl::flat_hash_map<ObjectID, std::unique_ptr<LocalObject>>  *ObjectStore::GetPlasmaMeta() {
  // RAY_LOG(WARNING) << "hucc get plasma meta start: " << "\n";
  // for (auto &entry : object_table_) {
  //   ObjectID object_id = entry.first;
  //   const Allocation &allocation = entry.second->GetAllocation();
  //   RAY_LOG(WARNING) << "hucc get plasma meta object id " << object_id << "allocation information: " << allocation.address << "endl";
  // }
  // std::cout<< "object_table_ address " << &object_table_ <<std::endl;

  return &object_table_;
}

void ObjectStore::InsertObjectInfo(const absl::optional<Allocation>& allocation , const ray::ObjectInfo &object_info) {
  auto source = plasma::flatbuf::ObjectSource::ReceivedFromRemoteRaylet;
  auto it = object_table_.find(object_info.object_id);
  if(it!=object_table_.end())
    return;
  RAY_CHECK(object_table_.count(object_info.object_id) == 0)
      << object_info.object_id << " already exists!";
  RAY_LOG(DEBUG) << "InsertObjectInfo " << object_info.object_id;
  auto ptr = std::make_unique<LocalObject>(std::move(allocation.value()));
  auto entry =
      object_table_.emplace(object_info.object_id, std::move(ptr)).first->second.get();
  entry->object_info = object_info;
  entry->state = ObjectState::PLASMA_SEALED;
  entry->create_time = std::time(nullptr);
  entry->construct_duration = -1;
  entry->source = source;
}

void ObjectStore::InsertObjectInfoThread(const Allocation& allocation , const ray::ObjectInfo &object_info, const std::pair<const plasma::LocalObject *, plasma::flatbuf::PlasmaError>& pair) {
  auto it = object_table_.find(object_info.object_id);
  if(it!=object_table_.end())
    return;
  RAY_LOG(DEBUG) << "InsertObjectInfo " << object_info.object_id;
  auto ptr = std::make_unique<LocalObject>(std::move(pair.first->GetAllocation()));
  
  auto entry = object_table_.emplace(object_info.object_id, std::move(ptr)).first->second.get();
  entry->state = ObjectState::PLASMA_SEALED;
  RAY_LOG(DEBUG) << "InsertObjectInfoThread object_table " << entry->object_info.GetObjectSize();
}

}  // namespace plasma
