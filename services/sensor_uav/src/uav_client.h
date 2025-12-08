#pragma once

#include "fusion/fusion.grpc.pb.h"
#include "sensors/uav.pb.h"
#include <grpcpp/grpcpp.h>
#include <memory>

class UAVClient
{
public:
    explicit UAVClient(std::shared_ptr<grpc::Channel> channel);
    ~UAVClient();

    bool sendTelemetry(const sensors::UAVTelemetry &msg);

private:
    std::unique_ptr<fusion::FusionService::Stub> stub_;
    std::unique_ptr<grpc::ClientWriter<sensors::UAVTelemetry>> writer_;
    grpc::ClientContext context_;
    fusion::FusionAck ack_;
};