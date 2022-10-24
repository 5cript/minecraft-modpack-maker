#pragma once

#ifndef __kernel_entry
#    define __kernel_entry
#endif
#include <boost/process.hpp>

#include <csignal>
#include <iostream>
#include <memory>

class Minecraft
{
  public:
    Minecraft();
    void start(std::string const& serverJar);
    bool stop(int waitTimeoutSeconds = 120);
    void forwardIo();

  private:
    std::unique_ptr<boost::process::child> process_;
    boost::asio::streambuf inputBuffer_;
};
