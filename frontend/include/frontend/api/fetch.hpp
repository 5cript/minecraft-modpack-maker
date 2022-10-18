#pragma once

#include <backend/fetch_options.hpp>

#include <functional>

void fetch(std::string const& url, FetchOptions const& options, std::function<void(FetchResponse const&)> callback);