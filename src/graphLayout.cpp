#include "graphLayout.h"
#include <iostream>

// src/graphLayout.cpp
void GraphLayout::calculateGridLayout(
    std::vector<Node>& nodes,
    Vector2 centerPos,
    const std::unordered_map<int, std::vector<int>>& bookToGenreMap)
{
    m_TargetPositions.clear();

    std::vector<Node*> genres;
    std::vector<Node*> books;

    // Rozdìlíme uzly na žánry a knihy
    for (auto& n : nodes) {
        if (n.type == NodeType::Genre) genres.push_back(&n);
        else books.push_back(&n);
    }

    float spacingX = 280.0f; // Vzdálenost mezi knihami na polièce
    float spacingY = 300.0f; // Vzdálenost mezi jednotlivými žánry (øádky)

    // Výpoèet poèáteèního bodu
    // X posuneme více doleva, aby polièky mìly prostor rùst doprava
    float startX = centerPos.x - 600.0f;
    // Y vycentrujeme podle toho, kolik máme žánrù
    float startY = centerPos.y - (genres.size() * spacingY) / 2.0f;

    std::unordered_set<int> placedBooks; // Sem si zapisujeme, co už má místo
    int row = 0;

    // 1. Rozmístìní knih do polièek podle žánrù
    for (auto* g : genres) {
        // Uzel Žánru usadíme na úplný zaèátek øádku
        m_TargetPositions[g->id] = { startX, startY + row * spacingY };

        int col = 1; // Sloupec 0 je žánr, knihy zaèínají na 1

        for (auto* b : books) {
            // Pokud už kniha leží na jiné polièce, pøeskoèíme ji
            if (placedBooks.count(b->id)) continue;

            auto it = bookToGenreMap.find(b->id);
            if (it != bookToGenreMap.end()) {
                const auto& bookGenres = it->second;
                // Zkontrolujeme, jestli kniha patøí do aktuálního žánru
                if (std::find(bookGenres.begin(), bookGenres.end(), g->id) != bookGenres.end()) {
                    // Kniha patøí sem! Usadíme ji vedle.
                    m_TargetPositions[b->id] = { startX + col * spacingX, startY + row * spacingY };
                    placedBooks.insert(b->id);
                    col++;
                }
            }
        }
        row++; // Pøesun na další øádek (další žánr)
    }

    // 2. Úklid sirotkù (knihy, které nemají žádný žánr)
    int orphansCol = 0;
    for (auto* b : books) {
        if (!placedBooks.count(b->id)) {
            // Dáme je do úplnì posledního, extra øádku
            m_TargetPositions[b->id] = { startX + orphansCol * spacingX, startY + row * spacingY };
            orphansCol++;
        }
    }
}

bool GraphLayout::updateLerp(std::vector<Node>& nodes) {
    bool isMoving = false;
    float lerpSpeed = 0.05f; // 0.05 znamená, že uzel každý snímek uletí 5% zbývající cesty

    for (auto& n : nodes) {
        // Ignorujeme uzly, které uživatel zrovna drží myší
        if (!n.isDragged && m_TargetPositions.count(n.id)) {
            Vector2 target = m_TargetPositions[n.id];
            float dist = Vector2Distance(n.position, target);

            // Pokud jsme daleko, letíme
            if (dist > 1.0f) {
                // Vector2Lerp je super funkce z raymath.h pro plynulý pohyb
                n.position = Vector2Lerp(n.position, target, lerpSpeed);
                isMoving = true;
            }
            else {
                n.position = target; // Dorazili jsme
            }
        }
    }
    return isMoving;
}

bool GraphLayout::updatePhysics(
    std::vector<Node>& nodes,
    const std::unordered_map<int, std::vector<int>>& bookToGenreMap,
    Vector2 centerPos,
    float dt,
    Node* draggedNode)
{
    std::vector<Vector2> displacements(nodes.size(), { 0.0f, 0.0f });

    // ZÁKLAD: Všechny uzly jsou aktivní (pokud nic netáhneme)
    std::vector<bool> isActive(nodes.size(), true);

    // Pokud táhneme uzel, omezíme fyziku jen na propojené a blízké uzly
    if (draggedNode != nullptr) {
        float activeRadius = 800.0f; // Vzdálenost, ve které uzly uhýbají

        // Nejdøív vše uspíme
        std::fill(isActive.begin(), isActive.end(), false);

        for (size_t i = 0; i < nodes.size(); ++i) {
            // 1. Zóna kolem kurzoru (aby cizí uzly uhýbaly)
            if (Vector2Distance(nodes[i].position, draggedNode->position) < activeRadius) {
                isActive[i] = true;
            }
        }

        // 2. Tvoje pravidlo: Probuzení podle propojení (pøes žánry)
        if (draggedNode->type == NodeType::Book) {
            auto it = bookToGenreMap.find(draggedNode->id);
            if (it != bookToGenreMap.end()) {
                for (int genreId : it->second) {
                    for (size_t i = 0; i < nodes.size(); ++i) {
                        if (nodes[i].id == genreId && nodes[i].type == NodeType::Genre) isActive[i] = true;
                    }
                }
            }
        }
        else if (draggedNode->type == NodeType::Genre) {
            for (size_t i = 0; i < nodes.size(); ++i) {
                if (nodes[i].type == NodeType::Book) {
                    auto it = bookToGenreMap.find(nodes[i].id);
                    if (it != bookToGenreMap.end()) {
                        for (int gId : it->second) {
                            if (gId == draggedNode->id) isActive[i] = true;
                        }
                    }
                }
            }
        }
    }

    float repulsionStrength = 250000.0f;
    float attractionStrength = 0.05f;
    float centerGravity = 0.01f;

    // 1. ODPUZOVÁNÍ
    for (size_t i = 0; i < nodes.size(); ++i) {
        for (size_t j = i + 1; j < nodes.size(); ++j) {
            // Pokud OBA spí, nemusíme je øešit
            if (!isActive[i] && !isActive[j]) continue;

            Vector2 delta = Vector2Subtract(nodes[i].position, nodes[j].position);
            float dist = Vector2Length(delta);

            if (dist < 0.1f) {
                delta = { (float)(rand() % 100) / 100.0f, (float)(rand() % 100) / 100.0f };
                dist = Vector2Length(delta);
            }

            float force = repulsionStrength / (dist * dist);

            if (nodes[i].type == NodeType::Genre && nodes[j].type == NodeType::Genre) {
                force *= 10.0f;
            }

            Vector2 repulseVector = Vector2Scale(Vector2Normalize(delta), force);

            // Aplikujeme sílu pouze tìm uzlùm, které nespí
            if (isActive[i]) displacements[i] = Vector2Add(displacements[i], repulseVector);
            if (isActive[j]) displacements[j] = Vector2Subtract(displacements[j], repulseVector);
        }
    }

    // 2. PØITAHOVÁNÍ (Pružiny)
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].type != NodeType::Book) continue;

        auto it = bookToGenreMap.find(nodes[i].id);
        if (it == bookToGenreMap.end()) continue;

        for (int genreId : it->second) {
            size_t genreIdx = 0;
            bool found = false;
            for (size_t j = 0; j < nodes.size(); ++j) {
                if (nodes[j].id == genreId && nodes[j].type == NodeType::Genre) {
                    genreIdx = j; found = true; break;
                }
            }

            if (found) {
                // Pokud obì strany spí, neøešíme pružinu
                if (!isActive[i] && !isActive[genreIdx]) continue;

                Vector2 delta = Vector2Subtract(nodes[genreIdx].position, nodes[i].position);
                float dist = Vector2Length(delta);
                float idealLength = nodes[i].radius + nodes[genreIdx].radius + 80.0f;

                if (dist > idealLength) {
                    float force = (dist - idealLength) * attractionStrength;
                    Vector2 attractVector = Vector2Scale(Vector2Normalize(delta), force);

                    if (isActive[i]) displacements[i] = Vector2Add(displacements[i], attractVector);
                    if (isActive[genreIdx]) displacements[genreIdx] = Vector2Subtract(displacements[genreIdx], attractVector);
                }
            }
        }
    }

    // 3. GRAVITACE KE STØEDU
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (!isActive[i]) continue;

        Vector2 delta = Vector2Subtract(centerPos, nodes[i].position);
        float distFromCenter = Vector2Length(delta);

        if (distFromCenter > 300.0f) {
            Vector2 gravityVec = Vector2Scale(Vector2Normalize(delta), (distFromCenter - 300.0f) * centerGravity);
            displacements[i] = Vector2Add(displacements[i], gravityVec);
        }
    }

    float totalMovement = 0.0f;

    // 4. APLIKACE SIL NA POZICI
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (!isActive[i]) continue;

        if (!nodes[i].locked && !nodes[i].isDragged) {

            // ZMÌNA 1: Aplikujeme tlumení teplotou hned pøi výpoètu posunu
            Vector2 actualMove = Vector2Scale(displacements[i], dt * m_Temperature);

            // ZMÌNA 2: Omezíme až tento zchlazený pohyb (aby uzly na zaèátku neuletìly)
            float moveLength = Vector2Length(actualMove);
            if (moveLength > 30.0f) {
                actualMove = Vector2Scale(Vector2Normalize(actualMove), 30.0f);
            }

            nodes[i].position = Vector2Add(nodes[i].position, actualMove);
            totalMovement += Vector2Length(actualMove);
        }
    }

    // 5. TVRDÉ KOLIZE
    totalMovement += resolveNodeOverlaps(20.0f, nodes, isActive);

    m_Temperature *= 0.98f;

    // Pokud už graf skoro vychladl, natvrdo ho zmrazíme (vrátíme false -> fyzika se vypne)
    if (m_Temperature < 0.05f) {
        m_Temperature = 0.0f;
        return false;
    }

    return true;
}


float GraphLayout::resolveNodeOverlaps(float padding, std::vector<Node>& nodes, const std::vector<bool>& isActive)
{
    float maxDisplacement = 15.0f;
    float overlapMovement = 0.0f;

    for (size_t i = 0; i < nodes.size(); ++i) {
        for (size_t j = i + 1; j < nodes.size(); ++j) {
            // Kolizi neøešíme, pokud se na sebe tlaèí dva spící uzly
            if (!isActive[i] && !isActive[j]) continue;

            Node& a = nodes[i];
            Node& b = nodes[j];

            float totalRadius = a.radius + b.radius + padding;
            Vector2 delta = Vector2Subtract(b.position, a.position);
            float dist = Vector2Length(delta);

            if (dist < 0.01f) {
                float angle = (float)(rand() % 360) * DEG2RAD;
                delta = { cos(angle), sin(angle) };
                dist = 1.0f;
            }

            if (dist < totalRadius) {
                Vector2 direction = Vector2Normalize(delta);
                float overlap = totalRadius - dist;
                Vector2 displacement = Vector2Scale(direction, std::min(overlap * 0.5f, maxDisplacement));

                // Neaktivní uzly se chovají, jako by byly locked (zamèené), takže aktivní se od nich odráží
                bool aFixed = a.locked || a.isDragged || !isActive[i];
                bool bFixed = b.locked || b.isDragged || !isActive[j];

                float aMove = aFixed ? 0.0f : (bFixed ? 1.0f : 0.5f);
                float bMove = bFixed ? 0.0f : (aFixed ? 1.0f : 0.5f);

                if (aMove > 0.0f) {
                    Vector2 moveVec = Vector2Scale(displacement, aMove);
                    a.position = Vector2Subtract(a.position, moveVec);
                    overlapMovement += Vector2Length(moveVec);
                }
                if (bMove > 0.0f) {
                    Vector2 moveVec = Vector2Scale(displacement, bMove);
                    b.position = Vector2Add(b.position, moveVec);
                    overlapMovement += Vector2Length(moveVec);
                }
            }
        }
    }
    return overlapMovement;
}