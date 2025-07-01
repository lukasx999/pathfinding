#include <list>
#include <span>
#include <unordered_map>
#include <ranges>
#include <print>
#include <vector>
#include <algorithm>

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

static inline void draw_text_centered(const std::string &text, Vector2 center, float fontsize, Color color) {
    int textsize = MeasureText(text.c_str(), fontsize);
    DrawText(text.c_str(), center.x-textsize/2.0f, center.y-fontsize/2.0f, fontsize, color);
}

class Solver {
    // data
    std::unordered_map<VertexId, Vertex> m_vertices;
    std::list<VertexId> m_unvisited;
    static constexpr int m_inf = 999;
    std::unordered_map<VertexId, int> m_distances;

    // state
    VertexId m_current;
    std::vector<Edge>::const_iterator m_neighbour;
    enum class State {
        Idle,
        NextVertex,
        Visiting,
        Terminated,
    } m_state = State::Idle;

    friend class Renderer;

public:
    Solver(std::unordered_map<VertexId, Vertex> vertices, VertexId source)
    : m_vertices(vertices)
    {

        ranges::for_each(m_vertices, [&](const std::pair<VertexId, Vertex> &kv) {
            auto &[key, value] = kv;
            m_unvisited.push_back(key);
            m_distances[key] = key == source ? 0 : m_inf;
        });

    }

    void next() {
        switch (m_state) {
            case State::Terminated:
                ;
                break;

            case State::Idle: {

                if (m_unvisited.empty()) {
                    m_state = State::Terminated;
                    return;
                };

                next_unvisited();
                m_state = State::NextVertex;

            } break;

            case State::NextVertex: {
                auto &vtx = m_vertices.at(m_current);
                m_neighbour = vtx.m_neighbours.cbegin();
                m_state = State::Visiting;

            } break;

            case State::Visiting: {
                auto &vtx = m_vertices.at(m_current);

                if (m_neighbour == vtx.m_neighbours.end()) {
                    m_unvisited.remove(m_current);
                    m_state = State::Idle;
                    next();
                    return;
                }

                visit_neighbour();
                m_neighbour++;

            } break;

        }
    }

private:
    inline void next_unvisited() {
        m_current = *ranges::min_element(m_unvisited, [&](VertexId a, VertexId b) {
            return m_distances.at(a) < m_distances.at(b);
        });
    }

    void visit_neighbour() {
        auto current = m_vertices.at(m_current);
        int current_dist = m_distances.at(current.m_id);

        auto v = *m_neighbour;
        auto other = v.m_other_id;

        bool is_unvisited = ranges::find(m_unvisited, other) != m_unvisited.end();
        if (!is_unvisited) return;

        int dist = current_dist + v.m_weight;
        if (dist < m_distances.at(other)) {
            m_distances[other] = dist;
        }

    }

    [[nodiscard]] static constexpr const char *stringify_state(State state) {
        switch (state) {
            case State::Idle:       return "Idle";
            case State::NextVertex: return "NextVertex";
            case State::Visiting:   return "Visiting";
            case State::Terminated: return "Terminated";
        }
        std::unreachable();
    }

};

class Renderer {
    const Solver &m_solver;
    static constexpr float m_fontsize = 50;
    static constexpr Vector2 m_draw_offset { WIDTH/2.0f, HEIGHT/2.0f };
    static constexpr float m_vertex_spacing = 300;

public:
    Renderer(const Solver &solver) : m_solver(solver) { }

    void draw() const {

        for (auto &[key, value] : m_solver.m_vertices) {
            auto pos = value.m_pos * m_vertex_spacing + m_draw_offset;
            draw_neighbours(pos, value.m_neighbours, key);
        }

        for (auto &[key, value] : m_solver.m_vertices) {
            // TODO: remove code duplication
            auto pos = value.m_pos * m_vertex_spacing + m_draw_offset;
            auto color = value.m_id == m_solver.m_current ? RED : BLUE;

            if (m_solver.m_state == Solver::State::Visiting) {
                auto neighbour = m_solver.m_neighbour->m_other_id;
                if (value.m_id == neighbour)
                    color = GREEN;
            }

            float radius = 30;
            DrawCircleV(value.m_pos * m_vertex_spacing + m_draw_offset, radius, color);

            float fontsize = 50;
            draw_text_centered(std::format("{}", key), pos, fontsize, WHITE);

        }

        draw_ui();

    }

private:
    inline void draw_ui() const {
        DrawText(std::format("unvisited: {}", m_solver.m_unvisited).c_str(), 0, 0, m_fontsize, WHITE);
        DrawText(std::format("state: {}", m_solver.stringify_state(m_solver.m_state)).c_str(), 0, m_fontsize, m_fontsize, WHITE);
        draw_distance_table({ 0, m_fontsize*3 });
    }

    void draw_distance_table(Vector2 pos) const {
        for (auto &&[idx, kv] : std::views::enumerate(m_solver.m_distances)) {
            auto &[key, dist] = kv;

            DrawText(
                std::format("{}: {}", key, dist).c_str(),
                pos.x,
                m_fontsize * idx + pos.y,
                m_fontsize,
                WHITE
            );

        }
    }

    void draw_neighbours(Vector2 vertex_pos, std::span<const Edge> edges, VertexId origin) const {

        for (const auto &edge : edges) {

            VertexId id = edge.m_other_id;
            auto vtx = m_solver.m_vertices.at(id);
            auto pos = vtx.m_pos * m_vertex_spacing + m_draw_offset;

            auto color = GRAY;

            if (m_solver.m_state == Solver::State::Visiting) {

                bool is_origin = origin == m_solver.m_current;
                bool is_currently_visited = m_solver.m_neighbour->m_other_id == id;

                if (is_origin && is_currently_visited) {
                    color = GREEN;
                }

            }

            DrawLineEx(vertex_pos, pos, 5, color);

            auto diff = pos - vertex_pos;
            float dist = Vector2Length(diff) / 2.0f;
            auto line_middle = vertex_pos + Vector2Normalize(diff) * dist;

            float fontsize = 50;
            draw_text_centered(std::format("{}", edge.m_weight), line_middle, fontsize, WHITE);

        }

    }

};

int main() {

    std::unordered_map<VertexId, Vertex> m_vertices {
        { 1, { 1, { { 2, 5 }, { 5, 2 }, { 3, 1 } }, { 0, 0.5 } } },
        { 2, { 2, { { 1, 5 }, { 3, 2 }, { 4, 1 } }, { 1, 1 } } },
        { 3, { 3, { { 2, 2 }, { 4, 2 }, { 1, 1 } }, { 2, 0.5 } } },
        { 4, { 4, { { 3, 2 }, { 5, 1 }, { 2, 1 } }, { 0.75, 0 } } },
        { 5, { 5, { { 1, 2 }, { 4, 1 } }, { 0.25, 0 } } },
    };

    Solver solver(m_vertices, 1);
    Renderer renderer(solver);

    InitWindow(WIDTH, HEIGHT, "Path Finding");

    while (!WindowShouldClose()) {
        BeginDrawing();
        {
            ClearBackground(BLACK);
            renderer.draw();

            if (IsKeyPressed(KEY_J))
                solver.next();

        }
        EndDrawing();
    }

    CloseWindow();

    return EXIT_SUCCESS;
}
