#pragma once

#include "entry.hpp"
#include "error.hpp"

#include <functional>
#include <future>
#include <memory>
#include <string>

namespace Archive
{
    /**
     * @brief Derive from this class in order to provide data for a tar reader.
     */
    class DataProvider
    {
      public:
        virtual ~DataProvider() = default;

        /**
         * @brief Called before the first bytes are read, can be used for setup/initialization.
         */
        virtual void initialize(){};

        /**
         * @brief Called when the archive was read completely.
         */
        virtual void finalize(){};

        /**
         * @brief Set the buffer parameter to a valid buffer containing new data and return its size.
         *
         * @param buffer An out parameter. Set this to a buffer.
         * @return std::size_t Size of readable bytes in the buffer.
         */
        virtual ssize_t read(void const*& buffer) = 0;
    };

    /**
     * @brief A convenience DataProvider that can take lambdas for the events.
     */
    struct DynamicDataProvider : public DataProvider
    {
        std::function<void()> initializeCallback{[]() {}};
        std::function<void()> finalizeCallback{[]() {}};
        std::function<ssize_t(void const*&)> reader{[](void const*&) {
            return ssize_t{0};
        }};

        void initialize() override
        {
            initializeCallback();
        }
        void finalize() override
        {
            finalizeCallback();
        }
        ssize_t read(void const*& buffer) override
        {
            return reader(buffer);
        }
    };

    /**
     * @brief Implement this interface to receive files and directories from the tar.
     */
    class DataReceiver
    {
      public:
        virtual ~DataReceiver() = default;

        /**
         * @brief This function is called whenever a new file/directory/... is encountered in the
         * archive.
         *
         * @param entry A class that contains info about the entry.
         */
        virtual void onNewEntry(Entry const& entry) = 0;

        /**
         * @brief This function is called with data that belongs to the previous entry.
         *
         * @param data A string_view encompassing the data.
         */
        virtual void onData(std::string_view data) = 0;

        /**
         * @brief This function is called when an entry completes (like a file).
         */
        virtual void onEntryComplete() = 0;

        /**
         * @brief This function is called when the entire operation completes.
         */
        virtual void onComplete() = 0;

        /**
         * @brief This function is called when an error occurs during extraction. The extraction is
         * aborted when this is called.
         */
        virtual void onError(Error const& error) = 0;

        /**
         * @brief This function is called when the extraction operation is aborted.
         */
        virtual void onAbort() = 0;
    };

    /**
     * @brief A convenience DataReceiver that can take lambdas for the events.
     */
    struct DynamicDataReceiver : public DataReceiver
    {
        std::function<void(Entry const&)> onNewEntryCallback{[](Entry const&) {}};
        std::function<void(std::string_view)> onDataCallback{[](std::string_view) {}};
        std::function<void()> onEntryCompleteCallback{[]() {}};
        std::function<void()> onCompleteCallback{[]() {}};
        std::function<void(Error const& error)> onErrorCallback{[](Error const&) {}};
        std::function<void()> onAbortCallback{[]() {}};

        void onNewEntry(Entry const& entry) override
        {
            onNewEntryCallback(entry);
        }
        void onData(std::string_view data) override
        {
            onDataCallback(data);
        }
        void onEntryComplete() override
        {
            onEntryCompleteCallback();
        }
        void onComplete() override
        {
            onCompleteCallback();
        }
        void onError(Error const& error) override
        {
            onErrorCallback(error);
        }
        void onAbort() override
        {
            onAbortCallback();
        }
    };

    /**
     * @brief This class can be used to asynchronously decompress streaming tar archives.
     *
     */
    class Reader
    {
      public:
        constexpr static std::size_t copyBufferSize = 4096;
        constexpr static std::chrono::seconds externalStopRequestedTimeout = std::chrono::seconds(30);

      public:
        /**
         * @param receiver A receiver structure that receives archive entries and their data.
         */
        Reader(DataProvider* provider, DataReceiver* receiver);
        ~Reader();

        /**
         * @brief Read data asynchronously.
         *
         * @param externalStopToken A stop token that can be used to shut down the operation.
         */
        void readAsync(
            std::shared_future<void> externalStopToken,
            std::chrono::seconds stopTokenTimeout = externalStopRequestedTimeout);

        /**
         * @brief Wait for the previous read operation to complete.
         */
        void awaitRead();

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}
