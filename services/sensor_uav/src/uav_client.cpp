#include "uav_client.h"
#include <iostream>

UAVClient::UAVClient(std::shared_ptr<grpc::Channel> channel)
{
    stub_ = fusion::FusionService::NewStub(channel);
    writer_ = stub_->StreamUAV(&context_, &ack_);
}

UAVClient::~UAVClient()
{
    if (writer_)
    {
        writer_->WritesDone();

        // Check server's final status
        grpc::Status status = writer_->Finish();

        if (status.ok())
        {
            std::cout << "[UAV] Stream closed successfully. Server ACK: "
                      << ack_.message() << std::endl;
        }
        else
        {
            std::cerr << "[UAV] Stream closed with error: "
                      << status.error_code() << ": " << status.error_message() << std::endl;
        }
    }
}

bool UAVClient::sendTelemetry(const sensors::UAVTelemetry &msg)
{
    if (!writer_)
    {
        std::cerr << "[UAV] Writer not initialized!" << std::endl;
        return false;
    }
    // If writing fails, the stream is likely broken
    if (!writer_->Write(msg))
    {
        std::cerr << "[UAV] Stream write failed!" << std::endl;
        return false;
    }
    return true;
}