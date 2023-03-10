
#include <_types/_uint32_t.h>
#include <cstddef>
#include <optional>
#include <set>
#include <src/comp_graph/vertex.hpp>
#include <src/loss_functions/loss.hpp>
#include <vector>

namespace fortis::comp_graph {

using fortis::loss_functions::Loss;

class Graph {
public:
  Graph() : _vertices({}), _logits({}), _loss_value(std::nullopt) {}
  Graph(const Graph &) = delete;
  Graph(Graph &&) = delete;

  void addVertex(VertexPointer vertex) { _vertices.insert(std::move(vertex)); }

  void launchForwardPass() {
    auto topologically_sorted_vertices = computeTopologicalOrdering();
    auto graph_size = topologically_sorted_vertices.size();

    for (uint32_t vertex_index = 0; vertex_index < graph_size; vertex_index++) {
      auto vertex = topologically_sorted_vertices[vertex_index];
      vertex->forward();
      if (vertex_index == graph_size - 1) {
        _logits = std::move(vertex->getOuput());
      }
    }
  }
  void evaluateLossFunctionValue(std::unique_ptr<Loss> loss_function) {
    _loss_value = loss_function->getValue();
  }
  void launchBackwardPass();

private:
  std::vector<VertexPointer> computeTopologicalOrdering() {
    std::unordered_set<uint32_t> visited_vertices;
  }

  std::set<VertexPointer> _vertices;
  std::vector<std::vector<float>> _logits;

  std::optional<float> _loss_value;
};

} // namespace fortis::comp_graph