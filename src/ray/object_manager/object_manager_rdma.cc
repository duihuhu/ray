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
      boost::bind(&ObjectManagerRdma::HandleAccept, this, boost::asio::placeholders::error));
}

void ObjectManagerRdma::HandleAccept(const boost::system::error_code &error) {
  if (!error) {
    // boost::bind(&ObjectManagerRdma::ProcessInfoMessage, this, boost::asio::placeholders::error);
     RAY_LOG(DEBUG) <<"remote ip:"<<socket_.remote_endpoint().address(); 
  }
  DoAccept();
}

void ObjectManagerRdma::Stop() {
  RAY_LOG(DEBUG) << " ObjectManagerRdma::Stop()  ";
  acceptor_.close(); 
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
    cfg_.use_event = 0;
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
	int                      rcnt, scnt;
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
  return;

}

void ObjectManagerRdma::pp_init_ctx(struct ibv_device *ib_dev,
					    int rx_depth, int port, int use_event)
{
	// struct pingpong_context *ctx;
	int access_flags = IBV_ACCESS_LOCAL_WRITE;

	ctx = calloc(1, sizeof *ctx);
	if (!ctx)
		return NULL;

	ctx->size       = plasma_size_;
	ctx->send_flags = IBV_SEND_SIGNALED;
	ctx->rx_depth   = rx_depth;

	ctx->buf = (void *) plasma_address_;
	if (!ctx->buf) {
		fprintf(stderr, "Couldn't allocate work buf.\n");
		goto clean_ctx;
	}

	/* FIXME memset(ctx->buf, 0, size); */
	// memset(ctx->buf, 0x7b, size);

	ctx->context = ibv_open_device(ib_dev);
	if (!ctx->context) {
		fprintf(stderr, "Couldn't get context for %s\n",
			ibv_get_device_name(ib_dev));
		goto clean_buffer;
	}

	if (use_event) {
		ctx->channel = ibv_create_comp_channel(ctx->context);
		if (!ctx->channel) {
			fprintf(stderr, "Couldn't create completion channel\n");
			goto clean_device;
		}
	} else
		ctx->channel = NULL;

	ctx->pd = ibv_alloc_pd(ctx->context);
	if (!ctx->pd) {
		fprintf(stderr, "Couldn't allocate PD\n");
		goto clean_comp_channel;
	}

	

  ctx->mr = ibv_reg_mr(ctx->pd, ctx->buf, plasma_size_, access_flags);
	if (!ctx->mr) {
		fprintf(stderr, "Couldn't register MR\n");
		goto clean_dm;
	}

  ctx->cq_s.cq = ibv_create_cq(ctx->context, rx_depth + 1, NULL,
              ctx->channel, 0);

	if (!pp_cq(ctx)) {
		fprintf(stderr, "Couldn't create CQ\n");
		goto clean_mr;
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
			fprintf(stderr, "Couldn't create QP\n");
			goto clean_cq;
		}

		ibv_query_qp(ctx->qp, &attr, IBV_QP_CAP, &init_attr);
    if (init_attr.cap.max_inline_data >= size)
			ctx->send_flags |= IBV_SEND_INLINE;

	}

	{
		struct ibv_qp_attr attr = {
			.qp_state        = IBV_QPS_INIT,
			.pkey_index      = 0,
			.port_num        = port,
			.qp_access_flags = 0
		};

		if (ibv_modify_qp(ctx->qp, &attr,
				  IBV_QP_STATE              |
				  IBV_QP_PKEY_INDEX         |
				  IBV_QP_PORT               |
				  IBV_QP_ACCESS_FLAGS)) {
			fprintf(stderr, "Failed to modify QP to INIT\n");
			goto clean_qp;
		}
	}


  clean_qp:
    ibv_destroy_qp(ctx->qp);

  clean_cq:
    ibv_destroy_cq(pp_cq(ctx));

  clean_mr:
    ibv_dereg_mr(ctx->mr);

  clean_dm:
    if (ctx->dm)
      ibv_free_dm(ctx->dm);

  // clean_pd:
    ibv_dealloc_pd(ctx->pd);

  clean_comp_channel:
    if (ctx->channel)
      ibv_destroy_comp_channel(ctx->channel);

  clean_device:
    ibv_close_device(ctx->context);

  clean_buffer:
    free(ctx->buf);

  clean_ctx:
    free(ctx);

	return NULL;
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
