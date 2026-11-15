#pragma once

#include <string>

#include "httplib.h"
#include "gridflow/sim/Simulation.h"

namespace gridflow {

class Server {
public:
    explicit Server(Simulation& simulation);
    bool listen(const std::string& host, int port);
    int bindToAnyPort(const std::string& host);
    void listenAfterBind();
    void stop();

private:
    httplib::Server server_;
};

}  // namespace gridflow
