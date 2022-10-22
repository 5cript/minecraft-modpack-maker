#include <backend/tar_extractor_sink.hpp>

TarExtractorSink::TarExtractorSink(std::filesystem::path targetDirectory)
    : startFlag_{}
    , stopSignaler_{}
    , stopToken_{stopSignaler_.get_future().share()}
    , streamingDataProvider_{stopToken_}
    , dataDistributor_{std::move(targetDirectory)}
    , reader_{&streamingDataProvider_, &dataDistributor_}
{}
TarExtractorSink::~TarExtractorSink()
{
    if (stopToken_.valid() && stopToken_.wait_for(std::chrono::seconds{0}) != std::future_status::ready)
        stopSignaler_.set_value();
}
void TarExtractorSink::feed(char const* buffer, std::size_t amount)
{
    std::call_once(startFlag_, [this]() {
        reader_.readAsync(stopToken_);
    });

    streamingDataProvider_.push(buffer, amount);
}
void TarExtractorSink::finalize()
{
    stopSignaler_.set_value();
    reader_.awaitRead();
}