
#include <iostream>
#include <vector>
using namespace std;

bool dfs(int node, vector<vector<int>>& graph,
         vector<int>& visited, vector<int>& path) {

    visited[node] = 1;
    path[node] = 1;

    for (int neighbor : graph[node]) {

        if (!visited[neighbor]) {
            if (dfs(neighbor, graph, visited, path)) {
                return true;
            }
        }
        else if (path[neighbor]) {
            return true;
        }
    }

    path[node] = 0;
    return false;
}

bool containsCycle(vector<vector<int>>& graph) {

    int n = graph.size();

    vector<int> visited(n, 0);
    vector<int> path(n, 0);

    // Check every node, including disconnected components.
    for (int i = 0; i < n; i++) {

        if (!visited[i]) {
            if (dfs(i, graph, visited, path)) {
                return true;
            }
        }
    }

    return false;
}

int main() {

    // Test 1: Graph with a cycle
    vector<vector<int>> graph1(3);

    graph1[0].push_back(1); // T1 -> T2
    graph1[1].push_back(2); // T2 -> T3
    graph1[2].push_back(0); // T3 -> T1

    cout << "Graph 1: ";

    if (containsCycle(graph1))
        cout << "Cycle detected\n";
    else
        cout << "No cycle\n";


    // Test 2: Graph without a cycle
    vector<vector<int>> graph2(4);

    graph2[0].push_back(1); // T1 -> T2
    graph2[1].push_back(2); // T2 -> T3
    graph2[2].push_back(3); // T3 -> T4

    cout << "Graph 2: ";

    if (containsCycle(graph2))
        cout << "Cycle detected\n";
    else
        cout << "No cycle\n";

    return 0;
}
