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
#include <string>

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

class ObjectManagerRdma {
  public:
  ObjectManagerRdma(instrumented_io_context &main_service, int port, std::string object_manager_address, unsigned long start_address, int64_t plasma_size)
    : acceptor_(main_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(object_manager_address), port))
      ,socket_(main_service),
      plasma_address_(start_address),
      plasma_size_(plasma_size) {
        InitRdmaConfig();
        DoAccept();
    }

  void DoAccept();
  void HandleAccept(const boost::system::error_code &error);
  void Stop();
  void InitRdmaConfig();
  void InitRdmaBaseCfg();
  void InitRdmaCtx();
  void pp_init_ctx(struct ibv_device *ib_dev, int rx_depth, int port, int use_event);
  struct ibv_cq* pp_cq();
  int pp_get_port_info(struct ibv_context *context, int port, struct ibv_port_attr *attr)
  
  private:
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::socket socket_;
    struct pingpong_context *ctx_;
    struct pingpong_dest my_dest_;

    struct Config cfg_;
    unsigned long plasma_address_;
    int64_t plasma_size_;
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