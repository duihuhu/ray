#include "ray/object_manager/object_manager_rdma.h"
// #include <endian.h>
// #include <stdlib.h>
// #include <stdio.h>
// #include <string.h>
#include "ray/util/util.h"


void ObjectManagerRdma::DoAccept() {
  RAY_LOG(DEBUG) << " ObjectManagerRdma::DoAccept()  ";
  acceptor_.async_accept(
      socket_,
      boost::bind(&ObjectManagerRdma::HandleAccept, this, boost::asio::placeholders::error, std::move(socket_)));
}

void ObjectManagerRdma::HandleAccept(const boost::system::error_code &error, boost::asio::ip::tcp::socket socket_) {
  if (!error) {
    // boost::bind(&ObjectManagerRdma::ProcessInfoMessage, this, boost::asio::placeholders::error);
     RAY_LOG(DEBUG)  <<"remote ip:"<<socket_.remote_endpoint().address(); 
  }
  DoAccept();
}

// enum ibv_mtu pp_mtu_to_enum(int mtu)
// {
// 	switch (mtu) {
// 	case 256:  return IBV_MTU_256;
// 	case 512:  return IBV_MTU_512;
// 	case 1024: return IBV_MTU_1024;
// 	case 2048: return IBV_MTU_2048;
// 	case 4096: return IBV_MTU_4096;
// 	default:   return 0;
// 	}
// }

// int pp_get_port_info(struct ibv_context *context, int port,
// 		     struct ibv_port_attr *attr)
// {
// 	return ibv_query_port(context, port, attr);
// }

// void wire_gid_to_gid(const char *wgid, union ibv_gid *gid)
// {
// 	char tmp[9];
// 	__be32 v32;
// 	int i;
// 	uint32_t tmp_gid[4];

// 	for (tmp[8] = 0, i = 0; i < 4; ++i) {
// 		memcpy(tmp, wgid + i * 8, 8);
// 		sscanf(tmp, "%x", &v32);
// 		tmp_gid[i] = be32toh(v32);
// 	}
// 	memcpy(gid, tmp_gid, sizeof(*gid));
// }

// void gid_to_wire_gid(const union ibv_gid *gid, char wgid[])
// {
// 	uint32_t tmp_gid[4];
// 	int i;

// 	memcpy(tmp_gid, gid, sizeof(tmp_gid));
// 	for (i = 0; i < 4; ++i)
// 		sprintf(&wgid[i * 8], "%08x", htobe32(tmp_gid[i]));
// }
