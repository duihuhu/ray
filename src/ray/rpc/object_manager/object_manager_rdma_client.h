#pragma once
#include <grpcpp/grpcpp.h>
#include <grpcpp/resource_quota.h>
#include <grpcpp/support/channel_arguments.h>

#include <thread>

#include "ray/common/status.h"
#include "ray/rpc/grpc_client.h"
#include "ray/util/logging.h"
#include "src/ray/protobuf/object_manager_rdma.grpc.pb.h"
#include "src/ray/protobuf/object_manager_rdma.pb.h"
namespace ray {
namespace rpc {

class ObjectManagerRdmaClient {
  public:

    ObjectManagerRdmaClient(const std::string &address,
                            const int port,
                            ClientCallManager &client_call_manager,
                            int num_connections = 4)
        : num_connections_(num_connections) {
      
      getobject_rr_index = rand() % num_connections_;
      grpc_clients_.reserve(num_connections_);
      for (int i = 0; i < num_connections_; i++) {
        grpc_clients_.emplace_back(new GrpcClient<ObjectManagerRdmaService>(
            address, port, client_call_manager, num_connections_));
      }
    };

    /// notfiy get object to  object manager
    ///
    /// \param request The request message.
    /// \param callback The callback function that handles reply from server
    VOID_RPC_CLIENT_METHOD(ObjectManagerRdmaService,
                           GetObject,
                           grpc_clients_[getobject_rr_index++ % num_connections_],
                           -1,)
  private:
    int num_connections_;

    std::atomic<unsigned int> getobject_rr_index;
    /// The RPC clients.
    std::vector<std::unique_ptr<GrpcClient<ObjectManagerRdmaService>>> grpc_clients_;
};


}  // namespace rpc
}  // namespace ray