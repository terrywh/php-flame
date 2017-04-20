#pragma once

#include <cstdint>
#include <cerrno>
#include <memory>
#include <thread>
#include <vector>

#include <sys/signalfd.h>
#include <signal.h>
#include <unistd.h>

#include <phpext.h>
#include <libmill.h>