#include "threadpool.h"
#include <grpc++/grpc++.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpcpp/security/credentials.h>
#include <iostream>
#include <fstream>
#include <memory>
#include "store.grpc.pb.h"
#include "vendor.grpc.pb.h"

#include <string>

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using store::Store;
using store::ProductReply;
using store::ProductQuery;
using store::ProductInfo;
using vendor::BidQuery;
using vendor::BidReply;
using vendor::Vendor;

using namespace std;

unique_ptr< Vendor::Stub> stubs[5];
unsigned vendors;

class ServerImpl final {

public:
    
    std::unique_ptr<ServerCompletionQueue> cq_;
    Store::AsyncService service_;
    std::unique_ptr<Server> server_;
    
//    Destructor to shutdown the server and completion queue
    ~ServerImpl() {
        server_->Shutdown();
        cq_->Shutdown();
    }

    void Run(string server_address, unsigned max_threads) {

        ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service_);
        cq_ = builder.AddCompletionQueue();
        server_ = builder.BuildAndStart();
        std::cout << "Server listening on " << server_address << std::endl;

        std::ifstream vendor_file;
        std::string vendor_addr;
        std::vector<std::string> vendor_lists;

        vendor_file.open("vendor_addresses.txt");
        while (getline(vendor_file,vendor_addr)){
            vendor_lists.push_back(vendor_addr);

        }
        vendor_file.close();
        vendors = vendor_lists.size();
//        Loading vendor addresses in the array
        for(int i=0; i<vendors ; i++) {
            shared_ptr<grpc::ChannelInterface> channel = grpc::CreateChannel(vendor_lists[i],grpc::InsecureChannelCredentials());
            stubs[i] = Vendor::NewStub(channel);
//            stubs[i]=std::unique_ptr<Vendor::Stub> stub_(Vendor::NewStub
//                                                    (grpc::CreateChannel(vendor_lists.at(i),
//                                                                             grpc::InsecureChannelCredentials())));
        }
//        Handling RPC Calls
        HandleRpcs(max_threads);
    }
    
    void HandleRpcs(unsigned max_threads) {
        
        new CallData(&service_, cq_.get());
        void* tag;
        bool ok;
        cout<<"handle rpcs"<<endl;
        threadpool pool{max_threads};
        while (true) {
//            Spawning over the RPC calls
            GPR_ASSERT(cq_->Next(&tag, &ok));
            GPR_ASSERT(ok);
            pool.addToQueue([tag](){
                static_cast<CallData*>(tag)->Proceed();
            });
        }
    }
    

private:
    
    class CallData {
    private:
        Store::AsyncService* service_;
        ServerCompletionQueue* cqs;
        ServerContext ctx_;
        ClientContext client_ctx_;
        ProductQuery request_;
        ProductReply reply_;
        ServerAsyncResponseWriter<ProductReply> responder_;
        enum CallStatus { CREATE, PROCESS, FINISH };
        CallStatus status_;
        
    public:
        
        CallData(Store::AsyncService* service, ServerCompletionQueue* cq)
                : service_(service), cqs(cq), responder_(&ctx_), status_(CREATE) {
            Proceed();
        }

        void Proceed() {
            if (status_ == CREATE) {
                // Make this instance progress to the PROCESS state.
                status_ = PROCESS;
//                cout<<"Current status=CREATE"<<endl;
                service_->RequestgetProducts(&ctx_, &request_, &responder_, cqs, cqs,
                                          this);
            } else if (status_ == PROCESS) {

                cout<<"Recieved "<<request_.product_name()<<endl;
                new CallData(service_, cqs);
                CompletionQueue cq;
                BidReply bidReply[vendors];
                Status status[vendors];
                for(int i=0; i<vendors; i++) {
//                      For each vendor start an RPC call to get the bids
                    ClientContext cctx;
                    BidQuery bidQuery;
//                    Status status;
                    bidQuery.set_product_name(request_.product_name());
                    std::unique_ptr<ClientAsyncResponseReader<BidReply> > async_rpc(
                            stubs[i]->PrepareAsyncgetProductBid(&cctx, bidQuery, &cq));
                    async_rpc->StartCall();
                    /*std::unique_ptr<ClientAsyncResponseReader<BidReply> > async_rpc(
                            stub_->AsyncgetProductBid(&client_ctx_, bidQuery, &cq));
*/
                    async_rpc->Finish(&bidReply[i], &status[i], (void *) (size_t) i);
                }
                for(int i=0;i<vendors;i++) {
//                    For each vendor receive the response for the BidQuery
                    void *got_tag;
                    bool ok = false;
//                      parse the vendor id and price from the bidreply using got_tag
                    cq.Next(&got_tag, &ok);
                    long int vendo = (long int)got_tag;
                    if(status[vendo].ok()) {
                        if (ok /*&& got_tag == (void *) (size_t) 0*/) {
                            ProductInfo info;
                            cout << "Vendor ID: " << bidReply[vendo].vendor_id() << " ; ";
                            cout << "Bid Price: " << bidReply[vendo].price() << std::endl;
                            info.set_price(bidReply[vendo].price());
                            info.set_vendor_id(bidReply[vendo].vendor_id());
                            reply_.add_products()->CopyFrom(info);
                            //reply_.products(i).vendor_id();
                            //reply_.products(i).price();
                        }
                    }
               /* if (ok && got_tag == (void *) (size_t) 1) {
                    // check reply and status
                    //cout<<"asasasasasasas  "<< *(int*)got_tag <<endl;
                    ProductInfo info;
                    cout << "Vendor ID: " << bidReply[1].vendor_id() << " ; ";
                    cout << "Bid Price: " << bidReply[1].price() << std::endl;
                    info.set_price(bidReply[1].price());
                    info.set_vendor_id(bidReply[1].vendor_id());
                    reply_.add_products()->CopyFrom(info);
                    //reply_.products(i).vendor_id();
                    //reply_.products(i).price();
                }
                if (ok && got_tag == (void *) (size_t) 2) {
                    // check reply and status
                    //cout<<"asasasasasasas  "<< *(int*)got_tag <<endl;
                    ProductInfo info;
                    cout << "Vendor ID: " << bidReply[2].vendor_id() << " ; ";
                    cout << "Bid Price: " << bidReply[2].price() << std::endl;
                    info.set_price(bidReply[2].price());
                    info.set_vendor_id(bidReply[2].vendor_id());
                    reply_.add_products()->CopyFrom(info);
                    //reply_.products(i).vendor_id();
                    //reply_.products(i).price();
                }
                if (ok && got_tag == (void *) (size_t) 3) {
                    // check reply and status
                    //cout<<"asasasasasasas  "<< *(int*)got_tag <<endl;
                    ProductInfo info;
                    cout << "Vendor ID: " << bidReply[3].vendor_id() << " ; ";
                    cout << "Bid Price: " << bidReply[3].price() << std::endl;
                    info.set_price(bidReply[3].price());
                    info.set_vendor_id(bidReply[3].vendor_id());
                    reply_.add_products()->CopyFrom(info);
                    //reply_.products(i).vendor_id();
                    //reply_.products(i).price();
                }
                    if (ok && got_tag == (void *) (size_t) 4) {
                        // check reply and status
                        //cout<<"asasasasasasas  "<< *(int*)got_tag <<endl;
                        ProductInfo info;
                        cout << "Vendor ID: " << bidReply[4].vendor_id() << " ; ";
                        cout << "Bid Price: " << bidReply[4].price() << std::endl;
                        info.set_price(bidReply[4].price());
                        info.set_vendor_id(bidReply[4].vendor_id());
                        reply_.add_products()->CopyFrom(info);
                        //reply_.products(i).vendor_id();
                        //reply_.products(i).price();
                    }
*/
                }
                status_ = FINISH;
                responder_.Finish(reply_, Status::OK, this);
                
            }
            else {
                GPR_ASSERT(status_ == FINISH);
                delete this;
            }
        }

    };

};

int main(int argc, char** argv) {

    string server_address = "0.0.0.0:50056";
    unsigned max_threads = 5;
    if (argc > 1) {
        server_address = argv[1];
        if (argc > 2)
            max_threads = atoi(argv[2]);
    }
    ServerImpl server;
    server.Run(server_address,max_threads);

}
