#pragma once
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <sys/time.h>
// #include <netdb.h>
// #include <malloc.h>
// #include <getopt.h>
// #include <arpa/inet.h>
// #include <time.h>
// #include <inttypes.h>
// #include <stdbool.h>
#include <infiniband/verbs.h>
#include <boost/asio.hpp>
#include <boost/asio/error.hpp>
#include <boost/bind/bind.hpp>
#include "ray/common/asio/instrumented_io_context.h"
#include "ray/gcs/gcs_client/gcs_client.h"
#include "ray/common/id.h"
#include "ray/object_manager/object_manager.h"
#include <fstream>
#include <string>
#include <random>
#include "src/ray/raylet/dependency_manager.h"
#include <thread>

#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable
#include "ray/object_manager/concurrentqueue.h"
#include "ray/rpc/object_manager_rdma/object_manager_rdma_client.h"
#include "ray/rpc/object_manager_rdma/object_manager_rdma_server.h"


// #include <stddef.h>
// #include <cstdlib>

// #include "ray/common/common_protocol.h"
// #include "ray/stats/metric_defs.h"
// #include "ray/util/util.h"


#define num_qp_pair 8
struct ObjectRdmaInfo {
    std::string object_address;
    ray::ObjectInfo object_info;
    unsigned long object_virt_address;
    int object_sizes;
    std::string rem_ip_address;
};

struct Config {
	uint32_t	port;
	char	*ib_devname;
  int ib_port;
  int		size;
  enum ibv_mtu mtu;
	unsigned int rx_depth;
	unsigned int iters;
  int sl;
  int gidx;
	int		num_threads;
	// char	*op_type;
  char	*server_name;
  int use_event;
};

struct pingpong_dest {
	int lid;
	int qpn;
	int psn;
	union ibv_gid gid;
  uint32_t rkey;
};


struct pingpong_context {
	struct ibv_context	*context;
	struct ibv_comp_channel *channel;
	struct ibv_pd		*pd;
	struct ibv_mr		*mr;
	struct ibv_dm		*dm;
	union {
		struct ibv_cq		*cq;
		struct ibv_cq_ex	*cq_ex;
	} cq_s;
	struct ibv_qp		*qp;
	struct ibv_qp_ex	*qpx;
	void			*buf;
	int			 size;
	int			 send_flags;
	int			 rx_depth;
	int			 pending;
	struct ibv_port_attr     portinfo;
	uint64_t		 completion_timestamp_mask;
};

class ObjectManagerRdma : public ray::rpc::ObjectManagerRdmaServiceHandler {
public:
  ObjectManagerRdma(instrumented_io_context &main_service, int port, std::string object_manager_address, unsigned long start_address, int64_t plasma_size,\
         std::shared_ptr<ray::gcs::GcsClient> gcs_client, ray::ObjectManager &object_manager, ray::raylet::DependencyManager *dependency_manager, int rpc_service_threads_number, int object_manager_rdma_port, std::string object_manager_rdma_address)
    :  main_service_(&main_service),
      acceptor_(main_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(object_manager_address), port)),
      socket_(main_service),
      plasma_address_(start_address),
      plasma_size_(plasma_size), 
      gcs_client_(gcs_client),
      object_manager_(object_manager),
      dependency_manager_(dependency_manager),
      rpc_service_threads_number_(rpc_service_threads_number),
      local_ip_address_(object_manager_address),
      object_manager_rdma_server_("ObjectManagerRdma",
                             object_manager_rdma_port,
                             object_manager_rdma_address == "127.0.0.1",
                             rpc_service_threads_number),
      rpc_work_(rpc_service_),
      object_manager_rdma_service_(rpc_service_, *this),
      client_call_manager_(main_service, rpc_service_threads_number)
       {
        RAY_LOG(DEBUG) << "Init ObjectManagerRdma Start Address " << start_address << " Plasma Size " << plasma_size;
        InitRdmaConfig();
        StartRdmaService();
        DoAccept();
        // ExRdmaConfig();
        StartRpcService();
      }
  ~ObjectManagerRdma();

  void DoAccept();
  void HandleAccept(const boost::system::error_code &error);
  void Stop();
  void InitRdmaConfig();
  void InitRdmaBaseCfg();
  void InitRdmaCtx(struct pingpong_context *ctx, struct pingpong_dest *my_dest);
  void pp_init_ctx(struct pingpong_context *ctx, struct ibv_device *ib_dev, int rx_depth, int port, int use_event);
  struct ibv_cq* pp_cq(struct pingpong_context *ctx);
  int pp_get_port_info(struct ibv_context *context, int port, struct ibv_port_attr *attr);
  
  // void ExRdmaConfig();
  void ConnectAndEx(std::string ip_address);
  void FreeRdmaResource(struct pingpong_context *ctx);
  void PrintRemoteRdmaInfo();
  void FetchObjectFromRemotePlasma(const std::vector<std::string> &object_address, const std::vector<unsigned long>  &object_virt_address, 
                                  const std::vector<int>  &object_sizes, std::vector<ray::ObjectInfo> &object_info, const std::vector<std::string> &rem_ip_address, ray::raylet::DependencyManager &dependency_manager);
  int CovRdmaStatus(struct pingpong_context *ctx, struct pingpong_dest *dest, struct pingpong_dest *my_dest);
  void QueryQp(struct pingpong_context *ctx);
  int PostSend(struct pingpong_context *ctx, struct pingpong_dest *rem_dest, unsigned long buf, int msg_size, unsigned long remote_address, int opcode);
  int PollCompletion(struct pingpong_context *ctx, const absl::optional<plasma::Allocation> &allocation, const ray::ObjectInfo &object_info, ray::raylet::DependencyManager *dependency_manager, int64_t start_time);

  void StartRdmaService();
  void StopRdmaService();
  void InsertObjectInQueue(std::vector<ObjectRdmaInfo> &object_rdma_info);

  void RunRdmaService(int64_t index);
  void FetchObjectFromRemotePlasmaThreads(ObjectRdmaInfo &object_rdma_info, int64_t t_index);
  int PollCompletionThreads(struct pingpong_context *ctx, const plasma::Allocation &allocation, const ray::ObjectInfo &object_info, const std::pair<const plasma::LocalObject *, plasma::flatbuf::PlasmaError>& pair, int64_t start_time, int64_t te_fetch_object_rdma_space, int64_t te_fetch_object_post_send);
  

  void HandleGetObject(const ray::rpc::GetObjectRequest &request,
                  ray::rpc::GetObjectReply *reply,
                  ray::rpc::SendReplyCallback send_reply_callback) override;

  /// Get the port of the object manager rpc server.
  int GetServerPort() const { return object_manager_rdma_server_.GetPort(); }

private:
  instrumented_io_context *main_service_;
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ip::tcp::socket socket_;
  // struct pingpong_context *ctx_;
  // struct pingpong_dest my_dest_;
  struct Config cfg_;
  unsigned long plasma_address_;
  int64_t plasma_size_;
  std::shared_ptr<ray::gcs::GcsClient> gcs_client_;
  //remote ip , local ctx, local dest, remote dest
  absl::flat_hash_map<std::string, std::pair<std::pair<struct pingpong_context*, struct pingpong_dest*>, struct pingpong_dest*>> remote_dest_;
  ray::ObjectManager &object_manager_;
  std::string local_ip_address_;
  ray::raylet::DependencyManager *dependency_manager_;


  /// The thread pool used for running `rmda_fetch`.
  std::vector<std::thread> rpc_threads_;
  std::vector<std::thread> object_threads_;

  int rpc_service_threads_number_;
  std::mutex mtx_;
  std::condition_variable cv_;

  /// Multi-thread asio service, deal with all outgoing and incoming RPC request.
  instrumented_io_context rpc_service_;

  /// Keep rpc service running when no task in rpc service.
  boost::asio::io_service::work rpc_work_;

  moodycamel::ConcurrentQueue<ObjectRdmaInfo> object_rdma_queue_;

  /// The gPRC server.
  ray::rpc::GrpcServer object_manager_rdma_server_;
  /// The gRPC service.
  ray::rpc::ObjectManagerRdmaGrpcService object_manager_rdma_service_;

  /// The client call manager used to deal with reply.
  ray::rpc::ClientCallManager client_call_manager_;

};

class Session
  : public std::enable_shared_from_this<Session>
{
public:
  Session(tcp::socket socket, struct pingpong_context *ctx, struct pingpong_dest *rem_dest, struct pingpong_dest *my_dest, struct Config cfg)
    : socket_(std::move(socket)),
      rem_dest_(rem_dest),
      my_dest_(my_dest),
      ctx_(ctx),
      cfg_(cfg)
  {
  }

  void Start()
  {
    DoRead();
  }
  int CovRdmaStatus(struct pingpong_context *ctx, struct pingpong_dest *dest, struct pingpong_dest *my_dest, struct Config &cfg_);

private:
  void DoRead()
  {
    auto self(shared_from_this());
    // socket_.async_read_some(boost::asio::buffer(rem_dest_, sizeof(pingpong_dest)),
    async_read(socket_, boost::asio::buffer(rem_dest_, sizeof(struct pingpong_dest) * num_qp_pair),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            for(int i =0; i <num_qp_pair; i++) {
              RAY_LOG(DEBUG) << "do read remote info remote psn server" << (rem_dest_+i)->psn << " remote rkey " << (rem_dest_+i)->rkey;
              CovRdmaStatus(ctx_+i, rem_dest_+i, my_dest_+i, cfg_);
            }
              
            DoWrite(length);
          }
        });
  }

  void DoWrite(std::size_t length)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(my_dest_, sizeof(struct pingpong_dest) * num_qp_pair),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            DoRead();
          }
        });
  }

  tcp::socket socket_;
  // enum { max_length = 1024 };
  // char data_[max_length];
  struct pingpong_dest *rem_dest_;
  struct pingpong_dest *my_dest_;
  struct pingpong_context *ctx_;
  struct Config cfg_;
  // ObjectManagerRdma object_manager_rdma_;
};
// struct Config cfg = {
//   7000, /*port*/
// 	NULL, /* dev_name */
//   1, /*ib_port */
//   1, 	/*size */
//   IBV_MTU_4096, /*ibv mtu */
//   500,  /*rx_depth */
//   1, /*iters */
//   0, /*sl */
//   1, /*gid_idx */
// 	1, /* num_threads */
//   // "SR", /* op type */
// 	NULL, /* server_name */
//   0,  /* use_event */
// };




// enum ibv_mtu pp_mtu_to_enum(int mtu);
// int pp_get_port_info(struct ibv_context *context, int port,
// 		     struct ibv_port_attr *attr);
// void wire_gid_to_gid(const char *wgid, union ibv_gid *gid);
// void gid_to_wire_gid(const union ibv_gid *gid, char wgid[]);