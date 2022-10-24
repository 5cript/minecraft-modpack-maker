#include <update_server/minecraft.hpp>

#include <cstdlib>
#include <iostream>

using namespace std::string_literals;

Minecraft::Minecraft()
{}

void Minecraft::start(std::string const& serverJar)
{
    namespace bp = boost::process;

    process_ = std::make_unique<bp::child>(
        "java -jar "s + serverJar, bp::std_out > stdout, bp::std_err > stderr, bp::std_in < stdin);
}
bool Minecraft::stop(int waitTimeoutSeconds)
{
#ifdef _WIN32
    process_->terminate();
#else
    kill(process_->id(), SIGINT);
#endif
    for (int i = 0; i != waitTimeoutSeconds; ++i)
    {
        std::cout << "Waiting for " << i << " seconds for minecraft to shutdown...\n";
        if (process_->wait_for(std::chrono::seconds(1)))
        {
            return true;
        }
    }
    return false;
}
void Minecraft::forwardIo()
{
    std::string line;
    do
    {
        std::getline(std::cin, line);
    } while (line != "/stop");
}