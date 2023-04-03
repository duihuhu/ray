#include "ray/object_manager/object_manager_rdma.h"
// #include <endian.h>
// #include <stdlib.h>
// #include <stdio.h>
// #include <string.h>
#include "ray/util/util.h"
#include <sys/socket.h>

void ObjectManagerRdma::DoAccept() {
  // RAY_LOG(DEBUG) << " ObjectManagerRdma::DoAccept()  ";
  // acceptor_.async_accept(
  //     socket_,
  //     boost::bind(&ObjectManagerRdma::HandleAccept, this, boost::asio::placeholders::error));
  

  acceptor_.async_accept(
    [this](boost::system::error_code ec, tcp::socket socket)
    {
      if (!ec)
      {
        struct pingpong_dest *rem_dest = new pingpong_dest();
        remote_dest_.emplace(socket.remote_endpoint().address().to_string(), rem_dest);
        std::make_shared<Session>(std::move(socket), rem_dest, my_dest_)->Start();
      }

      DoAccept();
    });

}

void ObjectManagerRdma::ConnectAndEx(std::string ip_address) {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket s(io_context);
    boost::asio::ip::tcp::resolver resolver(io_context);
    boost::asio::connect(s, resolver.resolve(ip_address, std::to_string(cfg_.port)));
    boost::asio::write(s, boost::asio::buffer(&my_dest_, sizeof(struct pingpong_dest)));
    struct pingpong_dest* rem_dest = new pingpong_dest();
    size_t reply_length = boost::asio::read(s,
        boost::asio::buffer(&rem_dest, sizeof(struct pingpong_dest)));
    remote_dest_.emplace(ip_address, rem_dest);

}

// void ObjectManagerRdma::HandleAccept(const boost::system::error_code &error) {
//   if (!error) {
//     // boost::bind(&ObjectManagerRdma::ProcessInfoMessage, this, boost::asio::placeholders::error);
//      RAY_LOG(DEBUG) <<"remote ip:"<<socket_.remote_endpoint().address(); 
//   }
//   DoAccept();
// }

void ObjectManagerRdma::Stop() {
  RAY_LOG(DEBUG) << " ObjectManagerRdma::Stop()  ";
  acceptor_.close();
  if (!remote_dest_.empty()) {
    for (auto &entry: remote_dest_) {
      free(entry.second);
    }
    remote_dest_.clear();
  }
}

void ObjectManagerRdma::InitRdmaBaseCfg() {
    cfg_.port = 7000;
    cfg_.ib_devname = "mlx5_1";
    cfg_.ib_port = 1;
    cfg_.size = 4096;
    cfg_.mtu = IBV_MTU_4096;
    cfg_.rx_depth = 500;
    cfg_.iters = 1;
    cfg_.sl = 0;
    cfg_.gidx = 1;
    cfg_.num_threads = 1;
    cfg_.server_name = NULL;
    cfg_.use_event = 1;
}

void ObjectManagerRdma::InitRdmaConfig() {
  InitRdmaBaseCfg();
  RAY_LOG(DEBUG) << "InitRdmaConfig " << plasma_address_ << " " << plasma_size_;
  InitRdmaCtx();
}

void ObjectManagerRdma::InitRdmaCtx() {
  struct ibv_device      **dev_list;
	struct ibv_device	*ib_dev;
	// struct pingpong_context *ctx;
	// struct pingpong_dest     my_dest;
	// struct pingpong_dest    *rem_dest;
	char                    *ib_devname = cfg_.ib_devname;
	unsigned int             port = cfg_.port;
	int                      ib_port = 1;
	unsigned int             size = cfg_.size;
	enum ibv_mtu		 mtu = cfg_.mtu;
	unsigned int             rx_depth = cfg_.rx_depth;
	unsigned int             iters = cfg_.iters;
	int                      use_event = cfg_.use_event;
	int                      routs;
	// int                      rcnt, scnt;
	int                      num_cq_events = 0;
	int                      sl = 0;
	int			 gidx = cfg_.gidx;
	char			 gid[33];
  int page_size;
  srand48(getpid() * time(NULL));

	page_size = sysconf(_SC_PAGESIZE);

	dev_list = ibv_get_device_list(NULL);
	if (!dev_list) {
		// perror("Failed to get IB devices list");
    RAY_LOG(ERROR) << "Failed to get IB devices list";
		return;
	}

	if (!ib_devname) {
		ib_dev = *dev_list;
		if (!ib_dev) {
			// fprintf(stderr, "No IB devices found\n");
      RAY_LOG(ERROR) << "No IB devices found";

			return;
		}
	} else {
		int i;
		for (i = 0; dev_list[i]; ++i)
			if (!strcmp(ibv_get_device_name(dev_list[i]), ib_devname))
				break;
		ib_dev = dev_list[i];
		if (!ib_dev) {
			// fprintf(stderr, "IB device %s not found\n", ib_devname);
      RAY_LOG(ERROR) << "IB device" << ib_devname << "not found";

			return;
		}
	}
  RAY_LOG(DEBUG) << "InitRdmaCtx " << ib_devname;
  pp_init_ctx(ib_dev, rx_depth, ib_port, use_event);
	// if (!ctx)
	// 	return 0;

  if (use_event)
		if (ibv_req_notify_cq(pp_cq(), 0)) {
      RAY_LOG(ERROR) << "Couldn't request CQ notification";
			return ;
		}

  if (pp_get_port_info(ctx_->context, cfg_.ib_port, &ctx_->portinfo)) {
    RAY_LOG(ERROR) << "Couldn't get port info";
		return;
	}

	my_dest_.lid = ctx_->portinfo.lid;
	if (ctx_->portinfo.link_layer != IBV_LINK_LAYER_ETHERNET &&
							!my_dest_.lid) {
    RAY_LOG(ERROR) << "Couldn't get local LID";
		return;
	}

	if (cfg_.gidx >= 0) {
		if (ibv_query_gid(ctx_->context, cfg_.ib_port, cfg_.gidx, &my_dest_.gid)) {
      RAY_LOG(ERROR) << "can't read sgid of index " << cfg_.gidx;
			return;
		}
	} else
		memset(&my_dest_.gid, 0, sizeof my_dest_.gid);

	my_dest_.qpn = ctx_->qp->qp_num;
	my_dest_.psn = lrand48() & 0xffffff;
	inet_ntop(AF_INET6, &my_dest_.gid, gid, sizeof gid);
	// printf("  local address:  LID 0x%04x, QPN 0x%06x, PSN 0x%06x, GID %s\n",
	//        my_dest_.lid, my_dest_.qpn, my_dest_.psn, gid);
  RAY_LOG(DEBUG) << "  local address:  LID " << my_dest_.lid << " QPN " <<  my_dest_.qpn \
  <<" PSN " << my_dest_.psn << " GID " << gid;
  return;

}

struct ibv_cq* ObjectManagerRdma::pp_cq()
{
    return ctx_->cq_s.cq;
}

void ObjectManagerRdma::pp_init_ctx(struct ibv_device *ib_dev,
					    int rx_depth, int port, int use_event)
{
	// struct pingpong_context *ctx;
	// int access_flags = IBV_ACCESS_LOCAL_WRITE;
  int access_flags =  IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC;
	ctx_ = (struct pingpong_context *) calloc(1, sizeof *ctx_);

	if (!ctx_)
		return;

	ctx_->size       = plasma_size_;
	ctx_->send_flags = IBV_SEND_SIGNALED;
	ctx_->rx_depth   = rx_depth;

	ctx_->buf = (void *) plasma_address_;

	// if (!ctx_->buf) {
	// 	fprintf(stderr, "Couldn't allocate work buf.\n");
	// 	goto clean_ctx;
	// }

	/* FIXME memset(ctx->buf, 0, size); */
	// memset(ctx->buf, 0x7b, size);

	ctx_->context = ibv_open_device(ib_dev);
	if (!ctx_->context) {
    RAY_LOG(ERROR) << "Couldn't get context for " << ibv_get_device_name(ib_dev);

		// fprintf(stderr, "Couldn't get context for %s\n", ibv_get_device_name(ib_dev));
		// goto clean_buffer;
	}

	if (use_event) {
		ctx_->channel = ibv_create_comp_channel(ctx_->context);
		if (!ctx_->channel) {
      RAY_LOG(ERROR) << "Couldn't create completion channel";
			// goto clean_device;
		}
	} else
		ctx_->channel = NULL;


	ctx_->pd = ibv_alloc_pd(ctx_->context);
	if (!ctx_->pd) {
    RAY_LOG(ERROR) << "Couldn't allocate PD";
		// goto clean_comp_channel;
	}


  ctx_->mr = ibv_reg_mr(ctx_->pd, ctx_->buf, plasma_size_, access_flags);
	if (!ctx_->mr) {
    RAY_LOG(ERROR) << "Couldn't register MR";
		// goto clean_dm;
	}

  ctx_->cq_s.cq = ibv_create_cq(ctx_->context, rx_depth + 1, NULL,
              ctx_->channel, 0);

	if (!pp_cq()) {
    RAY_LOG(ERROR) << "Couldn't create CQ";
		// goto clean_mr;
	}

	{
		struct ibv_qp_attr attr;
		struct ibv_qp_init_attr init_attr = {
			.send_cq = pp_cq(),
			.recv_cq = pp_cq(),
			.cap     = {
				.max_send_wr  = 1,
				.max_recv_wr  = rx_depth + 1, 
				.max_send_sge = 1,
				.max_recv_sge = 1
			},
			.qp_type = IBV_QPT_RC
		};

    ctx_->qp = ibv_create_qp(ctx_->pd, &init_attr);

		if (!ctx_->qp)  {
      RAY_LOG(ERROR) << "Couldn't create QP";
			// goto clean_cq;
		}

		ibv_query_qp(ctx_->qp, &attr, IBV_QP_CAP, &init_attr);
    // if (init_attr.cap.max_inline_data >= size)
		// 	ctx_->send_flags |= IBV_SEND_INLINE;

	}

	{
		struct ibv_qp_attr attr = {
			.qp_state        = IBV_QPS_INIT,
			.pkey_index      = 0,
			.port_num        = port
		};
    attr.qp_access_flags = 0;

		if (ibv_modify_qp(ctx_->qp, &attr,
				  IBV_QP_STATE              |
				  IBV_QP_PKEY_INDEX         |
				  IBV_QP_PORT               |
				  IBV_QP_ACCESS_FLAGS)) {

      RAY_LOG(ERROR) << "Failed to modify QP to INIT";
			// goto clean_qp;
		}
	}

  RAY_LOG(DEBUG) << "ibv_open_device success " << ibv_get_device_name(ib_dev);

  // clean_qp:
  //   ibv_destroy_qp(ctx_->qp);

  // clean_cq:
  //   ibv_destroy_cq(pp_cq());

  // clean_mr:
  //   ibv_dereg_mr(ctx_->mr);

  // clean_dm:
  //   if (ctx_->dm)
  //     ibv_free_dm(ctx_->dm);

  // clean_pd:
  //   ibv_dealloc_pd(ctx_->pd);

  // clean_comp_channel:
  //   if (ctx_->channel)
  //     ibv_destroy_comp_channel(ctx_->channel);

  // clean_device:
  //   ibv_close_device(ctx_->context);

  // clean_buffer:
  //   free(ctx_->buf);

  // clean_ctx:
  //   free(ctx_);

	return;
}

int ObjectManagerRdma::pp_get_port_info(struct ibv_context *context, int port,
		     struct ibv_port_attr *attr)
{
	return ibv_query_port(context, port, attr);
}

// void ObjectManagerRdma::ExRdmaConfig() {
//   RAY_LOG(DEBUG) << "ExRdmaConfig ";
//   const auto &node_map = gcs_client_->Nodes().GetAll();
//   for (const auto &item : node_map) {
//     RAY_LOG(DEBUG) << "node_manager_address " << item.second.node_manager_address();
//   }
// }


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
