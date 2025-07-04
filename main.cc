#include <cassert>
#include <random>
#include <list>
#include <span>
#include <unordered_map>
#include <ranges>
#include <print>
#include <vector>
#include <algorithm>

#include <raylib.h>
#include <raymath.h>

#include "./tinyxml2.h"

#define PRINT(x) std::println("{}: {}", #x, x)

static constexpr int WIDTH = 1600;
static constexpr int HEIGHT = 900;

namespace ranges = std::ranges;

using VertexId = int64_t;

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
    struct TableEntry {
        int m_dist; // distance from source vertex
        VertexId m_prev; // previous vertex
    };

    // data
    std::unordered_map<VertexId, Vertex> m_vertices;
    VertexId m_source;
    std::list<VertexId> m_unvisited;
    static constexpr int m_inf = 999;
    std::unordered_map<VertexId, TableEntry> m_table;

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
        , m_source(source)
    {
        reset();
    }

    [[nodiscard]] auto get_optimal_path(VertexId dest) const {
        std::vector<VertexId> path;

        VertexId prev = dest;
        while (prev != m_source) {
            prev = m_table.at(prev).m_prev;
            path.push_back(prev);
        }

        ranges::reverse(path);

        return path;
    }

    [[nodiscard]] bool is_done() const {
        return m_state == State::Terminated;
    }

    void reset() {
        m_state = State::Idle;
        ranges::for_each(m_vertices, [&](const std::pair<VertexId, Vertex> &kv) {
            auto &[key, value] = kv;
            m_unvisited.push_back(key);
            m_table[key] = { key == m_source ? 0 : m_inf, -1 };
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
                    assert(m_unvisited.empty());
                    return;
                };

                next_unvisited();
                m_state = State::NextVertex;

            } break;

            case State::NextVertex: {
                auto &vtx = m_vertices.at(m_current);
                m_neighbour = vtx.m_neighbours.cbegin();

                bool no_neighbours = m_neighbour == vtx.m_neighbours.end();

                if (no_neighbours) {
                    m_unvisited.remove(m_current);
                    m_state = State::Idle;
                    return;
                }

                m_state = State::Visiting;

            } break;

            case State::Visiting: {
                auto &vtx = m_vertices.at(m_current);

                visit_neighbour();
                m_neighbour++;

                if (m_neighbour == vtx.m_neighbours.end()) {
                    m_unvisited.remove(m_current);
                    m_state = State::Idle;
                    return;
                }

            } break;

        }
    }

private:
    inline void next_unvisited() {
        m_current = *ranges::min_element(m_unvisited, [&](VertexId a, VertexId b) {
            int dist_a = m_table.at(a).m_dist;
            int dist_b = m_table.at(b).m_dist;
            return dist_a < dist_b;
        });
    }

    void visit_neighbour() {
        auto current = m_vertices.at(m_current);
        int current_dist = m_table.at(current.m_id).m_dist;

        auto v = *m_neighbour;
        auto other = v.m_other_id;

        bool is_unvisited = ranges::find(m_unvisited, other) != m_unvisited.end();
        if (!is_unvisited) return;

        int dist = current_dist + v.m_weight;
        int other_dist = m_table.at(other).m_dist;
        if (dist < other_dist) {
            m_table[other] = { dist, current.m_id };
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
    static constexpr Vector2 m_draw_offset { 0, 0 };

public:
    Renderer(const Solver &solver) : m_solver(solver) { }

    void draw() const {

        for (auto &[id, vtx] : m_solver.m_vertices) {
            auto pos = convert_vertex_pos(vtx.m_pos);
            draw_neighbours(pos, vtx.m_neighbours);
        }

        if (m_solver.m_state == Solver::State::Visiting) {
            auto other_pos = m_solver.m_vertices.at(m_solver.m_neighbour->m_other_id).m_pos;
            auto pos = m_solver.m_vertices.at(m_solver.m_current).m_pos;
            DrawLineEx(convert_vertex_pos(pos), convert_vertex_pos(other_pos), 5, GREEN);
        }

        for (auto &[id, vtx] : m_solver.m_vertices) {
            draw_vertex(id, vtx);
        }

        float radius = 10;
        DrawCircleV(convert_vertex_pos(m_solver.m_vertices.at(m_solver.m_source).m_pos), radius, RED);

        draw_ui();

    }

private:
    [[nodiscard]] static Vector2 convert_vertex_pos(Vector2 pos) {
        return pos * Vector2 { WIDTH, HEIGHT };
    }

    void draw_vertex(VertexId id, Vertex vtx) const {

        auto pos = convert_vertex_pos(vtx.m_pos);
        auto color = vtx.m_id == m_solver.m_current ? RED : BLUE;

        if (m_solver.m_state == Solver::State::Visiting) {
            auto neighbour = m_solver.m_neighbour->m_other_id;
            if (vtx.m_id == neighbour)
                color = GREEN;
        }

        if (m_solver.m_state == Solver::State::Terminated) {
            // TODO: make dest configurable
            auto path = m_solver.get_optimal_path(3);
            bool exists = ranges::find(path, id) != path.end();
            if (exists) {
                color = PURPLE;
            }
        }

        float radius = 30;
        DrawCircleV(convert_vertex_pos(vtx.m_pos), radius, color);

        float fontsize = 50;
        draw_text_centered(std::format("{}", id), pos, fontsize, WHITE);
    }

    void draw_ui() const {

        DrawText(
            std::format("unvisited: {}", m_solver.m_unvisited).c_str(),
            0,
            0,
            m_fontsize,
            WHITE
        );

        DrawText(
            std::format("state: {}", m_solver.stringify_state(m_solver.m_state)).c_str(),
            0,
            m_fontsize,
            m_fontsize,
            WHITE
        );

        draw_distance_table({ 0, m_fontsize*3 });
    }

    void draw_distance_table(Vector2 pos) const {
        for (auto &&[idx, kv] : std::views::enumerate(m_solver.m_table)) {
            auto &[key, entry] = kv;

            DrawText(
                std::format("{}: {} {}", key, entry.m_dist, entry.m_prev).c_str(),
                pos.x,
                m_fontsize * idx + pos.y,
                m_fontsize,
                WHITE
            );

        }
    }

    void draw_neighbours(Vector2 vertex_pos, std::span<const Edge> edges) const {

        for (const auto &edge : edges) {

            VertexId id = edge.m_other_id;
            auto vtx = m_solver.m_vertices.at(id);
            auto pos = convert_vertex_pos(vtx.m_pos);

            DrawLineEx(vertex_pos, pos, 3, GRAY);

            // TODO:
            // auto diff = pos - vertex_pos;
            // float dist = Vector2Length(diff) / 2.0f;
            // auto line_middle = vertex_pos + Vector2Normalize(diff) * dist;
            // float fontsize = 50;
            // draw_text_centered(std::format("{}", edge.m_weight), line_middle, fontsize, WHITE);

        }

    }

};

[[nodiscard]] static double random_number() {
    std::mt19937 rng(std::random_device{}());
    return static_cast<double>(rng()) / rng.max();
}

[[nodiscard]] static std::unordered_map<VertexId, Vertex> generate_random_vertices(int n) {
    int max_weight = 10;
    std::unordered_map<VertexId, Vertex> verts;

    std::vector<VertexId> nodes;
    nodes.resize(n);
    ranges::iota(nodes, 1);

    for (auto &node : nodes) {
        Vector2 pos { static_cast<float>(random_number()), static_cast<float>(random_number()) };
        verts[node] = { node, { }, pos };

    }

    for (auto &node : nodes) {

        std::vector<Edge> neighbours;
        for (auto &other : nodes) {
            if (other != node)
                neighbours.push_back({ other, static_cast<int>(random_number() * static_cast<double>(max_weight)) });
        }

        verts[node].m_neighbours = neighbours;

    }

    return verts;
}

[[nodiscard]] static auto xml_get_child_elements(tinyxml2::XMLElement *elem, const char *name) {
    assert(elem != nullptr);

    std::vector<tinyxml2::XMLElement*> elems;

    tinyxml2::XMLElement *node = elem->FirstChildElement(name);
    while (node != nullptr) {
        elems.push_back(node);
        node = node->NextSiblingElement(name);
    }

    return elems;
}

[[nodiscard]] static constexpr Vector2 vec2_from_lat_lon(float lat, float lon, float width, float height) {
    float x = ((width/360.0) * (180 + lon));
    float y = ((height/180.0) * (90 - lat));
    return { x, y };
}

[[nodiscard]] static auto vertices_from_xml(const char *filename) {
    std::unordered_map<VertexId, Vertex> vertices;

    tinyxml2::XMLDocument doc;
    doc.LoadFile(filename);

    auto osm = doc.FirstChildElement("osm");
    assert(osm != nullptr);

    auto nodes = xml_get_child_elements(osm, "node");

    for (auto &node : nodes) {
        auto id = node->Attribute("id");
        auto lat = node->Attribute("lat");
        auto lon = node->Attribute("lon");

        VertexId vtx_id = 0;
        std::from_chars(id, id + strlen(id), vtx_id);

        float latf = 0;
        std::from_chars(lat, lat + strlen(lat), latf);

        float lonf = 0;
        std::from_chars(lon, lon + strlen(lon), lonf);

        Vector2 pos = vec2_from_lat_lon(latf, lonf, 1.0f, 1.0f);
        std::println("id: {}, x: {}, y: {}", id, pos.x, pos.y);

        pos.x -= 0.52;
        pos.y -= 0.22;
        pos *= 30;

        vertices[vtx_id] = { vtx_id, { }, pos };
    }

    auto ways = xml_get_child_elements(osm, "way");

    for (auto &way : ways) {
        auto nds = xml_get_child_elements(way, "nd");

        auto first_str = nds[0]->Attribute("ref");
        VertexId first = 0;
        std::from_chars(first_str, first_str + strlen(first_str), first);

        auto last_str = nds[nds.size()-1]->Attribute("ref");
        VertexId last = 0;
        std::from_chars(last_str, last_str + strlen(last_str), last);

        // vertices[first].m_neighbours.push_back({ last, 1 });
        // vertices[last].m_neighbours.push_back({ first, 1 });

        for (auto &nd : nds) {
            auto id_str = nd->Attribute("ref");
            VertexId id = 0;
            std::from_chars(id_str, id_str + strlen(id_str), id);

            for (auto &other : nds) {
                auto other_str = other->Attribute("ref");
                VertexId other_id = 0;
                std::from_chars(other_str, other_str + strlen(other_str), other_id);

                if (other_id == id) continue;

                vertices[id].m_neighbours.push_back(Edge { other_id, 1 });
            }

        }

    }

    return vertices;
}

int main() {

    // auto vertices = vertices_from_xml("./map.osm");

    // auto vertices = generate_random_vertices(10);

    std::unordered_map<VertexId, Vertex> vertices {
        { 1, { 1, { { 2, 5 }, { 5, 2 } }, { 0.1, 0.5 } } },
        { 2, { 2, { { 1, 5 }, { 3, 2 }, { 4, 1 } }, { 0.9, 0.9 } } },
        { 3, { 3, { { 2, 2 }, { 4, 2 } }, { 0.9, 0.5 } } },
        { 4, { 4, { { 3, 2 }, { 5, 1 }, { 2, 1 } }, { 0.75, 0.1 } } },
        { 5, { 5, { { 1, 2 }, { 4, 1 } }, { 0.25, 0.1 } } },
        { 6, { 6, { }, { 0.35, 0.3 } } },
    };

    std::println("vertices: {}", vertices.size());

    // Solver solver(vertices, 12966960339);
    Solver solver(vertices, 1);
    Renderer renderer(solver);

    SetTraceLogLevel(LOG_ERROR);
    InitWindow(WIDTH, HEIGHT, "Path Finding");

    float fut = 0;
    float interval = 0.1;

    while (!WindowShouldClose()) {
        BeginDrawing();
        {
            ClearBackground(BLACK);
            renderer.draw();

            if (GetTime() >= fut) {

                // if (solver.is_done()) {
                //     solver.reset();
                // }


                fut = GetTime() + interval;
            }

            if (IsKeyPressed(KEY_J))
                solver.next();


        }
        EndDrawing();
    }

    CloseWindow();

    return EXIT_SUCCESS;
}
