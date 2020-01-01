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
        struct pingpong_dest *my_dest = new pingpong_dest();
        struct pingpong_context *ctx = new pingpong_context();
        InitRdmaCtx(ctx, my_dest);
        // remote_dest_.emplace(socket.remote_endpoint().address().to_string(), rem_dest);
        remote_dest_.emplace(socket.remote_endpoint().address().to_string(), std::make_pair(std::make_pair(ctx, my_dest),rem_dest));

        std::make_shared<Session>(std::move(socket), ctx, rem_dest, my_dest, cfg_)->Start();
      }
      DoAccept();
    });

}

void ObjectManagerRdma::ConnectAndEx(std::string ip_address) {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket s(io_context);
    boost::asio::ip::tcp::resolver resolver(io_context);
    boost::asio::connect(s, resolver.resolve(ip_address, std::to_string(cfg_.port)));
    struct pingpong_dest *my_dest = new pingpong_dest();
    struct pingpong_context *ctx = new pingpong_context();
    InitRdmaCtx(ctx, my_dest);
    boost::asio::write(s, boost::asio::buffer(&my_dest, sizeof(struct pingpong_dest)));
    struct pingpong_dest* rem_dest = new pingpong_dest();
    size_t reply_length = boost::asio::read(s,
        boost::asio::buffer(rem_dest, sizeof(struct pingpong_dest)));
    // remote_dest_.emplace(ip_address, rem_dest);
    CovRdmaStatus(ctx, rem_dest, my_dest);
    // remote_dest_[socket.remote_endpoint().address().to_string()] = std::make_pair(std::make_pair(ctx, my_dest),rem_dest);
    remote_dest_.emplace(ip_address, std::make_pair(std::make_pair(ctx, my_dest),rem_dest));
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
      FreeRdmaResource(entry.second.first.first);
      free(entry.second.first.second);
      free(entry.second.second);
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
  // InitRdmaCtx();
}

void ObjectManagerRdma::InitRdmaCtx(struct pingpong_context *ctx, struct pingpong_dest *my_dest) {
  struct ibv_device      **dev_list;
	struct ibv_device	*ib_dev;
	// struct pingpong_context *ctx;
	// struct pingpong_dest     my_dest;
	// struct pingpong_dest    *rem_dest;
	char                    *ib_devname = cfg_.ib_devname;
	// unsigned int             port = cfg_.port;
	int                      ib_port = 1;
	// unsigned int             size = cfg_.size;
	// enum ibv_mtu		 mtu = cfg_.mtu;
	unsigned int             rx_depth = cfg_.rx_depth;
	// unsigned int             iters = cfg_.iters;
	int                      use_event = cfg_.use_event;
	// int                      routs;
	// int                      rcnt, scnt;
	// int                      num_cq_events = 0;
	// int                      sl = 0;
	int			 gidx = cfg_.gidx;
	char			 gid[33];
  // int page_size;
  srand48(getpid() * time(NULL));

	// page_size = sysconf(_SC_PAGESIZE);

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
  pp_init_ctx(ctx, ib_dev, rx_depth, ib_port, use_event);
	// if (!ctx)
	// 	return 0;

  if (use_event)
		if (ibv_req_notify_cq(pp_cq(ctx), 0)) {
      RAY_LOG(ERROR) << "Couldn't request CQ notification";
			return ;
		}

  if (pp_get_port_info(ctx->context, cfg_.ib_port, &ctx->portinfo)) {
    RAY_LOG(ERROR) << "Couldn't get port info";
		return;
	}

	my_dest->lid = ctx->portinfo.lid;
	if (ctx->portinfo.link_layer != IBV_LINK_LAYER_ETHERNET &&
							!my_dest->lid) {
    RAY_LOG(ERROR) << "Couldn't get local LID";
		return;
	}

	if (cfg_.gidx >= 0) {
		if (ibv_query_gid(ctx->context, cfg_.ib_port, cfg_.gidx, &my_dest->gid)) {
      RAY_LOG(ERROR) << "can't read sgid of index " << cfg_.gidx;
			return;
		}
	} else
		memset(&my_dest->gid, 0, sizeof my_dest->gid);

	my_dest->qpn = ctx->qp->qp_num;
	my_dest->psn = lrand48() & 0xffffff;
	inet_ntop(AF_INET6, &my_dest->gid, gid, sizeof gid);
	// printf("  local address:  LID 0x%04x, QPN 0x%06x, PSN 0x%06x, GID %s\n",
	//        my_dest_.lid, my_dest_.qpn, my_dest_.psn, gid);
  RAY_LOG(DEBUG) << "  local address:  LID " << my_dest->lid << " QPN " <<  my_dest->qpn \
  <<" PSN " << my_dest->psn << " GID " << gid;
  return;

}

struct ibv_cq* ObjectManagerRdma::pp_cq(struct pingpong_context *ctx)
{
    return ctx->cq_s.cq;
}

void ObjectManagerRdma::pp_init_ctx(struct pingpong_context *ctx, struct ibv_device *ib_dev,
					    int rx_depth, int port, int use_event)
{
	// struct pingpong_context *ctx;
	// int access_flags = IBV_ACCESS_LOCAL_WRITE;
  int access_flags =  IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC;
	// ctx = (struct pingpong_context *) calloc(1, sizeof *ctx);

	// if (!ctx)
	// 	return;

	ctx->size       = plasma_size_;
	ctx->send_flags = IBV_SEND_SIGNALED;
	ctx->rx_depth   = rx_depth;

	ctx->buf = (void *) plasma_address_;

	// if (!ctx_->buf) {
	// 	fprintf(stderr, "Couldn't allocate work buf.\n");
	// 	goto clean_ctx;
	// }

	/* FIXME memset(ctx->buf, 0, size); */
	// memset(ctx->buf, 0x7b, size);

	ctx->context = ibv_open_device(ib_dev);
	if (!ctx->context) {
    RAY_LOG(ERROR) << "Couldn't get context for " << ibv_get_device_name(ib_dev);

		// fprintf(stderr, "Couldn't get context for %s\n", ibv_get_device_name(ib_dev));
		// goto clean_buffer;
	}

	if (use_event) {
		ctx->channel = ibv_create_comp_channel(ctx->context);
		if (!ctx->channel) {
      RAY_LOG(ERROR) << "Couldn't create completion channel";
			// goto clean_device;
		}
	} else
		ctx->channel = NULL;


	ctx->pd = ibv_alloc_pd(ctx->context);
	if (!ctx->pd) {
    RAY_LOG(ERROR) << "Couldn't allocate PD";
		// goto clean_comp_channel;
	}


  ctx->mr = ibv_reg_mr(ctx->pd, ctx->buf, plasma_size_, access_flags);
	if (!ctx->mr) {
    RAY_LOG(ERROR) << "Couldn't register MR";
		// goto clean_dm;
	}

  ctx->cq_s.cq = ibv_create_cq(ctx->context, rx_depth + 1, NULL,
              ctx->channel, 0);

	if (!pp_cq(ctx)) {
    RAY_LOG(ERROR) << "Couldn't create CQ";
		// goto clean_mr;
	}

	{
		struct ibv_qp_attr attr;
		struct ibv_qp_init_attr init_attr = {
			.send_cq = pp_cq(ctx),
			.recv_cq = pp_cq(ctx),
			.cap     = {
				.max_send_wr  = 1,
				.max_recv_wr  = rx_depth + 1, 
				.max_send_sge = 1,
				.max_recv_sge = 1
			},
			.qp_type = IBV_QPT_RC
		};

    ctx->qp = ibv_create_qp(ctx->pd, &init_attr);

		if (!ctx->qp)  {
      RAY_LOG(ERROR) << "Couldn't create QP";
			// goto clean_cq;
		}

		ibv_query_qp(ctx->qp, &attr, IBV_QP_CAP, &init_attr);
    // if (init_attr.cap.max_inline_data >= size)
		// 	ctx_->send_flags |= IBV_SEND_INLINE;

	}

	{
		struct ibv_qp_attr attr = {
			.qp_state        = IBV_QPS_INIT,
			.pkey_index      = 0,
			.port_num        = port
		};
    // attr.qp_access_flags = 0;
    attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE  | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC;
		if (ibv_modify_qp(ctx->qp, &attr,
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
  //   ibv_destroy_qp(ctx->qp);

  // clean_cq:
  //   ibv_destroy_cq(pp_cq(ctx));

  // clean_mr:
  //   ibv_dereg_mr(ctx->mr);

  // clean_dm:
  //   if (ctx->dm)
  //     ibv_free_dm(ctx->dm);

  // clean_pd:
  //   ibv_dealloc_pd(ctx->pd);

  // clean_comp_channel:
  //   if (ctx_->channel)
  //     ibv_destroy_comp_channel(ctx->channel);

  // clean_device:
  //   ibv_close_device(ctx->context);

  // clean_buffer:
  //   free(ctx->buf);

  // clean_ctx:
  //   free(ctx);

	return;
}


int ObjectManagerRdma::CovRdmaStatus(struct pingpong_context *ctx, struct pingpong_dest *dest, struct pingpong_dest *my_dest)
{
	struct ibv_qp_attr attr = {
		.qp_state		= IBV_QPS_RTR,
		.path_mtu		= cfg_.mtu,
		.dest_qp_num		= dest->qpn,
		// .rq_psn			= dest->psn,
		.max_dest_rd_atomic	= 1,
		.min_rnr_timer		= 12,
		// .ah_attr		= {
		// 	.is_global	= 0,
		// 	.dlid		= dest->lid,
		// 	.sl		= cfg_.sl,
		// 	.src_path_bits	= 0,
		// 	.port_num	= cfg_.ib_port
		// }
	};
  attr.rq_psn = dest->psn;
  attr.ah_attr.is_global = 0;
  attr.ah_attr.dlid = dest->lid;
  attr.ah_attr.sl = cfg_.sl;
  attr.ah_attr.src_path_bits = 0;
  attr.ah_attr.port_num = cfg_.ib_port;

	if (dest->gid.global.interface_id) {
		attr.ah_attr.is_global = 1;
		attr.ah_attr.grh.hop_limit = 1;
		attr.ah_attr.grh.dgid = dest->gid;
		attr.ah_attr.grh.sgid_index = cfg_.gidx;
	}
	if (ibv_modify_qp(ctx->qp, &attr,
			  IBV_QP_STATE              |
			  IBV_QP_AV                 |
			  IBV_QP_PATH_MTU           |
			  IBV_QP_DEST_QPN           |
			  IBV_QP_RQ_PSN             |
			  IBV_QP_MAX_DEST_RD_ATOMIC |
			  IBV_QP_MIN_RNR_TIMER)) {
		// fprintf(stderr, "Failed to modify QP to RTR\n");
    RAY_LOG(ERROR) << "Failed to modify QP to RTR";
		return 1;
	}

	attr.qp_state	    = IBV_QPS_RTS;
	attr.timeout	    = 14;
	attr.retry_cnt	    = 7;
	attr.rnr_retry	    = 7;
	attr.sq_psn	    = my_dest->psn;
	attr.max_rd_atomic  = 1;
	if (ibv_modify_qp(ctx->qp, &attr,
			  IBV_QP_STATE              |
			  IBV_QP_TIMEOUT            |
			  IBV_QP_RETRY_CNT          |
			  IBV_QP_RNR_RETRY          |
			  IBV_QP_SQ_PSN             |
			  IBV_QP_MAX_QP_RD_ATOMIC)) {
		// fprintf(stderr, "Failed to modify QP to RTS\n");
    RAY_LOG(ERROR) << "Failed to modify QP to RTS";
		return 1;
	}
  RAY_LOG(DEBUG) << "Accomplish modify QP to RTS";

	return 0;
}



void ObjectManagerRdma::FreeRdmaResource(struct pingpong_context *ctx) {
    ibv_destroy_qp(ctx->qp);
    ibv_destroy_cq(pp_cq(ctx));
    ibv_dereg_mr(ctx->mr);
    ibv_free_dm(ctx->dm);
    ibv_dealloc_pd(ctx->pd);
    ibv_destroy_comp_channel(ctx->channel);
    ibv_close_device(ctx->context);
    free(ctx);
}

int ObjectManagerRdma::pp_get_port_info(struct ibv_context *context, int port,
		     struct ibv_port_attr *attr)
{
	return ibv_query_port(context, port, attr);
}

void ObjectManagerRdma::PrintRemoteRdmaInfo() {
  for(auto &entry: remote_dest_) {
    RAY_LOG(DEBUG) << "PrintRemoteRdmaInfo " << entry.first;
  }
}

void ObjectManagerRdma::FetchObjectFromRemotePlasma(const ray::WorkerID &worker_id, const std::vector<std::string> &object_address, const std::vector<unsigned long>  object_virt_address, const std::vector<int>  object_sizes) {
  RAY_LOG(DEBUG) << "Starting get object through rdma for worker " << worker_id;
  for(int i = 0; i < object_address.size(); ++i) {
    std::string address = object_address[i];
    auto it = remote_dest_.find(address);
    if(it!=remote_dest_.end())
      continue;
      // QueryQp(it.second.first);
  }
}

void ObjectManagerRdma::QueryQp(struct pingpong_context *ctx) {
  struct ibv_qp_attr attr;
  struct ibv_qp_init_attr init_attr;
  
  if (ibv_query_qp(ctx->qp, &attr,
        IBV_QP_STATE, &init_attr)) {
    // fprintf(stderr, "Failed to query QP state\n");
    RAY_LOG(ERROR) << "Failed to query QP state ";
    return ;
  }
  RAY_LOG(DEBUG) << "query QP state " << attr.qp_state;

}



int Session::CovRdmaStatus(struct pingpong_context *ctx, struct pingpong_dest *dest, struct pingpong_dest *my_dest,  struct Config &cfg_)
{
	struct ibv_qp_attr attr = {
		.qp_state		= IBV_QPS_RTR,
		.path_mtu		= cfg_.mtu,
		.dest_qp_num		= dest->qpn,
		// .rq_psn			= dest->psn,
		.max_dest_rd_atomic	= 1,
		.min_rnr_timer		= 12,
		// .ah_attr		= {
		// 	.is_global	= 0,
		// 	.dlid		= dest->lid,
		// 	.sl		= cfg_.sl,
		// 	.src_path_bits	= 0,
		// 	.port_num	= cfg_.ib_port
		// }
	};

	if (dest->gid.global.interface_id) {
		attr.ah_attr.is_global = 1;
		attr.ah_attr.grh.hop_limit = 1;
		attr.ah_attr.grh.dgid = dest->gid;
		attr.ah_attr.grh.sgid_index = cfg_.gidx;
	}
	if (ibv_modify_qp(ctx->qp, &attr,
			  IBV_QP_STATE              |
			  IBV_QP_AV                 |
			  IBV_QP_PATH_MTU           |
			  IBV_QP_DEST_QPN           |
			  IBV_QP_RQ_PSN             |
			  IBV_QP_MAX_DEST_RD_ATOMIC |
			  IBV_QP_MIN_RNR_TIMER)) {
		// fprintf(stderr, "Failed to modify QP to RTR\n");
    RAY_LOG(ERROR) << "Failed to modify QP to RTR";
		return 1;
	}

	attr.qp_state	    = IBV_QPS_RTS;
	attr.timeout	    = 14;
	attr.retry_cnt	    = 7;
	attr.rnr_retry	    = 7;
	attr.sq_psn	    = my_dest->psn;
	attr.max_rd_atomic  = 1;
	if (ibv_modify_qp(ctx->qp, &attr,
			  IBV_QP_STATE              |
			  IBV_QP_TIMEOUT            |
			  IBV_QP_RETRY_CNT          |
			  IBV_QP_RNR_RETRY          |
			  IBV_QP_SQ_PSN             |
			  IBV_QP_MAX_QP_RD_ATOMIC)) {
		// fprintf(stderr, "Failed to modify QP to RTS\n");
    RAY_LOG(ERROR) << "Failed to modify QP to RTS";
		return 1;
	}
  RAY_LOG(DEBUG) << "Accomplish modify QP to RTS";

	return 0;
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
