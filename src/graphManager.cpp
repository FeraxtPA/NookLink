#include "graphManager.h"
#include <iostream>

#include <sstream>

const std::string GraphManager::getGenreNameByNodeId(int nodeId) const {
    auto opt = getGenreByNodeId(nodeId);
    return opt.value_or("Unknown");
}

bool GraphManager::TryGrabNodeAt(Vector2 mousePos, bool isShiftPressed)
{
    for (auto& node : m_Nodes) {
        
        if (Vector2Distance(mousePos, node.position) <= node.radius) {
            node.isDragged = true;

            
            if (isShiftPressed && node.type != NodeType::Genre) {
                node.locked = true;
            }

            setDraggedNode(&node);
            return true; 
        }
    }
    return false;
}

void GraphManager::releaseDraggedNode()
{
    if(draggedNode) {
		draggedNode->isDragged = false;
		setDraggedNode(nullptr);
	}
}

bool GraphManager::updateDraggedNodePosition(Vector2 mousePos)
{
    if(draggedNode) {
		draggedNode->position = mousePos;
		if (draggedNode->type == NodeType::Genre) {
			updateGenrePosition(draggedNode->id, mousePos);
		}
		return true;
	}
}

bool GraphManager::tryUnlockNodeAt(Vector2 mousePos)
{
    Node* node = getNodeAtPosition(mousePos);
    if (node && node->locked) {
        node->locked = false;
        return true;

    }
    return false;
}

const std::optional<std::string> GraphManager::getGenreByNodeId(int nodeId) const {
    for (const auto& [name, info] : m_Genres) {
        if (info.nodeId == nodeId) return name;
    }
    return std::nullopt;
}

Node* GraphManager::getNodeAtPosition(Vector2 mousePos)
{
    for (auto& node : m_Nodes) {
        float dist = Vector2Distance(mousePos, node.position);
        if (dist <= node.radius) {
            return &node;
        }
    }
    return nullptr;
}

void GraphManager::updatePhysics(float dt) {

    if (!m_IsPhysicsActive) return;


    if (m_LayoutMode == LayoutMode::Grid) {
        m_IsPhysicsActive = m_GraphLayout.updateLerp(m_Nodes);
        return;
    }

    std::unordered_map<int, std::vector<int>> bookToGenreMap;

    // Vytvoøíme si mapování (Kniha -> Její žánry)
    for (const auto& book : m_BookManager.getBooks()) {
        std::vector<int> connectedGenreNodeIds;
        for (const auto& genreStr : book.getGenres()) {
            auto it = m_Genres.find(genreStr);
            if (it != m_Genres.end()) {
                connectedGenreNodeIds.push_back(it->second.nodeId);
            }
        }
        bookToGenreMap[book.getId()] = connectedGenreNodeIds;
    }

    Vector2 centerPos = { m_CanvasSize.x / 2.0f, m_CanvasSize.y / 2.0f };

    // Zavoláme naši novou výpoèetní fyziku z GraphLayoutu
    m_IsPhysicsActive = m_GraphLayout.updatePhysics(m_Nodes, bookToGenreMap, centerPos, dt);
}



void GraphManager::drawEdges(float zoom, const Rectangle& viewRect) {

    m_NodeRenderer.drawEdges(m_ConnectionManager, m_Nodes, viewRect, zoom);
}

int GraphManager::getNumOfConnectedBooks(int genreNodeId) const
{
    int counter = 0;
    for (const auto& edge : m_ConnectionManager.getEdges()) {
		if (edge.type == EdgeType::BookToGenre && edge.toId == genreNodeId) {
			counter++;
		}
	}
    return counter;
}


void GraphManager::resetNodeState() {
    m_Nodes.clear();
    m_Genres.clear();
    m_GenreIdBase = -1;
}

float GraphManager::calculateCircleRadius(int nodeCount, float minRadius) const
{
    if (nodeCount == 0) return minRadius;
    return std::max(minRadius, (nodeCount * 2.0f * 100.0f) / (2.0f * PI)); 
}

void GraphManager::placeGenreNodes(const std::unordered_set<std::string>& genres,
    const std::unordered_map<std::string, int>& genreIdMap,
    const std::unordered_map<std::string, Vector2>& oldPositions)
{
    Vector2 canvasCenter = { m_CanvasSize.x / 2.0f, m_CanvasSize.y / 2.0f };
    float nodeRadius = 100.0f;

    // Calculate the radius of the large circle on which genres will be placed
    float genreCircleRadius = calculateCircleRadius((int)genres.size(), m_CanvasSize.x / 3.0f);

    int i = 0;
    for (const auto& genreStr : genres) {

       
        float angle = 2.0f * PI * i++ / genres.size();
        Vector2 pos = {
            canvasCenter.x + genreCircleRadius * cos(angle),
            canvasCenter.y + genreCircleRadius * sin(angle)
        };

        
        if (oldPositions.find(genreStr) != oldPositions.end()) {
            pos = oldPositions.at(genreStr);
        }

        auto it = genreIdMap.find(genreStr);
        if (it == genreIdMap.end()) {
            std::cerr << "Warning: Genre ID not found for genre " << genreStr << "\n";
            continue;
        }
        int genreId = it->second;

        float genreRadius = nodeRadius * 1.5f;

        m_Nodes.push_back({ genreId, NodeType::Genre, pos, genreRadius });
        m_Genres[genreStr] = { genreId, pos };
    }
}


void GraphManager::placeBookNodes(const std::unordered_map<int, Vector2>& oldPositions) {
    Vector2 canvasCenter = { m_CanvasSize.x / 2.0f, m_CanvasSize.y / 2.0f };
    float nodeRadius = 100.0f;
    float offsetDistance = 250.0f;

    static std::default_random_engine rng(std::random_device{}());
    std::uniform_real_distribution<float> distAngle(0.0f, 2.0f * PI);

    for (const auto& book : m_BookManager.getBooks()) {

        
        if (oldPositions.find(book.getId()) != oldPositions.end()) {
            m_Nodes.push_back({ book.getId(), NodeType::Book, oldPositions.at(book.getId()), nodeRadius });
            continue; // Skip the rest of the calculation logic for this book
        }

        // New position
        Vector2 avgPos = {};
        int count = 0;

        for (const auto& genreStr : book.getGenres()) {
            auto it = m_Genres.find(genreStr);
            if (it != m_Genres.end()) {
                avgPos = Vector2Add(avgPos, it->second.position);
                count++;
            }
        }

        avgPos = (count > 0) ? Vector2Scale(avgPos, 1.0f / count) : canvasCenter;

        float angle = distAngle(rng);
        Vector2 pos = {
            avgPos.x + offsetDistance * cos(angle),
            avgPos.y + offsetDistance * sin(angle)
        };

        m_Nodes.push_back({ book.getId(), NodeType::Book, pos, nodeRadius });
    }
}


bool GraphManager::isNodeVisible(const Node& node, const Rectangle& viewRect) {
    return !(node.position.x + node.radius < viewRect.x ||
        node.position.x - node.radius > viewRect.x + viewRect.width ||
        node.position.y + node.radius < viewRect.y ||
        node.position.y - node.radius > viewRect.y + viewRect.height);
}

GraphManager::GraphManager(const BookManager& bm, ConnectionManager& cm, Vector2 canvasSize, TextRenderer* tr)
    : m_BookManager(bm), m_ConnectionManager(cm), m_NodeRenderer(bm, tr), m_CanvasSize(canvasSize)
{
}


void GraphManager::initializePositions()
{

    std::unordered_map<int, Vector2> oldBookPos;
    std::unordered_map<std::string, Vector2> oldGenrePos;

    for (const auto& node : m_Nodes) {
        if (node.type == NodeType::Book) {
            oldBookPos[node.id] = node.position;
        }
        else if (node.type == NodeType::Genre) {
            
            auto nameOpt = getGenreByNodeId(node.id);
            if (nameOpt.has_value()) {
                oldGenrePos[nameOpt.value()] = node.position;
            }
        }
    }

    m_ConnectionManager.updateConnections(m_BookManager.getBooks());

    resetNodeState();

    placeGenreNodes(m_ConnectionManager.getExistingGenres(), m_ConnectionManager.getGenreIdMap(), oldGenrePos);
    placeBookNodes(oldBookPos);

    for (auto& node : m_Nodes) {
        m_NodeIdMap[node.id] = &node;
    }

}

void GraphManager::removeNodeById(int id) {

    
    auto removeSingleNode = [&](int targetId) {
       
        m_Nodes.erase(
            std::remove_if(m_Nodes.begin(), m_Nodes.end(),
                [targetId](const Node& node) { return node.id == targetId; }),
            m_Nodes.end()
        );

        // Remove edges connected to this node
        m_ConnectionManager.removeEdgesConnectedToNode(targetId);

        //Remove from m_Genres map if it's a genre node
        for (auto it = m_Genres.begin(); it != m_Genres.end(); ) {
            if (it->second.nodeId == targetId) {
                it = m_Genres.erase(it);
            }
            else {
                ++it;
            }
        }

        // Clear dragged pointer if we just deleted it
        if (draggedNode && draggedNode->id == targetId) {
            draggedNode = nullptr;
        }
        };

    //Remove book
    removeSingleNode(id);


    //Check if any genre is now alone
    std::vector<int> emptyGenres;

    for (const auto& node : m_Nodes) {
        if (node.type == NodeType::Genre) {
            
            if (getNumOfConnectedBooks(node.id) == 0) {
                emptyGenres.push_back(node.id);
            }
        }
    }

    // Remove the identified empty genres
    for (int genreId : emptyGenres) {
        std::cout << "Auto-removing empty Genre Node ID: " << genreId << std::endl;
        removeSingleNode(genreId);
    }


    //Rebuild map
    m_NodeIdMap.clear();
    for (auto& node : m_Nodes) {
        m_NodeIdMap[node.id] = &node;
    }
}



const Rectangle GraphManager::getCameraViewRect(const Camera2D& camera, Vector2 screenDimensions) {
    float left = camera.target.x - (static_cast<float>(screenDimensions.x) / 2) / camera.zoom;
    float top = camera.target.y - (static_cast<float>(screenDimensions.y) / 2) / camera.zoom;
    float width = screenDimensions.x / camera.zoom;
    float height = screenDimensions.y / camera.zoom;
    return { left, top, width, height };
}



std::string statusEnumToString(Status s) {
    switch (s) {
    case Status::ToRead: return "to read";
    case Status::Reading: return "reading";
    case Status::Read: return "read";
    default: return "";
    }
}

void GraphManager::drawNodes(float zoom, const Rectangle& viewRect) {
    bool searchActive = !m_SearchQuery.empty();

    //Rules for  searching, should be moved out to a different function/class parser
    enum class RuleType { Text, RatingGreater, RatingLower, RatingEqual, Status, Genre };

    struct FilterRule {
        RuleType type = RuleType::Text;
        float ratingVal = 0.0f;
        std::string stringVal = ""; // Stores text, status, or genre target
    };

    std::vector<FilterRule> rules;

    //Parsing on "|"
    if (searchActive) {
        std::stringstream ss(m_SearchQuery);
        std::string segment;

        while (std::getline(ss, segment, '|')) {
            // Trim Whitespace
            size_t first = segment.find_first_not_of(" ");
            if (first == std::string::npos) continue; // Skip empty segments
            size_t last = segment.find_last_not_of(" ");
            segment = segment.substr(first, (last - first + 1));

            //Convert to Lowercase
            std::transform(segment.begin(), segment.end(), segment.begin(), ::tolower);

            FilterRule rule;

            // Determine Rule Type
            // Check for "rating >"
            if (segment.find("r>") != std::string::npos || segment.find("rating>") != std::string::npos) {
                rule.type = RuleType::RatingGreater;
                size_t pos = segment.find('>');
                try { rule.ratingVal = std::stof(segment.substr(pos + 1)); }
                catch (...) {}
            }
            // Check for "rating <"
            else if (segment.find("r<") != std::string::npos || segment.find("rating<") != std::string::npos) {
                rule.type = RuleType::RatingLower;
                size_t pos = segment.find('<');
                try { rule.ratingVal = std::stof(segment.substr(pos + 1)); }
                catch (...) { rule.ratingVal = 5.0f; }
            }
            // Check for "rating =" or "rating :"
            else if (segment.find("r=") != std::string::npos || segment.find("rating=") != std::string::npos ||
                segment.find("r:") != std::string::npos || segment.find("rating:") != std::string::npos) {
                rule.type = RuleType::RatingEqual;
                size_t pos = segment.find('=');
                if (pos == std::string::npos) pos = segment.find(':');
                try { rule.ratingVal = std::stof(segment.substr(pos + 1)); }
                catch (...) {}
            }
            // Check for "status:"
            else if (segment.find("s:") != std::string::npos) {
                rule.type = RuleType::Status;
                size_t pos = segment.find(':');
                rule.stringVal = segment.substr(pos + 1);
                // Trim value inside the rule
                if (rule.stringVal.find_first_not_of(" ") != std::string::npos)
                    rule.stringVal.erase(0, rule.stringVal.find_first_not_of(" "));
            }
            // Check for "genre:"
            else if (segment.find("g:") != std::string::npos) {
                rule.type = RuleType::Genre;
                size_t pos = segment.find(':');
                rule.stringVal = segment.substr(pos + 1);
                if (rule.stringVal.find_first_not_of(" ") != std::string::npos)
                    rule.stringVal.erase(0, rule.stringVal.find_first_not_of(" "));
            }
            // Default to Text Search
            else {
                rule.type = RuleType::Text;
                rule.stringVal = segment;
            }

            rules.push_back(rule);
        }
    }

    
    for (const auto& node : m_Nodes) {
        if (!isNodeVisible(node, viewRect)) continue;

        if(!node.visible) continue;
       
        bool isDimmed = false;

        if (searchActive && !rules.empty()) {

           
            bool matchesAll = true;

            for (const auto& rule : rules) {
                bool ruleMatch = false;

                
                if (node.type == NodeType::Book) {
                    const Book* b = m_BookManager.findBookById(node.id);
                    if (b) {
                        if (rule.type == RuleType::RatingGreater) {
                            if (b->getRating() >= rule.ratingVal) ruleMatch = true;
                        }
                        else if (rule.type == RuleType::RatingLower) {
                            if (b->getRating() <= rule.ratingVal) ruleMatch = true;
                        }
                        else if (rule.type == RuleType::RatingEqual) {
                            if (std::abs(b->getRating() - rule.ratingVal) < 0.01f) ruleMatch = true;
                        }
                        else if (rule.type == RuleType::Status) {
                            std::string s = statusEnumToString(b->getStatus());
                           
                            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
                            if (s.find(rule.stringVal) != std::string::npos) ruleMatch = true;
                        }
                        else if (rule.type == RuleType::Genre) {
                            for (const auto& genreStr : b->getGenres()) {
                                std::string g = genreStr;
                                std::transform(g.begin(), g.end(), g.begin(), ::tolower);
                                if (g.find(rule.stringVal) != std::string::npos) {
                                    ruleMatch = true;
                                    break;
                                }
                            }
                        }
                        else if (rule.type == RuleType::Text) {
                            std::string t = b->getTitle();
                            std::transform(t.begin(), t.end(), t.begin(), ::tolower);
                            std::string a = b->getAuthor();
                            std::transform(a.begin(), a.end(), a.begin(), ::tolower);
                            if (t.find(rule.stringVal) != std::string::npos ||
                                a.find(rule.stringVal) != std::string::npos) {
                                ruleMatch = true;
                            }
                        }
                    }
                }
               
                else if (node.type == NodeType::Genre) {
                 

                    if (rule.type == RuleType::Genre || rule.type == RuleType::Text) {
                        std::string g = getGenreNameByNodeId(node.id);
                        std::transform(g.begin(), g.end(), g.begin(), ::tolower);
                        if (g.find(rule.stringVal) != std::string::npos) ruleMatch = true;
                    }
                }

              
                if (!ruleMatch) {
                    matchesAll = false;
                    break; 
                }
            }

            if (!matchesAll) isDimmed = true;
        }

        m_NodeRenderer.drawNode(node, zoom, m_Genres, isDimmed);
    }
}

void GraphManager::recalculateVisibility()
{
    for (auto& node : m_Nodes) {
       
        node.visible = true;

        if (node.type == NodeType::Book) {
            const Book* b = m_BookManager.findBookById(node.id);
            if (!b) continue;

            
            if (m_HiddenStatuses.count(b->getStatus())) {
                node.visible = false;
                continue; 
            }

            // Check Genre (Hide book if *any* of its genres are hidden)
            // Alternatively: Hide only if *all* genres are hidden. 
            // Below implements "Hide if any genre matches the blocklist"
            for (const auto& g : b->getGenres()) {
                if (m_HiddenGenres.count(g)) {
                    node.visible = false;
                    break;
                }
            }
        }
        else if (node.type == NodeType::Genre) {
           
            std::string genreName = getGenreNameByNodeId(node.id);
            if (m_HiddenGenres.count(genreName)) {
                node.visible = false;
            }
        }
    }
}

void GraphManager::drawNode(const Node& node,float zoom) {

    m_NodeRenderer.drawNode(node, zoom, m_Genres);
}

void GraphManager::updateGenrePosition(int nodeId, Vector2 newPos) {
    // Update position inside m_Genres for the genre that matches nodeId
    for (auto& [genre, info] : m_Genres) {
        if (info.nodeId == nodeId) {
            info.position = newPos;
            break;
        }
    }
   
}

void GraphManager::setLayoutMode(LayoutMode mode)
{
    m_LayoutMode = mode;

    if (m_LayoutMode == LayoutMode::Grid) {
        Vector2 centerPos = { m_CanvasSize.x / 2.0f, m_CanvasSize.y / 2.0f };
        m_GraphLayout.calculateGridLayout(m_Nodes, centerPos);
        m_IsPhysicsActive = true; // Probudíme graf, aby bubliny mohly odletìt do møížky
    }
    else {
        wakeUpPhysics(); // Probudíme fyziku, aby se chaos mohl rozjet nanovo
    }
}




