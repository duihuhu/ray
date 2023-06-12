// rdma server for handle object get request

#pragma once
#include "ray/common/asio/instrumented_io_context.h"
#include "ray/rpc/grpc_server.h"
#include "ray/rpc/server_call.h"
#include "src/ray/protobuf/object_manager_rdma.grpc.pb.h"
#include "src/ray/protobuf/object_manager_rdma.pb.h"


namespace ray {
namespace rpc {

#define RAY_OBJECT_MANAGER_RDMA_RPC_HANDLERS \
  RPC_SERVICE_HANDLER(ObjectManagerRdmaService, GetObject, -1)



class ObjectManagerRdmaServiceHandler {
  public:

    virtual void HandleGetObject(const GetObjectRequest &request,
                                 GetObjectReply *reply,
                                 SendReplyCallback send_reply_callback) = 0;
};

class ObjectManagerRdmaGrpcService : public GrpcService {
  public:
    /// Construct a `ObjectManagerRdmaGrpcService`.
    ///
    /// \param[in] port See `GrpcService`.
    /// \param[in] handler The service handler that actually handle the requests.
    ObjectManagerRdmaService(instrumented_io_context &io_service,
                            ObjectManagerRdmaServiceHandler &service_handler)
        : GprcService(io_serivce), service_hander_(service_handler){};
  
  protected:
    grpc::Service &GetGrpcService() override {return service_; }

    void InitServerCallFactories(
        const std::unique_ptr<grpc::ServerCompletionQueue> &cq,
        std::vector<std::unique_ptr<ServerCallFactory>> *server_call_factories) override {
      RAY_OBJECT_MANAGER_RDMA_RPC_HANDLERS
    }
  
  private:
    /// The grpc async service object.
    ObjectManagerRdmaService::AsyncService service_;
    /// The service handler that actually handle the requests.
    ObjectManagerRdmaServiceHandler &service_hander_;
};



}  // namespace rpc
}  // namespace ray