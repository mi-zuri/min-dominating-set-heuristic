#include <iostream>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <algorithm>
#include <chrono>
#include <stdexcept>

class OptimizedSolver {
private:
    int n;
    std::vector<std::pair<int, int>> edges;
    int distance;
    std::vector<std::set<int>> adj_list;
    std::map<int, std::set<int>> d_neighborhoods;

    void validate_edge(int u, int v) {
        if (u < 0 || u >= n || v < 0 || v >= n) {
            throw std::runtime_error("Edge vertices out of bounds");
        }
    }

    std::map<int, std::set<int>> _compute_d_neighborhoods() {
        auto init_time = std::chrono::steady_clock::now();
        std::map<int, std::set<int>> d_neighborhoods;

        for (int v = 0; v < n; ++v) {
            std::queue<int> queue;
            queue.push(v);
            std::vector<bool> visited(n, false);
            visited[v] = true;
            d_neighborhoods[v].insert(v);
            int current_distance = 0;

            while (!queue.empty() && current_distance < distance) {
                int level_size = queue.size();
                for (int i = 0; i < level_size; ++i) {
                    int curr_v = queue.front();
                    queue.pop();

                    if (curr_v >= 0 && curr_v < n) {
                        for (const int& neighbor : adj_list[curr_v]) {
                            if (!visited[neighbor]) {
                                visited[neighbor] = true;
                                d_neighborhoods[v].insert(neighbor);
                                queue.push(neighbor);
                            }
                        }
                    }
                }
                current_distance++;
            }
        }
        std::cout << "Neighborhoods computed in: " << std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - init_time
        ).count() << "ms" << std::endl;
        return d_neighborhoods;
    }

public:
    OptimizedSolver(int n, const std::vector<std::pair<int, int>>& edges, int distance)
        : n(n), edges(edges), distance(distance) {
        if (n <= 0) {
            throw std::runtime_error("Number of vertices must be positive");
        }
        if (distance < 0) {
            throw std::runtime_error("Distance must be non-negative");
        }

        // Initialize adjacency list with correct size
        adj_list.resize(n);

        // Build adjacency list with bounds checking
        for (const auto& edge : edges) {
            try {
                validate_edge(edge.first, edge.second);
                adj_list[edge.first].insert(edge.second);
                adj_list[edge.second].insert(edge.first);
            } catch (const std::exception& e) {
                std::cerr << "Invalid edge: (" << edge.first << ", " << edge.second << "): " << e.what() << std::endl;
                throw;
            }
        }

        try {
            // Precompute d-neighborhoods
            d_neighborhoods = _compute_d_neighborhoods();
        } catch (const std::exception& e) {
            std::cerr << "Error computing neighborhoods: " << e.what() << std::endl;
            throw;
        }
    }

    std::vector<int> fast_dominating_set_heuristic(bool randomized = false) {
        auto init_time = std::chrono::steady_clock::now();
        std::vector<int> result;
        std::set<int> dominated;
        int undominated_count = n;

        // Initial scores for vertices
        std::vector<int> dscore(n, 0);
        for (int v = 0; v < n; ++v) {
            auto it = d_neighborhoods.find(v);
            if (it != d_neighborhoods.end()) {
                dscore[v] = it->second.size();
            }
        }

        std::set<int> active_vertices;
        for (int i = 0; i < n; ++i) {
            active_vertices.insert(i);
        }

        while (undominated_count > 0 && !active_vertices.empty()) {
            // Find vertex with highest score
            auto best_it = std::max_element(
                active_vertices.begin(),
                active_vertices.end(),
                [&](int a, int b) {
                    return (dominated.count(a) ? -1 : dscore[a]) <
                           (dominated.count(b) ? -1 : dscore[b]);
                }
            );

            if (best_it == active_vertices.end()) break;
            int best_vertex = *best_it;

            result.push_back(best_vertex);

            // Mark its neighborhood as dominated
            auto neighborhood_it = d_neighborhoods.find(best_vertex);
            if (neighborhood_it != d_neighborhoods.end()) {
                for (int v : neighborhood_it->second) {
                    if (dominated.count(v) == 0) {
                        dominated.insert(v);
                        undominated_count--;

                        auto v_neighborhood = d_neighborhoods.find(v);
                        if (v_neighborhood != d_neighborhoods.end()) {
                            for (int u : v_neighborhood->second) {
                                if (active_vertices.count(u)) {
                                    dscore[u]--;
                                }
                            }
                        }
                    }
                }
            }

            // Remove dominated vertices from active consideration
            for (auto it = active_vertices.begin(); it != active_vertices.end();) {
                if (dominated.count(*it)) {
                    it = active_vertices.erase(it);
                } else {
                    ++it;
                }
            }
        }

        std::cout << "Heuristic computed in: " << std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - init_time
        ).count() << "ms" << std::endl;
        // std::cout << "Solution valid: " << is_valid_solution(result) << std::endl;
        return result;
    }

    std::vector<int> run_heuristics_with_randomization(
        std::chrono::steady_clock::time_point init_time,
        int time_limit = 20
    ) {
        auto heuristic_start_time = std::chrono::steady_clock::now();
        auto solution = fast_dominating_set_heuristic();
        auto heuristic_run_time = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - heuristic_start_time
        ).count();

        return solution;
    }

    std::vector<int> solve(int time_limit = 20) {
        auto init_time = std::chrono::steady_clock::now();
        return run_heuristics_with_randomization(init_time, time_limit);
    }

    bool is_valid_solution(const std::vector<int>& vertices) {
        std::set<int> dominated;
        for (int v : vertices) {
            auto it = d_neighborhoods.find(v);
            if (it != d_neighborhoods.end()) {
                dominated.insert(it->second.begin(), it->second.end());
            }
        }
        return dominated.size() == n;
    }
};

int main() {
    try {
        // Read input
        int n, m;
        if (!(std::cin >> n >> m)) {
            throw std::runtime_error("Failed to read n and m");
        }

        if (n <= 0 || m < 0) {
            throw std::runtime_error("Invalid n or m values");
        }

        std::vector<std::pair<int, int>> edges;
        edges.reserve(m);

        for (int i = 0; i < m; ++i) {
            int u, v;
            if (!(std::cin >> u >> v)) {
                throw std::runtime_error("Failed to read edge");
            }
            // Use vertices as-is, assuming 0-based indexing in input
            edges.emplace_back(u, v);
        }

        int distance;
        if (!(std::cin >> distance)) {
            throw std::runtime_error("Failed to read distance");
        }

        if (distance < 0) {
            throw std::runtime_error("Invalid distance value");
        }

        // Solve the problem
        OptimizedSolver solver(n, edges, distance);
        std::vector<int> result = solver.solve(3);

        // Print results
        if (result.empty()) {
            std::cout << 0 << std::endl;
        } else {
            std::cout << result.size() << std::endl;
            for (size_t i = 0; i < result.size(); ++i) {
                if (i > 0) std::cout << " ";
                // Don't add 1 to output, keep 0-based indexing
                std::cout << result[i];
            }
            std::cout << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}