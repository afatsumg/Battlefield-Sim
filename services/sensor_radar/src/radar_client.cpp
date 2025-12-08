#include "radar_client.h"
#include <iostream>

RadarClient::RadarClient(std::shared_ptr<grpc::Channel> channel)
{
    stub_ = fusion::FusionService::NewStub(channel);
    writer_ = stub_->StreamRadar(&context_, &ack_);
}

RadarClient::~RadarClient()
{
    if (writer_)
    {
        writer_->WritesDone();
        grpc::Status status = writer_->Finish();

        if (!status.ok())
        {
            std::cerr << "[RADAR] Stream closed with error: "
                      << status.error_message() << std::endl;
        }
    }
}

bool RadarClient::sendDetection(const sensors::RadarDetection &msg)
{
    if (!writer_)
    {
        std::cerr << "[RADAR] Writer not initialized!" << std::endl;
        return false;
    }

    if (!writer_->Write(msg))
    {
        std::cerr << "[RADAR] Stream write failed!" << std::endl;
        return false;
    }
    return true;
}