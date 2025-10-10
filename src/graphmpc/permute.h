#pragma once

#include "function.h"

class Permute : public Function {
   public:
    Permute(ProtocolConfig *conf, std::vector<Ring> *input, std::vector<Ring> *output, std::vector<Ring> *perm, bool inverse)
        : Function(conf, {}, {}, input, output), perm(perm), inverse(inverse) {
        output->resize(input->size());
    }

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        std::vector<Ring> result(size);
        if (inverse) {
            std::vector<Ring> inverse_vec(perm->size());
#pragma omp parallel for if (size > 10000)
            for (size_t i = 0; i < perm->size(); ++i) {
                inverse_vec[perm->at(i)] = i;
            }
#pragma omp parallel for if (size > 10000)
            for (size_t i = 0; i < inverse_vec.size(); ++i) {
                result[inverse_vec[i]] = input->at(i);
            }
        } else {
#pragma omp parallel for if (size > 10000)
            for (size_t i = 0; i < perm->size(); ++i) {
                result[perm->at(i)] = input->at(i);
            }
        }
        *output = result;
    }

   private:
    std::vector<Ring> *perm;
    bool inverse;
};
