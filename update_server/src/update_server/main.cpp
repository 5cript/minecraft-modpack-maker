#include <update_server/update_provider.hpp>

#include <roar/server.hpp>
#include <roar/utility/scope_exit.hpp>
#include <roar/utility/shutdown_barrier.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/program_options.hpp>

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
    ProgramOptions options{
        // assume "this" directory.
        .serverDirectory = std::filesystem::path{argv[0]}.parent_path(),
    };

    namespace po = boost::program_options;
    po::options_description desc("Allowed options");
    desc.add_options()("help", "produce help message")(
        "server-directory,s",
        po::value<std::filesystem::path>(&options.serverDirectory)->required(),
        "server directory");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        std::exit(0);
    }
    po::notify(vm);
    return options;
}