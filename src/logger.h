#pragma once

#include <Windows.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

namespace logger = SKSE::log;

void SetupLog();