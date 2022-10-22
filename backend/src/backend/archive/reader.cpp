#include <backend/archive/reader.hpp>

#include <backend/archive/archive.hpp>
#include <backend/archive/entry.hpp>

#include <chrono>
#include <mutex>
#include <optional>
#include <string_view>
#include <thread>

namespace Archive
{
    namespace
    {
        int onArchiveOpen(struct archive*, void* provider)
        {
            reinterpret_cast<DataProvider*>(provider)->initialize();
            return ARCHIVE_OK;
        }
        int onArchiveClose(struct archive*, void* provider)
        {
            reinterpret_cast<DataProvider*>(provider)->finalize();
            return ARCHIVE_OK;
        }
        la_ssize_t onArchiveRead(struct archive*, void* provider, const void** buffer)
        {
            return reinterpret_cast<DataProvider*>(provider)->read(*buffer);
        }
    }

    struct Reader::Implementation
    {
        std::unique_ptr<Archive> archive_;
        DataProvider* provider;
        DataReceiver* receiver;
        std::thread reader;
        std::atomic_bool internalStopRequested;

        Implementation(DataProvider* provider, DataReceiver* receiver)
            : archive_{std::make_unique<ArchiveReader>()}
            , provider{provider}
            , receiver{receiver}
            , reader{}
        {
            ::archive_read_support_filter_all(*archive_);
            ::archive_read_support_format_tar(*archive_);
        }
    };

    Reader::Reader(DataProvider* provider, DataReceiver* receiver)
        : impl_{std::make_unique<Implementation>(provider, receiver)}
    {}

    Reader::~Reader()
    {
        awaitRead();
    }

    void Reader::readAsync(std::shared_future<void> externalStopToken, std::chrono::seconds stopTokenTimeout)
    {
        impl_->reader = std::thread{[this, externalStopToken = std::move(externalStopToken), stopTokenTimeout]() {
            // This has to be done in the read thread, because the open may call the read function
            // already. Causing this to block if called from the constructor.
            auto result =
                ::archive_read_open(*impl_->archive_, impl_->provider, &onArchiveOpen, &onArchiveRead, &onArchiveClose);
            if (result != ARCHIVE_OK)
            {
                impl_->receiver->onError(Error(*impl_->archive_, result));
                return;
            }

            bool stopFlag = false;
            std::optional<std::chrono::system_clock::time_point> externalStopRequestedTime;
            auto shallStop = [this, &externalStopToken, &stopFlag, &externalStopRequestedTime, &stopTokenTimeout]() {
                if (!externalStopRequestedTime)
                {
                    if (externalStopToken.wait_for(std::chrono::seconds{0}) == std::future_status::ready)
                        externalStopRequestedTime = std::chrono::system_clock::now();
                }

                const auto shall = impl_->internalStopRequested ||
                    (externalStopRequestedTime &&
                     (externalStopRequestedTime.value() < std::chrono::system_clock::now() - stopTokenTimeout));
                if (shall && !stopFlag)
                    impl_->receiver->onAbort();
                stopFlag |= shall;
                return shall;
            };

            auto readEntryHeader = [this](archive_entry*& entry) {
                auto result = archive_read_next_header(*impl_->archive_, &entry);
                if (result != ARCHIVE_OK)
                {
                    if (result == ARCHIVE_EOF)
                        impl_->receiver->onComplete();
                    else
                        impl_->receiver->onError(Error(*impl_->archive_, result));
                    return false;
                }
                return true;
            };

            archive_entry* entry;
            char buffer[4096];
            while (!shallStop() && readEntryHeader(entry))
            {
                auto wrapped = Entry{entry};
                impl_->receiver->onNewEntry(wrapped);
                wrapped.release();
                ssize_t amountRead = 0;
                do
                {
                    amountRead = archive_read_data(*impl_->archive_, &buffer, 4096);
                    switch (amountRead)
                    {
                        case (ARCHIVE_RETRY):
                            continue;
                        case (ARCHIVE_WARN):
                        {
                            impl_->receiver->onError(Error(*impl_->archive_, static_cast<int>(amountRead)));
                            return;
                        }
                        case (ARCHIVE_FATAL):
                        {
                            impl_->receiver->onError(Error(*impl_->archive_, static_cast<int>(amountRead)));
                            return;
                        }
                        default:;
                    }
                    if (amountRead > 0)
                        impl_->receiver->onData(std::string_view{buffer, static_cast<unsigned long>(amountRead)});
                } while (!shallStop() && amountRead > 0);

                if (!stopFlag)
                    impl_->receiver->onEntryComplete();
                else
                    break;
            }
        }};
    }
    void Reader::awaitRead()
    {
        impl_->internalStopRequested = true;
        if (impl_->reader.joinable())
            impl_->reader.join();
    }
}
