#include <string>
#include "gridflow/model/Types.h"

namespace gridflow {

    struct Facility {
        std::string id;
        std::string name;

        NodeType type = NodeType::Processing;
        double capacity = 0.0;
        double supply = 0.0;
        double demand = 0.0;
        int priority = 0; //0: not a demand center, 1: critical, 3: lowest
        double x = 0.0; 
        double y = 0.0; //layout pos for frontend
        NodeStatus status = NodeStatus::Active;
        double severity = 0.0; //fraction of supply lost
        double surge = 0.0;//fractional increase in demand
        double effectiveSupply = 0.0; //supply after disruption
        double effectiveDemand = 0.0; //demand after surge
        double served = 0.0;
        double shortage = 0.0;
        double throughput = 0.0; //total flow through this node
    };
    
}