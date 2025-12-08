#pragma once

#include "fusion/fusion.grpc.pb.h"
#include "sensors/sigint.pb.h"
#include <grpcpp/grpcpp.h>
#include <memory>

class SigintClient
{
public:
    explicit SigintClient(std::shared_ptr<grpc::Channel> channel);
    ~SigintClient();
    bool sendHit(const sensors::SigintHit &msg);

private:
    std::unique_ptr<fusion::FusionService::Stub> stub_;
    std::unique_ptr<grpc::ClientWriter<sensors::SigintHit>> writer_;
    grpc::ClientContext context_;
    fusion::FusionAck ack_;
};
