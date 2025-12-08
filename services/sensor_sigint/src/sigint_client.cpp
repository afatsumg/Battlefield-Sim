#include "sigint_client.h"
#include <iostream>

SigintClient::SigintClient(std::shared_ptr<grpc::Channel> channel)
{
    stub_ = fusion::FusionService::NewStub(channel);
    writer_ = stub_->StreamSigint(&context_, &ack_);
}

SigintClient::~SigintClient()
{
    if (writer_)
    {
        writer_->WritesDone();                   // Tell server we're done writing
        grpc::Status status = writer_->Finish(); // Wait for server's final response

        if (status.ok())
        {
            std::cout << "[SIGINT] Stream closed successfully. Server says: "
                      << ack_.message() << std::endl;
        }
        else
        {
            std::cerr << "[SIGINT] Stream closed with error: "
                      << status.error_code() << ": " << status.error_message() << std::endl;
        }
    }
}

bool SigintClient::sendHit(const sensors::SigintHit &msg)
{
    if (!writer_)
    {
        std::cerr << "[SIGINT] Writer not initialized!" << std::endl;
        return false;
    }

    // If writing fails, the stream is likely broken
    if (!writer_->Write(msg))
    {
        std::cerr << "[SIGINT] Lost connection to Fusion Service!" << std::endl;
        return false;
    }
    return true;
}