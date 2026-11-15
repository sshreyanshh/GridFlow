#pragma once

#include "httplib.h"
#include "gridflow/sim/Simulation.h"

namespace gridflow {

void registerRoutes(httplib::Server& server, Simulation& simulation);

}  // namespace gridflow
