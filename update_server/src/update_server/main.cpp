#include <update_server/update_provider.hpp>

#include <roar/server.hpp>
#include <roar/utility/scope_exit.hpp>
#include <roar/utility/shutdown_barrier.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/thread_pool.hpp>

#include <cxxopts.hpp>

#include <iostream>

constexpr int port = 25002;

struct ProgramOptions
{
    std::filesystem::path serverDirectory;
};

ProgramOptions parseOptions(int argc, char** argv);

int main(int argc, char** argv)
{
    ProgramOptions options = parseOptions(argc, argv);

    boost::asio::thread_pool pool{4};

    // Create server.
    Roar::Server server{{.executor = pool.executor()}};

    const auto shutdownPool = Roar::ScopeExit{[&pool]() {
        pool.stop();
        pool.join();
    }};

    server.installRequestListener<UpdateProvider>(options.serverDirectory);

    // Start server and bind on port "port".
    server.start(port);

    std::cout << "Running on: " << port << "\n";

    // Prevent exit somehow:
    std::cin.get();
    // Roar::ShutdownBarrier barrier;
    // barrier.wait();
}

ProgramOptions parseOptions(int argc, char** argv)
{
    cxxopts::Options options("update_server", "Update server for minecraft servers");
    options.add_options()("s,server-directory", "Server directory", cxxopts::value<std::string>());
    auto result = options.parse(argc, argv);
    return ProgramOptions{
        .serverDirectory = result["server-directory"].as<std::string>(),
    };
}