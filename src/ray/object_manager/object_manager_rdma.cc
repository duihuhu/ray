#include "ray/object_manager/object_manager_rdma.h"
// #include <endian.h>
// #include <stdlib.h>
// #include <stdio.h>
// #include <string.h>
#include "ray/util/util.h"
#include <sys/socket.h>

ObjectManagerRdma::~ObjectManagerRdma() { StopRdmaService(); }

void ObjectManagerRdma::RunRdmaService(int64_t index) {
	RAY_LOG(DEBUG) << "RunRdmaService ";
	SetThreadName("obj.queue.rdma." + std::to_string(index));
	int64_t t_index = index;
  while(true) {
    ObjectRdmaInfo object_rdma_info;
    bool found = object_rdma_queue_.try_dequeue(object_rdma_info);
		{
			std::unique_lock<std::mutex> lck(mtx_);
			while (!found) {
				// std::cout << "thread " << id << " " <<  a  << " " << found << '\n';
				cv_.wait(lck);
				found = object_rdma_queue_.try_dequeue(object_rdma_info);
				// lck.unlock();
			}
		}
		if(found) {
			auto te_get_object_info = current_sys_time_us();
			// RAY_LOG(ERROR) << "raylet client send 2 " << te_get_object_info << " " << object_rdma_info.object_info.object_id  ;
			// std::thread::id tid = std::this_thread::get_id();
			// RAY_LOG(DEBUG) << "RunRdmaService thread " << tid;
			FetchObjectFromRemotePlasmaThreads(object_rdma_info, t_index);
			// t_index = t_index + 1;
		}
  }
}



void ObjectManagerRdma::FetchObjectFromRemotePlasmaThreads(ObjectRdmaInfo &object_rdma_info, int64_t t_index) {
  RAY_LOG(DEBUG) << "Starting get object through rdma for worker " << object_rdma_info.object_info.object_id;
  auto ts_fetch_object_rdma = current_sys_time_us();
  // for(uint64_t i = 0; i < object_address.size(); ++i) {
  //   std::string address = rem_ip_address[i];
	std::string obj_address = object_rdma_info.object_address;
	const ray::ObjectInfo &obj_info = object_rdma_info.object_info;
	auto it = remote_dest_.find(object_rdma_info.rem_ip_address);
	if(it!=remote_dest_.end()){
		// continue;
	//   QueryQp(it->second.first.first);
  	RAY_LOG(DEBUG) << "in remote_dest ";

		// std::random_device seed;//hardware to generate random seed
		// std::ranlux48 engine(seed());//use seed to generate 
		// std::uniform_int_distribution<> distrib(0, num_qp_pair-1);//set random min and max
		// int n_qp = distrib(engine);//n_qp
		// int n_qp = t_index%(num_qp_pair-1);
		int n_qp = t_index;
		
		// auto allocation = object_manager_.AllocateObjectSizeRdma(object_rdma_info.object_sizes);
  
		// auto ts_create_object = current_sys_time_us();
		auto pair = object_manager_.CreateObjectRdma(object_rdma_info.object_info);
		RAY_LOG(DEBUG) << " Allocate pair ";
		auto entry = pair.first;
		if (entry == nullptr) {
			// continue;
			RAY_LOG(ERROR) << "create rdma is null " << object_rdma_info.object_info.object_id;
			return;
		}
		// auto te_create_object = current_sys_time_us();
		// RAY_LOG(DEBUG) << " Allocate object size time " << object_rdma_info.object_info.object_id << " " << te_create_object - ts_create_object << " " << object_rdma_info.object_sizes;

		auto allocation = pair.first->GetAllocation();

		RAY_LOG(DEBUG) << " Allocate space allocation->address " << allocation.address << " object_id " << object_rdma_info.object_info.object_id;

		unsigned long local_address =(unsigned long) allocation.address;
		RAY_LOG(DEBUG) << " Allocate space for rdma object " << local_address;
		RAY_LOG(DEBUG) << " FetchObjectFromRemotePlasma " << local_address << " object_virt_address " << object_rdma_info.object_virt_address << "  object_sizes " <<  object_rdma_info.object_sizes << " " << object_rdma_info.rem_ip_address << " " << object_rdma_info.object_info.object_id << " " << obj_address << " " << n_qp;
		
		auto te_fetch_object_rdma_space = current_sys_time_us();
		// RAY_LOG(DEBUG) << "FetchObjectRdma time in create object space " << te_fetch_object_rdma_space - ts_fetch_object_rdma << " " << obj_info.object_id;
		// RAY_LOG(ERROR) << " post send " << obj_info.object_id << " " << object_rdma_info.object_virt_address;

		PostSend(it->second.first.first + n_qp, it->second.second + n_qp, local_address, object_rdma_info.object_sizes, object_rdma_info.object_virt_address, IBV_WR_RDMA_READ, t_index);
		// PollCompletion(it->second.first.first);
		auto ctx =  it->second.first.first + n_qp;
		auto te_fetch_object_post_send = current_sys_time_us();
		
		RAY_LOG(ERROR) << "post send object id " << object_rdma_info.object_info.object_id << " " << local_address;

//   RAY_LOG(DEBUG) << " PostSend object to RDMA ";
		// PollCompletionThreads(ctx, allocation, obj_info, ts_fetch_object_rdma);
		PollCompletionThreads(ctx, allocation, obj_info, pair, ts_fetch_object_rdma, te_fetch_object_rdma_space, te_fetch_object_post_send, t_index);
		// main_service_->post([this, ctx, allocation, obj_info, pair, ts_fetch_object_rdma, te_fetch_object_rdma_space, te_fetch_object_post_send]() { PollCompletionThreads(ctx, allocation, obj_info, pair, ts_fetch_object_rdma, te_fetch_object_rdma_space, te_fetch_object_post_send); },
		// 							"ObjectManagerRdma.PollCompletion");
		// auto te_fetch_object_rdma = current_sys_time_us();
		// RAY_LOG(DEBUG) << "FetchObjectRdma time " << obj_info.object_id << " " <<te_fetch_object_rdma - ts_fetch_object_rdma;
		// RAY_LOG(ERROR) << "raylet client send 3 " << te_fetch_object_rdma << " " << obj_info.object_id;

	}
  // }
  // auto te_fetch_object_rdma = current_sys_time_us();
	// RAY_LOG(DEBUG) << "FetchObjectRdma time " << te_fetch_object_rdma - ts_fetch_object_rdma;
}


int ObjectManagerRdma::PollCompletionThreads(struct pingpong_context *ctx, const plasma::Allocation &allocation, const ray::ObjectInfo &object_info, const std::pair<const plasma::LocalObject *, plasma::flatbuf::PlasmaError>& pair, int64_t start_time, int64_t te_fetch_object_rdma_space, int64_t te_fetch_object_post_send, int64_t t_index){
  RAY_LOG(ERROR) << "PollCompletion Threads start " << object_info.object_id;
  // auto ts_fetch_rdma = current_sys_time_us();
  struct ibv_wc wc;
	int poll_result;
	int rc = 0;
	
	if (cfg_.use_event == 1) {
		struct ibv_cq *ev_cq;
		void *ev_ctx;
		if (ibv_get_cq_event(ctx->channel, &ev_cq, &ev_ctx)) {
			rc = 1;
			return rc;
		}
		if (ev_cq != pp_cq(ctx)) {
			rc = 1;
			return rc;
		}
		if (ibv_req_notify_cq(pp_cq(ctx), 0)) {
			rc = 1;
			return rc;
		}
	}
	RAY_LOG(ERROR) << "ibv poll cq starting " << object_info.object_id;

	do {
		poll_result = ibv_poll_cq(pp_cq(ctx), 1, &wc);
		RAY_LOG(ERROR) << "ibv_poll_cq " << object_info.object_id << poll_result;
	} while (poll_result==0);
	if (poll_result < 0) {
    RAY_LOG(ERROR) << "poll cq failed";
		rc = 1;
	} else if (poll_result == 0) {
    RAY_LOG(ERROR) << "completion wasn't found in the cq after timeout";
		rc = 1;
	} else {
		// fprintf(stdout, "completion was found in cq with status 0x%x\n", wc.status);
		RAY_LOG(ERROR) << "completion was found in cq with status " << wc.status;
    if ( wc.status == IBV_WC_SUCCESS) {
			if (wc.wr_id != t_index) {
					RAY_LOG(ERROR) << "wc wr_id is error " << wc.wr_id << " " << t_index;
			}
			auto tc_fetch_rdma = current_sys_time_us();
			char *data = (char *) allocation.address;
			// RAY_LOG(ERROR) << "after " << object_info.object_id <<  " " << *(data+object_info.data_size);
			// int times = 0;
			// while(data[object_info.data_size]!='P' && data[object_info.data_size]!='R' && data[object_info.data_size]!='X' && data[object_info.data_size]!='p' && data[object_info.data_size]!='r' && data[object_info.data_size]!='x') {
			// 	// RAY_LOG(ERROR) << "object_id " << object_info.object_id << " data is:" << data[object_info.data_size];
			// 	std::this_thread::sleep_for(std::chrono::microseconds(10));
			// 	// times = times +1;
			// 	// if(times == 10) {
			// 	// 	break;
			// 	// }
			// }

			RAY_LOG(DEBUG) << " get object start time end in rdma " << object_info.object_id << " " << tc_fetch_rdma << " " << start_time;
		
			RAY_LOG(ERROR) << "before insert object info to thread " << object_info.object_id;
      // object_manager_.InsertObjectInfo(allocation, object_info);
			object_manager_.InsertObjectInfoThread(allocation, object_info, pair);
			// dependency_manager_->InsertObjectInfo(object_info);
			// auto ts_fetch_object_end = current_sys_time_us();
			// RAY_LOG(DEBUG) << "plasma client fetch object id start: " << object_info.object_id << " " << ts_fetch_object_end;
			// RAY_LOG(DEBUG) << "plasma client fetch object id start: " << object_info.object_id << " " << ts_fetch_object_end << " " << ts_fetch_rdma << " " << te_fetch_object_post_send << " " << start_time;
			// char *data = (char *) allocation.address;
			// // object info
			// int64_t data_size = allocation.size;
			// RAY_LOG(ERROR) << object_info.object_id <<  " " << object_info.object_id.Hash();
			// std::ofstream outfile1;
			// outfile1.open("hutmp_" + std::to_string(object_info.object_id.Hash()) + ".txt");
			// for(int i=0; i< data_size; ++i){
			//   outfile1<<*(data+i);
			// }
			// outfile1.close();
			/* Ack the event */ 
			ibv_ack_cq_events(pp_cq(ctx), 1);
    }
		if ( wc.status != IBV_WC_SUCCESS) {
			// fprintf(stderr, "got bad completion with status 0x:%x, verdor syndrome: 0x%x\n", wc.status, wc.vendor_err);
      RAY_LOG(ERROR) << "got bad completion with status " << wc.status << " verdor syndrome: " << wc.vendor_err;
      rc = 1;
		} 
	}
  // auto te_fetch_rdma = current_sys_time_us();
  // RAY_LOG(DEBUG) << "Poll Object in Rdma " << te_fetch_rdma - ts_fetch_rdma << " " << te_fetch_rdma - start_time  << " " << ts_fetch_rdma-start_time << " " << ts_fetch_rdma - te_fetch_object_post_send << " " <<object_info.object_id ;  
	return rc;
}

void ObjectManagerRdma::InsertObjectInQueue(std::vector<ObjectRdmaInfo> &object_rdma_info){
	//todo 
	// std::unique_lock<std::mutex> lck(mtx_);
	for(int i =0; i < object_rdma_info.size(); ++i) {
		if(object_manager_.CheckInsertObjectInfo(object_rdma_info[i].object_info.object_id) || object_rdma_info[i].object_sizes==0 || (object_rdma_info[i].rem_ip_address == local_ip_address_)) {
			RAY_LOG(DEBUG) << " Object is alread in local_object or object size is zero " << object_rdma_info[i].object_info.object_id << " " << object_rdma_info[i].object_sizes;
			continue;
		}
		else{
			// auto ts_get_obj_remote_rdma = current_sys_time_us();
			// RAY_LOG(DEBUG) << "get object id from queue start " << object_rdma_info[i].object_info.object_id << " " << ts_get_obj_remote_rdma;
			object_rdma_queue_.enqueue(object_rdma_info[i]);
			// auto ts_get_obj_remote_rdma = current_sys_time_us();
			// RAY_LOG(ERROR) << "raylet client send 1 " << " " << ts_get_obj_remote_rdma << " " << object_rdma_info[i].object_info.object_id ;

		}
	}
	cv_.notify_all();
};

void ObjectManagerRdma::RunRpcService(int index) {
  SetThreadName("rpc.obj.mgr.rdma." + std::to_string(index));
  rpc_service_.run();
}

void ObjectManagerRdma::StartRpcService() {
  rpc_threads_.resize(rpc_service_threads_number_);
  for (int i = 0; i < rpc_service_threads_number_; i++) {
    rpc_threads_[i] = std::thread(&ObjectManagerRdma::RunRpcService, this, i);
  }
  object_manager_rdma_server_.RegisterService(object_manager_rdma_service_);
  object_manager_rdma_server_.Run();
}

void ObjectManagerRdma::StopRpcService() {
  rpc_service_.stop();
  for (int i = 0; i < rpc_service_threads_number_; i++) {
    rpc_threads_[i].join();
  }
  object_manager_rdma_server_.Shutdown();
}

void ObjectManagerRdma::HandleGetObject(const ray::rpc::GetObjectRequest &request,
								ray::rpc::GetObjectReply *reply,
								ray::rpc::SendReplyCallback send_reply_callback) {

	std::vector<ObjectRdmaInfo> object_rdma_info;
	for (int i = 0; i<request.object_ids().size(); ++i) {
		ObjectRdmaInfo obj_rdma_info;
		obj_rdma_info.object_virt_address = request.virt_address().Get(i);
    obj_rdma_info.object_sizes = request.object_size().Get(i) + request.meta_size().Get(i);
    obj_rdma_info.object_address = request.owner_ip_address().Get(i);
    obj_rdma_info.object_info.object_id = ray::ObjectID::FromBinary(request.object_ids().Get(i));
    obj_rdma_info.object_info.data_size = request.object_size().Get(i);
    obj_rdma_info.object_info.metadata_size = request.meta_size().Get(i);
    obj_rdma_info.object_info.owner_raylet_id = ray::NodeID::FromBinary(request.owner_raylet_id().Get(i));
    obj_rdma_info.object_info.owner_ip_address = request.owner_ip_address().Get(i);
    obj_rdma_info.object_info.owner_port = request.owner_port().Get(i);
    obj_rdma_info.object_info.owner_worker_id = ray::WorkerID::FromBinary(request.owner_worker_id().Get(i));
    obj_rdma_info.rem_ip_address = request.rem_ip_address().Get(i);

    object_rdma_info.emplace_back(obj_rdma_info);
	}
	InsertObjectInQueue(object_rdma_info);
  // for (const auto &e : request.object_ids()) {
  //   object_ids.emplace_back(ObjectID::FromBinary(e));
  // }
	send_reply_callback(ray::Status::OK(), nullptr, nullptr);
}



void ObjectManagerRdma::StartRdmaService() {
	object_threads_.resize(rpc_service_threads_number_);
	for (int i = 0; i < rpc_service_threads_number_; i++) {
		object_threads_[i] = std::thread(&ObjectManagerRdma::RunRdmaService, this, i);
	}
}


void ObjectManagerRdma::StopRdmaService() {
  for (int i = 0; i < rpc_service_threads_number_; i++) {
    object_threads_[i].join();
  }
}

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
        struct pingpong_dest *rem_dest = new pingpong_dest[num_qp_pair];
        struct pingpong_dest *my_dest = new pingpong_dest[num_qp_pair];
        struct pingpong_context *ctx = new pingpong_context[num_qp_pair];
				for(int i = 0; i < num_qp_pair; ++i)
        	InitRdmaCtx(ctx+i, my_dest+i);
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
    struct pingpong_dest *my_dest = new pingpong_dest[num_qp_pair];
    struct pingpong_context *ctx = new pingpong_context[num_qp_pair];
		for(int i=0; i< num_qp_pair; ++i){
			InitRdmaCtx(ctx+i, my_dest+i);
		}
    boost::asio::write(s, boost::asio::buffer(my_dest, sizeof(struct pingpong_dest) * num_qp_pair));
    struct pingpong_dest* rem_dest = new pingpong_dest[num_qp_pair];

    size_t reply_length = boost::asio::read(s,
        boost::asio::buffer(rem_dest, sizeof(struct pingpong_dest) * num_qp_pair));
    // remote_dest_.emplace(ip_address, rem_dest);

		for(int i=0; i< num_qp_pair; ++i){
			RAY_LOG(DEBUG) << "do read remote info remote psn client" << (rem_dest+i)->psn << " remote rkey " << (rem_dest+i)->rkey;
    	CovRdmaStatus(ctx+i, rem_dest+i, my_dest+i);
		}

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
	StopRdmaService();
	StopRpcService();
}

void ObjectManagerRdma::InitRdmaBaseCfg() {
    cfg_.port = 7000;
    cfg_.ib_devname = "mlx5_1";
    cfg_.ib_port = 1;
    cfg_.size = 4096;
    cfg_.mtu = IBV_MTU_1024;
    cfg_.rx_depth = 1000;
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
  my_dest->rkey = ctx->mr->rkey;
  
  RAY_LOG(DEBUG) << "  local address:  LID " << my_dest->lid << " QPN " <<  my_dest->qpn \
  <<" PSN " << my_dest->psn << " GID " << gid << " local lkey " << ctx->mr->lkey << " remote rkey " << ctx->mr->rkey;
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
  int access_flags =  IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE ;
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
				.max_send_wr  = rx_depth + 1,
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
    if (init_attr.cap.max_inline_data >= plasma_size_)
			ctx->send_flags |= IBV_SEND_INLINE;

	}

	{
		struct ibv_qp_attr attr = {
			.qp_state        = IBV_QPS_INIT,
			.pkey_index      = 0,
			.port_num        = port
		};
    // attr.qp_access_flags = 0;
    // attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE  | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC;
    attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE  | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE ;

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
  RAY_LOG(ERROR) << "Accomplish modify QP to RTS";

	return 0;
}



void ObjectManagerRdma::FreeRdmaResource(struct pingpong_context *ctx) {
		for(int i=0; i<num_qp_pair; i++){
			ibv_destroy_qp((ctx+i)->qp);
			ibv_destroy_cq(pp_cq((ctx+i)));
			ibv_dereg_mr((ctx+i)->mr);
			ibv_free_dm((ctx+i)->dm);
			ibv_dealloc_pd((ctx+i)->pd);
			ibv_destroy_comp_channel((ctx+i)->channel);
			ibv_close_device((ctx+i)->context);
			free((ctx+i));
		}
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

void ObjectManagerRdma::FetchObjectFromRemotePlasma(const std::vector<std::string> &object_address, const std::vector<unsigned long>  &object_virt_address, 
                                                  const std::vector<int>  &object_sizes, std::vector<ray::ObjectInfo> &object_info, const std::vector<std::string> &rem_ip_address, ray::raylet::DependencyManager &dependency_manager) {
  RAY_LOG(DEBUG) << "Starting get object through rdma for worker ";
  auto ts_fetch_object_rdma = current_sys_time_us();
  for(uint64_t i = 0; i < object_address.size(); ++i) {
    std::string address = rem_ip_address[i];
		std::string obj_address = object_address[i];
    const ray::ObjectInfo &obj_info = object_info[i];

		if(object_manager_.CheckInsertObjectInfo(object_info[i].object_id) || object_sizes[i]==0 || (address == local_ip_address_)) {
			RAY_LOG(DEBUG) << " Object is alread in local_object or object size is zero " << object_info[i].object_id << " " << object_sizes[i];
			continue;
		}
    auto it = remote_dest_.find(address);
    if(it!=remote_dest_.end()){
      // continue;
    //   QueryQp(it->second.first.first);

			std::random_device seed;//hardware to generate random seed
			std::ranlux48 engine(seed());//use seed to generate 
			std::uniform_int_distribution<> distrib(0, num_qp_pair-1);//set random min and max
			int n_qp = distrib(engine);//n_qp
			
      auto allocation = object_manager_.AllocateObjectSizeRdma(object_sizes[i]);
      RAY_LOG(DEBUG) << " Allocate space allocation->address " << allocation->address << " object_id " << object_info[i].object_id;

      unsigned long local_address =(unsigned long) allocation->address;
      RAY_LOG(DEBUG) << " Allocate space for rdma object " << local_address;
      RAY_LOG(DEBUG) << " FetchObjectFromRemotePlasma " << local_address << " object_virt_address " << object_virt_address[i] << "  object_sizes " <<  object_sizes[i] << " " << address << " " << object_info[i].object_id << " " << obj_address << " " << n_qp;
      
      PostSend(it->second.first.first + n_qp, it->second.second + n_qp, local_address, object_sizes[i], object_virt_address[i], IBV_WR_RDMA_READ, 0);
      // PollCompletion(it->second.first.first);
      auto ctx =  it->second.first.first + n_qp;
	  
	//   RAY_LOG(DEBUG) << " PostSend object to RDMA ";

      main_service_->post([this, ctx, allocation, obj_info, &dependency_manager, ts_fetch_object_rdma]() { PollCompletion(ctx, allocation, obj_info, &dependency_manager, ts_fetch_object_rdma); },
                    "ObjectManagerRdma.PollCompletion");
      // std::ofstream outfile;
      // std::string filename = "buffer.txt";
      // void *buffer = (void *) local_address;
      // uint8_t *buf = (uint8_t*) buffer;
      // outfile.open(filename);
      // for(int j=0; j<object_sizes[i]; ++j){
      //   outfile<<buf[j];
      // }
      // outfile.close();
	}
  }
  // auto te_fetch_object_rdma = current_sys_time_us();
	// RAY_LOG(DEBUG) << "FetchObjectRdma time " << te_fetch_object_rdma - ts_fetch_object_rdma;
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

int ObjectManagerRdma::PostSend(struct pingpong_context *ctx, struct pingpong_dest *rem_dest, unsigned long buf, int msg_size, unsigned long remote_address, int opcode, int64_t t_index) {
	// RAY_LOG(ERROR) << "PostSend start ";
	struct ibv_send_wr sr;
	struct ibv_send_wr *bad_wr;
	struct ibv_sge sge;
	int rc;
	int flags;	
	memset(&sge, 0, sizeof(sge));
	// sge.addr = (uintptr_t)res->buf;
	sge.addr = buf;
	sge.length = msg_size;
	// RAY_LOG(DEBUG) << " PostSend local lkey " << ctx->mr->lkey << " remote rkey " << ctx->mr->rkey;
	sge.lkey = ctx->mr->lkey;

	memset(&sr, 0, sizeof(sr));
	sr.next = NULL;
	sr.wr_id = t_index;
	sr.sg_list = &sge;
	sr.num_sge = 1;
	sr.opcode = IBV_WR_RDMA_READ;
	sr.send_flags = IBV_SEND_SIGNALED;

	if (opcode != IBV_WR_SEND) {
		sr.wr.rdma.remote_addr = remote_address;
		sr.wr.rdma.rkey	= rem_dest->rkey;
	}
	rc = ibv_post_send(ctx->qp, &sr, &bad_wr);

	if (rc)
      RAY_LOG(ERROR) << "Failed to post sr " << rc;
	// else
  //   RAY_LOG(DEBUG) << "RDMA read request was posted";
  	// RAY_LOG(ERROR) << "PostSend end ";

	return rc;
}

int ObjectManagerRdma::PollCompletion(struct pingpong_context *ctx, const absl::optional<plasma::Allocation> &allocation, const ray::ObjectInfo &object_info, ray::raylet::DependencyManager *dependency_manager, int64_t start_time){
  // RAY_LOG(DEBUG) << "PollCompletion ";
  // auto ts_fetch_rdma = current_sys_time_us();
  struct ibv_wc wc;
	int poll_result;
	int rc = 0;
	do {
		poll_result = ibv_poll_cq(pp_cq(ctx), 1, &wc);
	} while (poll_result==0);
	if (poll_result < 0) {
    RAY_LOG(ERROR) << "poll cq failed";
		rc = 1;
	} else if (poll_result == 0) {
    RAY_LOG(ERROR) << "completion wasn't found in the cq after timeout";
		rc = 1;
	} else {
		// fprintf(stdout, "completion was found in cq with status 0x%x\n", wc.status);
    RAY_LOG(DEBUG) << "completion was found in cq with status " << wc.status;
    if ( wc.status == IBV_WC_SUCCESS) {
			auto tc_fetch_rdma = current_sys_time_us();
			RAY_LOG(DEBUG) << " get object start time end in rdma " << object_info.object_id << " " << tc_fetch_rdma << " " << start_time;

      object_manager_.InsertObjectInfo(allocation, object_info);
			dependency_manager->InsertObjectInfo(object_info);
    }
		if ( wc.status != IBV_WC_SUCCESS) {
			// fprintf(stderr, "got bad completion with status 0x:%x, verdor syndrome: 0x%x\n", wc.status, wc.vendor_err);
      RAY_LOG(ERROR) << "got bad completion with status " << wc.status << " verdor syndrome: " << wc.vendor_err;
      rc = 1;
		} 
	}
  // auto te_fetch_rdma = current_sys_time_us();
  // RAY_LOG(DEBUG) << "Poll Object in Rdma " << te_fetch_rdma - ts_fetch_rdma << " " << te_fetch_rdma - start_time;  
	return rc;
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
