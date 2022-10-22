#pragma once

#include "reader.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace Archive
{
    class StreamingDataProvider : public Archive::DataProvider
    {
      public:
        static constexpr std::chrono::seconds bufferUnderrunTimeLimit{60};
        static constexpr std::chrono::seconds bufferOverflowTimeLimit{60};
        static constexpr std::chrono::seconds stopGracePeriod{10};
        static constexpr std::size_t maxBufferChunks = 100;

        StreamingDataProvider(std::shared_future<void> externalStopToken)
            : bufferGuard{}
            , shallStopGuard{}
            , bufferQueue{}
            , externalStopToken{std::move(externalStopToken)}
            , stopDetectedTime{}
            , bufferAvailability{}
            , processedBytes{0}
        {}
        ssize_t read(void const*& buffer) override
        {
            bool isQueueEmpty = false;
            {
                std::scoped_lock lock{bufferGuard};
                isQueueEmpty = bufferQueue.empty();
            }

            if (isQueueEmpty)
            {
                auto awaitDataAvailability = [this]() {
                    std::unique_lock lock{bufferGuard};
                    if (!bufferQueue.empty())
                        return true;
                    return bufferAvailability.wait_for(lock, std::chrono::seconds{1}, [this] {
                        return !bufferQueue.empty();
                    });
                };

                auto timeBeforeWait = std::chrono::system_clock::now();
                while (!shallStop() && !awaitDataAvailability())
                {
                    if (std::chrono::system_clock::now() - timeBeforeWait >= bufferUnderrunTimeLimit)
                    {
                        buffer = nullptr;
                        // 0 is interpreted as EOF.
                        return 0;
                    }
                }
                if (shallStop())
                {
                    buffer = nullptr;
                    return 0;
                }
            }

            return readOnce(buffer);
        };
        bool push(char const* buffer, std::size_t amount)
        {
            return push(std::vector<char>(buffer, buffer + amount));
        }
        bool push(std::vector<char> block)
        {
            auto awaitBufferSpace = [this]() {
                std::unique_lock lock{bufferGuard};
                return overflowProtection.wait_for(lock, std::chrono::seconds{1}, [this] {
                    return bufferQueue.size() < maxBufferChunks;
                });
            };

            auto timeBeforeWait = std::chrono::system_clock::now();
            while (!shallStop() && !awaitBufferSpace())
            {
                if (std::chrono::system_clock::now() - timeBeforeWait >= bufferOverflowTimeLimit)
                    return false;
            }
            if (shallStop())
                return false;
            std::unique_lock lock{bufferGuard};
            processedBytes += block.size();
            bufferQueue.push_back(std::move(block));
            bufferAvailability.notify_one();
            return true;
        }

        std::uint64_t getProcessedByteAmount() const
        {
            return processedBytes;
        }

      private:
        ssize_t readOnce(void const*& buffer)
        {
            std::scoped_lock lock{bufferGuard};
            if (bufferQueue.empty())
            {
                buffer = nullptr;
                return std::size_t{0};
            }
            lastReadElement = std::move(bufferQueue.front());
            bufferQueue.pop_front();
            buffer = &lastReadElement.front();
            overflowProtection.notify_one();
            return static_cast<ssize_t>(lastReadElement.size());
        }

        bool shallStop()
        {
            std::scoped_lock lock{shallStopGuard};

            if (!externalStopToken.valid())
                return true;

            using namespace std::chrono_literals;
            if (!stopDetectedTime && externalStopToken.wait_for(0s) == std::future_status::ready)
                stopDetectedTime = std::chrono::system_clock::now();

            std::scoped_lock lock2{bufferGuard};
            return stopDetectedTime &&
                ((stopDetectedTime.value() < std::chrono::system_clock::now() - stopGracePeriod) ||
                 bufferQueue.empty());
        };

      private:
        std::mutex bufferGuard;
        std::mutex shallStopGuard;
        // Element stability and queue characteristics required.
        std::list<std::vector<char>> bufferQueue;
        std::shared_future<void> externalStopToken;
        std::optional<std::chrono::system_clock::time_point> stopDetectedTime;
        std::condition_variable bufferAvailability;
        std::condition_variable overflowProtection;
        std::vector<char> lastReadElement;
        std::atomic_uint64_t processedBytes;
    };

    /**
     * @brief This class takes the data and extracts it to the respective places relative to the given
     * basePath.
     */
    class DataDistributor : public Archive::DataReceiver
    {
      public:
        DataDistributor()
            : basePath_{}
            , currentPath__{}
            , currentFile_{}
            , isInErrorState_{false}
        {}
        DataDistributor(std::filesystem::path basePath)
            : basePath_{std::move(basePath)}
            , currentPath__{}
            , currentFile_{}
            , isInErrorState_{false}
        {}

        void setBasePath(std::filesystem::path const& basePath)
        {
            basePath_ = basePath;
        }

        /**
         * @brief Check if the error flag was set by the onError event.
         */
        bool isInErrorState() const
        {
            return isInErrorState_;
        }

        void onNewEntry(Archive::Entry const& entry) override
        {
            if (isInErrorState_)
                return;

            const auto path = entry.getPathname();
            if (entry.getType() == Archive::Entry::Type::Directory)
                std::filesystem::create_directory(basePath_ / path);
            if (entry.getType() == Archive::Entry::Type::RegularFile)
            {
                currentPath__ = std::filesystem::weakly_canonical(basePath_ / path);
                currentFile_ = std::ofstream{currentPath__, std::ios_base::binary};
                if (!currentFile_.is_open())
                    isInErrorState_ = true;
            }
        }

        void onData(std::string_view data) override
        {
            if (isInErrorState_)
                return;

            if (currentFile_.good())
                currentFile_.write(data.data(), static_cast<std::streamsize>(data.size()));
            else
                isInErrorState_ = true;
        }

        void onEntryComplete() override
        {
            currentFile_.close();
        }

        void onComplete() override
        {}

        void onError(Archive::Error const& error) override
        {
            std::cerr << "Error: " << error.what() << std::endl;
            isInErrorState_ = true;
            currentFile_.close();
        }

        void onAbort() override{};

      private:
        std::filesystem::path basePath_;
        std::filesystem::path currentPath__;
        std::ofstream currentFile_;
        std::atomic_bool isInErrorState_;
    };
}