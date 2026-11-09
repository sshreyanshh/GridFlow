#include "httplib.h"
#include "json.hpp"

int main() {
    httplib::Server svr;
    svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        nlohmann::json body = {{"ok", true}, {"data", {{"service", "gridflow"}}}};
        res.set_content(body.dump(), "application/json");
    });
    svr.listen("127.0.0.1", 8080);
}