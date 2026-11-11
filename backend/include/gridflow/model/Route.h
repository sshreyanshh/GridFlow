#include <string>
#include "gridflow/model/Types.h"

namespace gridflow {

    struct Route {
        std::string id;
        int from = -1; //source node index
        int to = -1; //destination node index
        double capacity = 0.0;
        double cost = 0.0;
        double transitTime = 0.0;

        EdgeStatus status = EdgeStatus::Active;
        double severity = 0.0; //fraction of capacity lost
        double effectiveCapacity = 0.0; //capacity after disruption
        double flow = 0.0; //flow through this edge
    };

}