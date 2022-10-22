#pragma once

#include <backend/archive/streaming_provider.hpp>
#include <roar/curl/sink.hpp>

#include <future>
#include <mutex>

class TarExtractorSink : public Roar::Curl::Sink
{
  public:
    TarExtractorSink(std::filesystem::path targetDirectory);
    ~TarExtractorSink();
    void feed(char const* buffer, std::size_t amount) override;
    void finalize();

  private:
    std::once_flag startFlag_;
    std::promise<void> stopSignaler_;
    std::shared_future<void> stopToken_;
    Archive::StreamingDataProvider streamingDataProvider_;
    Archive::DataDistributor dataDistributor_;
    Archive::Reader reader_;
};