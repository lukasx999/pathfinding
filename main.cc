#include <list>
#include <print>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <limits>

#include <raylib.h>
#include <raymath.h>

#define PRINT(x) std::println("{}: {}", #x, x)

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
    std::unordered_map<VertexId, Vertex> m_vertices {
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

    VertexId m_current;
    std::optional<std::vector<Edge>::iterator> m_neighbour;

public:
    void draw() const {

        float fontsize = 50;
        DrawText(std::format("current: {}", m_current).c_str(), 0, 0, fontsize, WHITE);
        DrawText(std::format("distances to src: {}", m_distances).c_str(), 0, fontsize, fontsize, WHITE);
        DrawText(std::format("unvisited: {}", m_unvisited).c_str(), 0, fontsize*2, fontsize, WHITE);

        for (auto &[key, value] : m_vertices) {
            Vector2 base { WIDTH/2.0f, HEIGHT/2.0f };
            float factor = 150;

            auto color = value.m_id == m_current ? RED : BLUE;

            if (m_neighbour.has_value()) {
                auto neighbour = m_neighbour.value()->m_other_id;
                if (value.m_id == neighbour)
                    color = GREEN;
            }

            DrawCircleV(value.m_pos*factor + base, 5, color);

            for (auto &other : value.m_neighbours) {
                auto other_id = m_vertices.at(other.m_other_id);
                DrawLineV(value.m_pos*factor+base, other_id.m_pos*factor+base, GRAY);
            }

        }
    }

    void next() {

        if (m_unvisited.empty()) return;

        if (m_neighbour.has_value()) {

            auto &vtx = m_vertices.at(m_current);

            if (m_neighbour == vtx.m_neighbours.end()) {
                m_neighbour.reset();
                m_unvisited.remove(m_current);
                next();

            } else {
                visit_neighbour();
                m_neighbour.value()++;

            }

        } else {
            m_current = *ranges::min_element(m_unvisited, [&](VertexId a, VertexId b) {
                return m_distances.at(a) < m_distances.at(b);
            });

            auto &vtx = m_vertices.at(m_current);
            m_neighbour = vtx.m_neighbours.begin();
        }

    }

    void visit_neighbour() {
        auto current = m_vertices.at(m_current);
        int current_dist = m_distances.at(current.m_id);

        auto v = *m_neighbour.value();
        auto other = v.m_other_id;

        bool is_unvisited = ranges::find(m_unvisited, other) != m_unvisited.end();
        if (!is_unvisited) return;

        int dist = current_dist + v.m_weight;
        if (dist < m_distances.at(other)) {
            m_distances[other] = dist;
        }

    }

};

int main() {

    Solver solver;

    // TODO: graph editor
    InitWindow(WIDTH, HEIGHT, "path finding");

    while (!WindowShouldClose()) {
        BeginDrawing();
        {
            ClearBackground(BLACK);
            solver.draw();

            if (IsKeyPressed(KEY_J))
                solver.next();

        }
        EndDrawing();
    }

    CloseWindow();

    return EXIT_SUCCESS;
}
