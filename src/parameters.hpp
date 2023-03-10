#pragma once

#include "cereal/access.hpp"
#include "utils.hpp"
#include <ios>
#include <memory>
#include <vector>

namespace fortis::parameters {

struct Parameter {
  explicit Parameter(std::vector<std::vector<float>> &&input)
      : _axes(input.size() == 1 ? 1 : 2), _value(input) {}

  constexpr uint32_t axes() const { return _axes; }
  std::vector<std::vector<float>> getValue() const { return _value; }

  std::vector<std::vector<float>> getGradient() const { return _gradient; }

  void setGradient(std::vector<std::vector<float>> &gradient) {
    _gradient = std::move(gradient);
  }

private:
  Parameter(){};
  uint32_t _axes;
  std::vector<std::vector<float>> _value;
  std::vector<std::vector<float>> _gradient;

  friend class cereal::access;

  template <typename Archive> void serialize(Archive &archive) {
    archive(_axes, _value, _gradient);
  }
};

using ParameterPointer = std::unique_ptr<Parameter>;

} // namespace fortis::parameters