#include "ProfilePrototype.hpp"

#include <pxr/base/tf/hashmap.h>
#include <pxr/base/tf/hashset.h>
#include <pxr/base/tf/Token.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>

PXR_NAMESPACE_USING_DIRECTIVE

using TfToken = pxr::TfToken;
using TokenSet = TfHashSet<TfToken, TfToken::HashFunctor>;
using TokenMap = TfHashMap<TfToken, std::vector<TfToken>, TfToken::HashFunctor>;

struct ParseResult {
    bool hasCycle = false;
    std::vector<TfToken> cyclePath;
    std::vector<std::string> errors;
};

class DagParser {
public:
    DagParser() {}

    void PrintTree() const {
        // Find root nodes (nodes with no ancestors)
        std::vector<TfToken> roots;
        for (const auto& [node, ancestors] : adjacencyList) {
            if (ancestors.empty()) {
                roots.push_back(node);
            }
        }

        // Print tree starting from each root
        for (const auto& root : roots) {
            PrintNode(root, 0);
        }
    }

    void PrintDAG() const {
        // Build a reverse adjacency list (children -> parents)
        std::unordered_map<TfToken, std::vector<TfToken>, TfToken::HashFunctor> reverseAdjList;
        std::unordered_map<TfToken, size_t, TfToken::HashFunctor> inDegree;
        
        for (const auto& [node, ancestors] : adjacencyList) {
            auto nodeIt = reverseAdjList.find(node);
            if (nodeIt == reverseAdjList.end()) {
                nodeIt = reverseAdjList.insert({node, std::vector<TfToken>()}).first;
            }
            for (const auto& ancestor : ancestors) {
                auto ancestorIt = reverseAdjList.find(ancestor);
                if (ancestorIt == reverseAdjList.end()) {
                    ancestorIt = reverseAdjList.insert({ancestor, std::vector<TfToken>()}).first;
                }
                ancestorIt->second.push_back(node);
                inDegree[node]++;
            }
        }

        // Perform topological sort (modified to put leaves first)
        std::vector<TfToken> sortedNodes;
        std::queue<TfToken> queue;

        // Find initial leaves (nodes with no children)
        for (const auto& [node, children] : reverseAdjList) {
            if (children.empty()) {
                queue.push(node);
            }
        }

        while (!queue.empty()) {
            TfToken current = queue.front();
            queue.pop();
            sortedNodes.push_back(current);

            // Process ancestors
            auto currentIt = adjacencyList.find(current);
            if (currentIt != adjacencyList.end()) {
                for (const auto& ancestor : currentIt->second) {
                    auto ancestorIt = reverseAdjList.find(ancestor);
                    if (ancestorIt != reverseAdjList.end()) {
                        bool allChildrenProcessed = true;
                        for (const auto& child : ancestorIt->second) {
                            if (std::find(sortedNodes.begin(), sortedNodes.end(), child) == sortedNodes.end()) {
                                allChildrenProcessed = false;
                                break;
                            }
                        }
                        if (allChildrenProcessed && std::find(sortedNodes.begin(), sortedNodes.end(), ancestor) == sortedNodes.end()) {
                            queue.push(ancestor);
                        }
                    }
                }
            }
        }

        // Create the visualization
        const int maxWidth = 80;  // Width for the ASCII art area
        const int nodeSpacing = 3;  // Vertical spacing between nodes
        std::vector<std::string> grid;
        std::unordered_map<TfToken, size_t, TfToken::HashFunctor> nodeToRow;

        // Initialize grid with empty space
        for (size_t i = 0; i < sortedNodes.size() * nodeSpacing; ++i) {
            grid.push_back(std::string(maxWidth, ' '));
        }

        // First pass: Place nodes and record their positions
        for (size_t i = 0; i < sortedNodes.size(); ++i) {
            const auto& node = sortedNodes[i];
            size_t row = i * nodeSpacing;
            nodeToRow.insert({node, row});

            // Place node marker
            grid[row][0] = '*';

            // Add node name
            std::string label = " " + node.GetString();
            if (finalNodes.find(node) != finalNodes.end()) {
                label += " (final)";
            }
            grid[row] += label;
        }

        // Second pass: Draw connections
        for (size_t i = 0; i < sortedNodes.size(); ++i) {
            const auto& node = sortedNodes[i];
            size_t nodeRow = nodeToRow[node];
            auto nodeIt = adjacencyList.find(node);
            if (nodeIt != adjacencyList.end() && !nodeIt->second.empty()) {
                const auto& ancestors = nodeIt->second;
                // Sort ancestors by their row number to minimize crossing lines
                std::vector<std::pair<size_t, TfToken>> sortedAncestors;
                for (const auto& ancestor : ancestors) {
                    auto ancestorRowIt = nodeToRow.find(ancestor);
                    if (ancestorRowIt != nodeToRow.end()) {
                        sortedAncestors.push_back({ancestorRowIt->second, ancestor});
                    }
                }
                std::sort(sortedAncestors.begin(), sortedAncestors.end());

                // Draw connections for each ancestor
                for (size_t j = 0; j < sortedAncestors.size(); ++j) {
                    const auto& [ancestorRow, ancestor] = sortedAncestors[j];
                    int col = 2 + j * 3;  // Space out connections horizontally

                    // Draw vertical lines and connections
                    if (nodeRow < ancestorRow) {
                        // Connection going up
                        grid[nodeRow][col] = '/';
                        if (ancestorRow > nodeRow + nodeSpacing) {
                            grid[ancestorRow][col] = '/';
                            // Draw vertical line
                            for (size_t row = nodeRow + 1; row < ancestorRow; ++row) {
                                if (grid[row][col] == ' ') {
                                    grid[row][col] = '|';
                                }
                            }
                        }
                    } else {
                        // Connection going down
                        grid[nodeRow][col] = '\\';
                        if (ancestorRow < nodeRow - nodeSpacing) {
                            grid[ancestorRow][col] = '\\';
                            // Draw vertical line
                            for (size_t row = ancestorRow + 1; row < nodeRow; ++row) {
                                if (grid[row][col] == ' ') {
                                    grid[row][col] = '|';
                                }
                            }
                        }
                    }

                    // Add horizontal connectors
                    if (j > 0) {
                        for (int k = col - 2; k < col; ++k) {
                            if (grid[nodeRow][k] == ' ') {
                                grid[nodeRow][k] = '-';
                            }
                        }
                    }
                }
            }
        }

        // Print the grid in reverse order (bottom to top)
        for (auto it = grid.rbegin(); it != grid.rend(); ++it) {
            // Skip empty lines (lines with only spaces)
            if (it->find_first_not_of(' ') != std::string::npos) {
                std::cout << *it << '\n';
            }
        }
    }

    ParseResult Parse(const std::string& input) {
        ParseResult result;
        std::istringstream stream(input);
        std::string line;
        while (std::getline(stream, line)) {
            if (line.empty()) continue;
            ParseLine(line, result);
        }
        SecondPass(result);
        return result;
    }

private:
    void PrintNode(const TfToken& node, int depth) const {
        // Print current node with proper indentation
        std::string indent(depth * 2, ' ');
        std::cout << indent << node.GetString();
        
        // Add "final" marker if applicable
        if (finalNodes.find(node) != finalNodes.end()) {
            std::cout << " (final)";
        }
        std::cout << "\n";

        // Find all nodes that have this node as an ancestor
        for (const auto& [child, ancestors] : adjacencyList) {
            for (const auto& ancestor : ancestors) {
                if (ancestor == node) {
                    PrintNode(child, depth + 1);
                    break;
                }
            }
        }
    }

    TokenMap adjacencyList;
    TokenSet declaredNodes;
    TokenSet unresolvedNodes;
    TokenSet finalNodes;

    void ParseLine(const std::string& line, ParseResult& result) {
        std::istringstream lineStream(line);
        std::string token;

        // Extract the first token (node name)
        if (!(lineStream >> token)) return;
        TfToken nodeName(token);

        declaredNodes.insert(nodeName);
        // Ensure node exists in adjacency list with empty vector
        if (adjacencyList.find(nodeName) == adjacencyList.end()) {
            adjacencyList.insert({nodeName, std::vector<TfToken>()});
        }

        // Extract ancestor tokens within brackets
        std::getline(lineStream, token, '[');
        std::string ancestors;
        if (std::getline(lineStream, ancestors, ']')) {
            std::istringstream ancestorsStream(ancestors);
            std::string ancestor;
            while (std::getline(ancestorsStream, ancestor, ',')) {
                if (!ancestor.empty()) {
                    TfToken ancestorToken(Trim(ancestor));
                    auto nodeIt = adjacencyList.find(nodeName);
                    if (nodeIt != adjacencyList.end()) {
                        nodeIt->second.push_back(ancestorToken);
                    }
                    unresolvedNodes.insert(ancestorToken);
                }
            }
            // Check for final keyword after the closing bracket
            std::getline(lineStream, token);
            if (Trim(token) == "final") {
                finalNodes.insert(nodeName);
            }
        } else {
            result.errors.push_back("Error: Missing or unmatched brackets for node " + nodeName.GetString());
        }
    }

    void SecondPass(ParseResult& result) {
        for (const auto& node : unresolvedNodes) {
            if (declaredNodes.find(node) == declaredNodes.end()) {
                result.errors.push_back("Error: Undefined ancestor " + node.GetString());
            }
            if (finalNodes.find(node) != finalNodes.end()) {
                result.errors.push_back("Error: Node " + node.GetString() + " is declared as final and cannot be an ancestor.");
            }
        }
        TokenSet visited;
        TokenSet recursionStack;
        for (const auto& [node, _] : adjacencyList) {
            if (visited.find(node) == visited.end()) {
                if (DetectCycle(node, visited, recursionStack, result)) {
                    result.hasCycle = true;
                    return;
                }
            }
        }
    }

    bool DetectCycle(const TfToken& node, TokenSet& visited, TokenSet& recursionStack, ParseResult& result) {
        visited.insert(node);
        recursionStack.insert(node);

        auto nodeIt = adjacencyList.find(node);
        if (nodeIt != adjacencyList.end()) {
            for (const auto& ancestor : nodeIt->second) {
                if (recursionStack.find(ancestor) != recursionStack.end()) {
                    result.cyclePath.push_back(ancestor);
                    return true;
                }
                if (visited.find(ancestor) == visited.end() && DetectCycle(ancestor, visited, recursionStack, result)) {
                    result.cyclePath.push_back(node);
                    return true;
                }
            }
        }

        recursionStack.erase(node);
        return false;
    }

    std::string Trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t");
        size_t end = str.find_last_not_of(" \t");
        return (start == std::string::npos || end == std::string::npos) ? "" : str.substr(start, end - start + 1);
    }
};

int testProfiles() {
    const std::string input = R"(
root []
usd [root]
python [root]
usd.geom [usd]
usd.geom.subdiv [usd.geom]
usd.geom.skel [usd.geom]
usd.shadeMaterial [usd]
usd.imaging [usd.geom.subdiv, usd.shadeMaterial]
usd.format [usd]
usd.format.usda [usd.format]
usd.format.usdc [usd.format]
usd.format.usdz [usd.format.usda, usd.format.usdc]
usd.format.abc [usd.format, usd.geom]
usd.format.obj [usd.format, usd.geom]
usd.python [usd, python]
usd.physics [usd.geom]

apple [root]
apple.realityKit [usd.physics, usd.imaging, usd.geom.skel]
apple.format.realityKit [apple.realityKit, usd.format.usdz] final

mtlx [root]
usd.format.mtlx, [mtlx, usd.format, usd.geom, usd.shadeMaterial]
usd.format.mtlx, 1.38 [mtlx, usd.format, usd.geom, usd.shadeMaterial]
usd.format.mtlx, 1.39 [mtlx, usd.format, usd.geom, usd.shadeMaterial]

usd.format.gltf-read [usd.format, usd.geom, usd.shadeMaterial]
usd.format.gltf-write [usd.format, usd.geom, usd.shadeMaterial]
usd.format.gltf [usd.format.gltf-read, usd.format-gltf.write]

hd [usd.imaging]
hd.subdiv [hd]
hd.splat [hd]
hd.matx [hd, matx]
hd.hio [hd]
hd.hio.avif-read [hd.hio]
hd.hio.exr-read [hd.hio]
hd.hio.exr-read [hd.hio]
hd.hio.exr [hd.hio.exr-read, hd.hio.exr-write]
hd.hio.png-read [hd.hio]
)";

    DagParser parser;
    ParseResult result = parser.Parse(input);

    for (const auto& error : result.errors) {
        std::cout << error << std::endl;
    }
    if (result.hasCycle) {
        std::cout << "Cycle detected: ";
        for (const auto& token : result.cyclePath) {
            std::cout << token.GetString() << " ";
        }
        std::cout << std::endl;
    } 
    else {
        std::cout << "No cycles detected." << std::endl;
        std::cout << "\nTree view:\n";
        parser.PrintTree();
        std::cout << "\nDAG view:\n";
        parser.PrintDAG();
    }
    return 0;
}
