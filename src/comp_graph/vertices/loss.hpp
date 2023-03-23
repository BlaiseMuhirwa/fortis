#pragma once

#include <_types/_uint32_t.h>
#include <algorithm>
#include <cereal/types/base_class.hpp>
#include <cereal/types/polymorphic.hpp>
#include <memory>
#include <optional>
#include <src/cereal/access.hpp>
#include <src/comp_graph/vertices/activ_functions.hpp>
#include <src/comp_graph/vertices/vertex.hpp>
#include <stdexcept>
#include <vector>

namespace fortis::comp_graph {

using fortis::comp_graph::SoftMaxActivation;
using fortis::comp_graph::Vertex;
using fortis::comp_graph::VertexPointer;

static float softmax(float current_logit, const std::vector<float> &input) {
  float sum = 0.0f;
  std::for_each(input.begin(), input.end(),
                [&sum](float logit) { sum += exp(logit); });
  return exp(current_logit) / sum;
}

/**
 * Here we use the cross-entropy loss always with the softmax activation
 * For more about the cross-entropy loss with the Softmax activation
 * check out this page:
 * https://d2l.ai/chapter_linear-classification/softmax-regression.html#the-softmax
 *
 */
class CrossEntropyLoss final
    : public Vertex,
      public std::enable_shared_from_this<CrossEntropyLoss> {

  /*
   * The constructor expects input vertex to have an output with
   * the same dimension as the label. The output vector computed
   * by the input vertex consists of logits prior to a softmax
   * operation.
   */
  CrossEntropyLoss(VertexPointer input_vertex, std::vector<float> &label)
      : _input(std::move(input_vertex)), _label(std::move(label)) {
    auto probabilities_vector_size = _input->getOutputSize();
    if (probabilities_vector_size != _label.size()) {
      throw std::invalid_argument(
          "The size of the probability vector must be equal to the size of the "
          "label vector. The Probabilities vector has size " +
          std::to_string(probabilities_vector_size) + " while the label vector has size " +
          std::to_string(_label.size()));
    }
  }

  void forward() final {
    assert(!_label.empty());
    assert(!_loss.has_value());

    applyOperation();
  }

  /**
   * Suppose P \in \mathbb{R}^k is the output probability vector computed
   * by the softmax function, and let CE(Y, P) be the cross entropy loss
   * computed by this vertex. Treating CE as a function of P (with Y constant),
   * i.e., CE(Y, P) = CE(P) = -log(P_j) where Y_j = 1.0,
   * we observe that the gradient is a 1 x n matrix given by
   * DCE = [0, 0, ..., (-1/P_j), ..., 0]
   *
   */
  void backward(const std::optional<std::vector<std::vector<float>>> &gradient =
                    std::nullopt) final {
    if (gradient.has_value()) {
      throw std::invalid_argument("The loss function's backward method should "
                                  "not have a gradient parameter.");
    }
    assert(_gradient.empty());
    auto output_size = _input->getOutputSize();
    auto probabilities = _input->getOutput().at(0);
    _gradient = std::vector<std::vector<float>>(
        1, std::vector<float>(output_size, 0.0));

    uint32_t index_with_positive_label = findIndexWithPositiveLabel(_label);
    
    // derivative of -log(P_j) where j is the index
    auto derivative_at_index =
        -(1.f / probabilities.at(index_with_positive_label));
    _gradient[0][index_with_positive_label] = derivative_at_index;
  }

  inline std::string getName() final { return "CrossEntropyLoss"; }

  inline std::vector<std::vector<float>> getOutput() const override {
    assert(_loss.has_value());
    return {{_loss.value()}};
  }

  constexpr uint32_t getOutputSize() const final { return 1; }

private:
  /**
   * Assuming a one-hot encoded vector as an input, this function returns
   * the index in the vector where the label is 1.0
   */
  static uint32_t findIndexWithPositiveLabel(const std::vector<float> &label) {
    auto iterator = std::find(label.begin(), label.end(), 1.0);
    if (iterator != label.end()) {
      return iterator - label.begin();
    }
    throw std::runtime_error("Each label vector must be one-hot encoded.");
  }
  /**
   * Let Y and P be the true distribution of the labels and the computed
   * probabilities by the neural network respectively. Assuming that they
   * are supported on a space of n possible values, the cross entropy is
   * given by
   *        CE(Y, P) = \sum_{k=1}^{n}y_k \log(p_k)
   * The function below computes exactly the expression above.
   * For more on cross-entropy, check out
   * https://eli.thegreenplace.net/2016/the-softmax-function-and-its-derivative/
   */
  std::shared_ptr<Vertex> applyOperation() final {
    _loss = 0.f;
    auto output_size = _label.size();
    auto probabilities = _input->getOutput().at(0);
    for (uint32_t prob_index = 0; prob_index < output_size; prob_index++) {
      (*_loss) += _label[prob_index] * log(probabilities[prob_index]);
    }
    return shared_from_this();
  }

  VertexPointer _input;

  // One-hot encoded vector representing the label
  std::vector<float> _label;
  std::optional<float> _loss;

  template <typename Archive> void serialize(Archive &archive) {
    archive(cereal::base_class<Vertex>(this), _input, _label, _loss, _gradient);
  }
};

} // namespace fortis::comp_graph

CEREAL_REGISTER_TYPE(fortis::comp_graph::CrossEntropyLoss)