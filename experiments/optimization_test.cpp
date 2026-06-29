// Minimal C++ smoke target for shared-variable-aware CC helper compilation.

#include <iostream>
#include <vector>

#include "../cc/soft_cc_update.cpp"

int main() {
    std::vector<double> context{0.0, 0.0, 0.0};
    const std::vector<double> candidate{1.0, 2.0, 3.0};
    const std::vector<int> variables{1, 2};
    const std::vector<double> shared_confidence{0.0, 0.5, 1.0};

    cwvig::blend_shared_variables(context, candidate, variables, shared_confidence, 0.1, 0.8);
    std::cout << context[0] << " " << context[1] << " " << context[2] << "\n";
    return 0;
}
