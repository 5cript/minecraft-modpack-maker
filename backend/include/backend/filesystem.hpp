#pragma once

#include <nui/backend/rpc_hub.hpp>

class FileSystem
{
  public:
    static void registerAll(Nui::RpcHub const& hub);

  private:
    static void registerReadFile(Nui::RpcHub const& hub);
    static void registerWriteFile(Nui::RpcHub const& hub);
    static void registerCreateDirectory(Nui::RpcHub const& hub);
    static void registerFileExists(Nui::RpcHub const& hub);
};