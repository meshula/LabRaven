#include <Tf/hashmap.h>
#include <Tf/hashset.h>
#include <Tf/Token.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

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
        adjacencyList[nodeName]; // Ensure the node is registered in the map

        // Extract ancestor tokens within brackets
        std::getline(lineStream, token, '[');
        std::string ancestors;
        if (std::getline(lineStream, ancestors, ']')) {
            std::istringstream ancestorsStream(ancestors);
            std::string ancestor;
            while (std::getline(ancestorsStream, ancestor, ',')) {
                if (!ancestor.empty()) {
                    TfToken ancestorToken(Trim(ancestor));
                    adjacencyList[nodeName].push_back(ancestorToken);
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

        for (const auto& ancestor : adjacencyList[node]) {
            if (recursionStack.find(ancestor) != recursionStack.end()) {
                result.cyclePath.push_back(ancestor);
                return true;
            }
            if (visited.find(ancestor) == visited.end() && DetectCycle(ancestor, visited, recursionStack, result)) {
                result.cyclePath.push_back(node);
                return true;
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

int main() {
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

matx [root]
usd.format.matx, [matx, usd.format, usd.geom, usd.shadeMaterial]
usd.format.matx, 1.38 [matx, usd.format, usd.geom, usd.shadeMaterial]
usd.format.matx, 1.39 [matx, usd.format, usd.geom, usd.shadeMaterial]

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
    } else {
        std::cout << "No cycles detected." << std::endl;
    }
    return 0;
}
