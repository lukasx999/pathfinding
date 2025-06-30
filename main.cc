#include <list>
#include <print>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <limits>

#include <raylib.h>
#include <raymath.h>

static constexpr int WIDTH = 1600;
static constexpr int HEIGHT = 900;

namespace ranges = std::ranges;

using VertexId = int;

struct Edge {
    VertexId m_other_id;
    int m_weight;
};

struct Vertex {
    VertexId m_id;
    std::vector<Edge> m_neighbours;
    Vector2 m_pos;
};

class Solver {
    const std::unordered_map<VertexId, Vertex> m_vertices {
        { 1, { 1, { { 2, 5}, { 5, 2 } }, { 0, 0.5 } } },
        { 2, { 2, { { 1, 5}, { 3, 2 } }, { 1, 1 } } },
        { 3, { 3, { { 2, 2}, { 4, 2 } }, { 2, 0.5 } } },
        { 4, { 4, { { 3, 2}, { 5, 1 } }, { 0.75, 0 } } },
        { 5, { 5, { { 1, 2}, { 4, 1 } }, { 0.25, 0 } } },
    };
    std::list<VertexId> m_unvisited { 1, 2, 3, 4, 5 };
    static constexpr int m_inf = std::numeric_limits<int>::max();
    std::unordered_map<VertexId, int> m_distances {
        { 1, 0 },
        { 2, m_inf },
        { 3, m_inf },
        { 4, m_inf },
        { 5, m_inf },
    };

public:
    void draw() const {

        float fontsize = 50;
        DrawText(std::format("distances to src: {}", m_distances).c_str(), 0, 0, fontsize, WHITE);

        for (auto &[key, value] : m_vertices) {
            Vector2 base { WIDTH/2.0f, HEIGHT/2.0f };
            float factor = 150;
            DrawCircleV(value.m_pos*factor + base, 5, BLUE);

            for (auto &other : value.m_neighbours) {
                auto other_id = m_vertices.at(other.m_other_id);
                DrawLineV(value.m_pos*factor+base, other_id.m_pos*factor+base, GRAY);
            }

        }
    }

    void solve() {

        while (!m_unvisited.empty()) {

            VertexId current = *ranges::min_element(m_unvisited, [&](VertexId a, VertexId b) {
                return m_distances.at(a) < m_distances.at(b);
            });

            auto vert = m_vertices.at(current);

            int current_dist = m_distances.at(current);
            for (auto &[other, weight] : vert.m_neighbours) {

                bool is_unvisited = ranges::find(m_unvisited, other) != m_unvisited.end();
                if (!is_unvisited) continue;

                int dist = current_dist + weight;
                if (dist < m_distances.at(other)) {
                    m_distances[other] = dist;
                }
            }

            m_unvisited.remove(current);

        }

    }

};

int main() {

    Solver solver;
    solver.solve();

    // TODO: graph editor
    InitWindow(WIDTH, HEIGHT, "path finding");

    while (!WindowShouldClose()) {
        BeginDrawing();
        {
            ClearBackground(BLACK);
            solver.draw();

        }
        EndDrawing();
    }

    CloseWindow();

    return EXIT_SUCCESS;
}
