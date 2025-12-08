#pragma once
#include "fusion/fusion.grpc.pb.h"
#include "sensors/radar.pb.h"
#include <grpcpp/grpcpp.h>
#include <memory>

class RadarClient
{
public:
    explicit RadarClient(std::shared_ptr<grpc::Channel> channel);
    ~RadarClient();

    bool sendDetection(const sensors::RadarDetection &msg);

private:
    std::unique_ptr<fusion::FusionService::Stub> stub_;
    std::unique_ptr<grpc::ClientWriter<sensors::RadarDetection>> writer_;
    grpc::ClientContext context_;
    fusion::FusionAck ack_;
};