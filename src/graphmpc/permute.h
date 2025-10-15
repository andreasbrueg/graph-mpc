#pragma once

#include "function.h"

class Permute : public Function {
   public:
    Permute(size_t f_id, ProtocolConfig *conf, std::vector<Ring> input, std::vector<Ring> perm, std::vector<Ring> output, bool inverse)
        : Function(f_id, conf, {}, {}, input, perm, output), inverse(inverse) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        if (inverse) {
            std::vector<Ring> inverse_vec(input2.size());
#pragma omp parallel for if (size > 10000)
            for (size_t i = 0; i < input2.size(); ++i) {
                inverse_vec[input2[i]] = i;
            }
#pragma omp parallel for if (size > 10000)
            for (size_t i = 0; i < inverse_vec.size(); ++i) {
                output[inverse_vec[i]] = input[i];
            }
        } else {
#pragma omp parallel for if (size > 10000)
            for (size_t i = 0; i < input2.size(); ++i) {
                output[input2[i]] = input[i];
            }
        }
    }

   private:
    bool inverse;
};
