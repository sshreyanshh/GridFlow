#include "gridflow/api/Server.h"

#include "gridflow/api/Routes.h"

namespace gridflow {

Server::Server(Simulation& simulation) { registerRoutes(server_, simulation); }
bool Server::listen(const std::string& host, int port) { return server_.listen(host, port); }
int Server::bindToAnyPort(const std::string& host) { return server_.bind_to_any_port(host); }
void Server::listenAfterBind() { server_.listen_after_bind(); }
void Server::stop() { server_.stop(); }

}  // namespace gridflow
