#pragma once
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <string.h>
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

class ObjectManagerRdma {
  public:
  ObjectManagerRdma(instrumented_io_context &main_service, int port)
    : acceptor_(main_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
      ,socket_(main_service) {
        DoAccept();
    }

  void DoAccept();
  void HandleAccept(const boost::system::error_code &error);

  private:
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::socket socket_;
};

// struct Config {
// 	uint32_t	port;
// 	char	*ib_devname;
//   int ib_port;
//   int		size;
//   enum ibv_mtu mtu;
// 	unsigned int rx_depth;
// 	unsigned int iters;
//   int sl;
//   int gidx;
// 	int		num_threads;
// 	// char	*op_type;
//   char	*server_name;
//   int use_event;
// };

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
	char			*buf;
	int			 size;
	int			 send_flags;
	int			 rx_depth;
	int			 pending;
	struct ibv_port_attr     portinfo;
	uint64_t		 completion_timestamp_mask;
};


// enum ibv_mtu pp_mtu_to_enum(int mtu);
// int pp_get_port_info(struct ibv_context *context, int port,
// 		     struct ibv_port_attr *attr);
// void wire_gid_to_gid(const char *wgid, union ibv_gid *gid);
// void gid_to_wire_gid(const union ibv_gid *gid, char wgid[]);