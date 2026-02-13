#include "graphLayout.h"
#include <iostream>

void GraphLayout::calculateGridLayout(std::vector<Node>& nodes, Vector2 centerPos) {
    m_TargetPositions.clear();

    std::vector<Node*> genres;
    std::vector<Node*> books;

    // Rozdìlíme uzly na žánry a knihy
    for (auto& n : nodes) {
        if (n.type == NodeType::Genre) genres.push_back(&n);
        else books.push_back(&n);
    }

    // Parametry møížky
    int maxCols = 6;              // Poèet sloupcù
    float spacingX = 250.0f;      // Mezera mezi sloupci
    float spacingY = 250.0f;      // Mezera mezi øádky

    // Výpoèet poèáteèního bodu, aby byla møížka zhruba vycentrovaná
    float startX = centerPos.x - ((maxCols - 1) * spacingX) / 2.0f;
    float startY = centerPos.y - 400.0f; // Zaèínáme trochu výš

    int col = 0, row = 0;

    // 1. Rozmístìní žánrù (do horních øad)
    for (auto* g : genres) {
        m_TargetPositions[g->id] = { startX + col * spacingX, startY + row * spacingY };
        col++;
        if (col >= maxCols) { col = 0; row++; }
    }

    // Odøádkování a vizuální mezera mezi žánry a knihami
    if (col > 0) { col = 0; row++; }
    row++; // Extra volná øada pro vizuální oddìlení

    // 2. Rozmístìní knih
    for (auto* b : books) {
        m_TargetPositions[b->id] = { startX + col * spacingX, startY + row * spacingY };
        col++;
        if (col >= maxCols) { col = 0; row++; }
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
    float dt)
{
    std::vector<Vector2> displacements(nodes.size(), { 0.0f, 0.0f });

    float repulsionStrength = 250000.0f; // Drasticky zvýšeno: uzly se budou víc odstrkovat
    float attractionStrength = 0.05f;    // Mírnì zvýšeno: knihy se budou lépe držet svého žánru
    float centerGravity = 0.01f;

    // 1. ODPUZOVÁNÍ (Všechny uzly se navzájem odpuzují)
    for (size_t i = 0; i < nodes.size(); ++i) {
        for (size_t j = i + 1; j < nodes.size(); ++j) {
            Vector2 delta = Vector2Subtract(nodes[i].position, nodes[j].position);
            float dist = Vector2Length(delta);

            if (dist < 0.1f) { // Zabrání dìlení nulou
                delta = { (float)(rand() % 100) / 100.0f, (float)(rand() % 100) / 100.0f };
                dist = Vector2Length(delta);
            }

            // Výpoèet síly (èím blíž, tím silnìjší)
            float force = repulsionStrength / (dist * dist);

            // Žánry se odpuzují navzájem MNOHEM víc, aby vytvoøily hlavní rozcestníky
            if (nodes[i].type == NodeType::Genre && nodes[j].type == NodeType::Genre) {
                force *= 10.0f;
            }

            Vector2 repulseVector = Vector2Scale(Vector2Normalize(delta), force);
            displacements[i] = Vector2Add(displacements[i], repulseVector);
            displacements[j] = Vector2Subtract(displacements[j], repulseVector);
        }
    }

    // 2. PØITAHOVÁNÍ (Pružiny z knih do jejich žánrù)
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].type != NodeType::Book) continue;

        auto it = bookToGenreMap.find(nodes[i].id);
        if (it == bookToGenreMap.end()) continue;

        for (int genreId : it->second) {
            // Najdeme index žánru
            size_t genreIdx = 0;
            bool found = false;
            for (size_t j = 0; j < nodes.size(); ++j) {
                if (nodes[j].id == genreId && nodes[j].type == NodeType::Genre) {
                    genreIdx = j; found = true; break;
                }
            }

            if (found) {
                Vector2 delta = Vector2Subtract(nodes[genreIdx].position, nodes[i].position);
                float dist = Vector2Length(delta);

                // Kniha se chce držet na této vzdálenosti od žánru
                float idealLength = nodes[i].radius + nodes[genreIdx].radius + 80.0f;

                if (dist > idealLength) {
                    float force = (dist - idealLength) * attractionStrength;
                    Vector2 attractVector = Vector2Scale(Vector2Normalize(delta), force);

                    displacements[i] = Vector2Add(displacements[i], attractVector);
                    displacements[genreIdx] = Vector2Subtract(displacements[genreIdx], attractVector);
                }
            }
        }
    }

    // 3. GRAVITACE (Jemné tažení ke støedu plátna)
    for (size_t i = 0; i < nodes.size(); ++i) {
        Vector2 delta = Vector2Subtract(centerPos, nodes[i].position);
        float distFromCenter = Vector2Length(delta);

        // Gravitace zaène pùsobit silnìji až když uzel utíká moc daleko (napø. víc než 300 pixelù od støedu)
        if (distFromCenter > 300.0f) {
            Vector2 gravityVec = Vector2Scale(Vector2Normalize(delta), (distFromCenter - 300.0f) * centerGravity);
            displacements[i] = Vector2Add(displacements[i], gravityVec);
        }
    }

    float totalMovement = 0.0f;

    // 4. APLIKACE VŠECH SIL
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (!nodes[i].locked && !nodes[i].isDragged) {
            float moveLength = Vector2Length(displacements[i]);
            if (moveLength > 30.0f) {
                displacements[i] = Vector2Scale(Vector2Normalize(displacements[i]), 30.0f);
            }

            // Pohneme uzlem a pøièteme to do poèítadla
            Vector2 actualMove = Vector2Scale(displacements[i], dt);
            nodes[i].position = Vector2Add(nodes[i].position, actualMove);

            totalMovement += Vector2Length(actualMove);
        }
    }

    // 5. TVRDÉ KOLIZE (Finální ošetøení, aby bubliny opravdu nesedìly na sobì)
    totalMovement += resolveNodeOverlaps(20.0f, nodes); 

    
    return totalMovement > 100.0f; // Pokud se uzly pohybují ménì než 100 pixelù, považujeme rozložení za stabilní
}


float GraphLayout::resolveNodeOverlaps(float padding, std::vector<Node>& nodes)
{
    float maxDisplacement = 15.0f;
    float overlapMovement = 0.0f; // NOVÉ

    for (size_t i = 0; i < nodes.size(); ++i) {
        for (size_t j = i + 1; j < nodes.size(); ++j) {
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

                bool aFixed = a.locked || a.isDragged;
                bool bFixed = b.locked || b.isDragged;

                float aMove = aFixed ? 0.0f : (bFixed ? 1.0f : 0.5f);
                float bMove = bFixed ? 0.0f : (aFixed ? 1.0f : 0.5f);

                // NOVÉ: Pøièítání pohybu pøi kolizích
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