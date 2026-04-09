#include "graphLayout.h"
#include <algorithm>

namespace {
float SmoothStep01(float t) {
    const float x = std::clamp(t, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}
}

// src/graphLayout.cpp
void GraphLayout::calculateGridLayout(
    std::vector<Node>& nodes,
    Vector2 centerPos,
    const std::unordered_map<int, std::vector<int>>& bookToGenreMap)
{
    m_TargetPositions.clear();

    std::vector<Node*> genres;
    std::vector<Node*> books;

    // Rozd�l�me uzly na ��nry a knihy
    for (auto& n : nodes) {
        if (n.type == NodeType::Genre) genres.push_back(&n);
        else books.push_back(&n);
    }

    float spacingX = 280.0f; // Vzd�lenost mezi knihami na poli�ce
    float spacingY = 300.0f; // Vzd�lenost mezi jednotliv�mi ��nry (��dky)

    // V�po�et po��te�n�ho bodu
    // X posuneme v�ce doleva, aby poli�ky m�ly prostor r�st doprava
    float startX = centerPos.x - 600.0f;
    // Y vycentrujeme podle toho, kolik m�me ��nr�
    float startY = centerPos.y - (genres.size() * spacingY) / 2.0f;

    std::unordered_set<int> placedBooks; // Sem si zapisujeme, co u� m� m�sto
    int row = 0;

    // 1. Rozm�st�n� knih do poli�ek podle ��nr�
    for (auto* g : genres) {
        // Uzel ��nru usad�me na �pln� za��tek ��dku
        m_TargetPositions[g->id] = { startX, startY + row * spacingY };

        int col = 1; // Sloupec 0 je ��nr, knihy za��naj� na 1

        for (auto* b : books) {
            // Pokud u� kniha le�� na jin� poli�ce, p�esko��me ji
            if (placedBooks.count(b->id)) continue;

            auto it = bookToGenreMap.find(b->id);
            if (it != bookToGenreMap.end()) {
                const auto& bookGenres = it->second;
                // Zkontrolujeme, jestli kniha pat�� do aktu�ln�ho ��nru
                if (std::find(bookGenres.begin(), bookGenres.end(), g->id) != bookGenres.end()) {
                    // Kniha pat�� sem! Usad�me ji vedle.
                    m_TargetPositions[b->id] = { startX + col * spacingX, startY + row * spacingY };
                    placedBooks.insert(b->id);
                    col++;
                }
            }
        }
        row++; // P�esun na dal�� ��dek (dal�� ��nr)
    }

    // 2. �klid sirotk� (knihy, kter� nemaj� ��dn� ��nr)
    int orphansCol = 0;
    for (auto* b : books) {
        if (!placedBooks.count(b->id)) {
            // D�me je do �pln� posledn�ho, extra ��dku
            m_TargetPositions[b->id] = { startX + orphansCol * spacingX, startY + row * spacingY };
            orphansCol++;
        }
    }

    // P�i p�epnut� Physics -> Grid lehce oh�ejeme syst�m a na chv�li zastav�me cooldown,
    // aby uzly m�ly �as plynule dojet do nov�ch c�lov�ch pozic.
    m_GridStartPositions.clear();
    for (const auto& node : nodes) {
        m_GridStartPositions[node.id] = node.position;
    }
    m_GridTransitionT = 0.0f;

    m_Temperature = std::max(m_Temperature, m_MinTransitionTemperature);
    m_CoolingHoldFrames = m_GridTransitionHoldFrames;
}

bool GraphLayout::updateLerp(std::vector<Node>& nodes, float dt) {
    bool isMoving = false;

    // Vy��� teplota = svi�n�j�� p�ibl�en� k c�li; dr�� transition �ivou po toggle.
    const float tempBoost = std::clamp(m_Temperature, 0.0f, 1.0f);
    const float transitionDuration = m_GridTransitionDuration * (1.15f - 0.35f * tempBoost);
    m_GridTransitionT = std::min(1.0f, m_GridTransitionT + (dt / std::max(transitionDuration, 0.01f)));
    const float alpha = SmoothStep01(m_GridTransitionT);

    for (auto& n : nodes) {
        // Ignorujeme uzly, kter� u�ivatel zrovna dr�� my��
        if (!n.isDragged && m_TargetPositions.count(n.id)) {
            const Vector2 target = m_TargetPositions[n.id];
            const Vector2 start = m_GridStartPositions.count(n.id) ? m_GridStartPositions[n.id] : n.position;
            float dist = Vector2Distance(n.position, target);

            if (m_GridTransitionT < 1.0f) {
                n.position = Vector2Lerp(start, target, alpha);
                if (Vector2Distance(n.position, target) > 0.5f) {
                    isMoving = true;
                }
            }
            else if (dist > 0.5f) {
                n.position = target;
                isMoving = true;
            }
            else {
                n.position = target; // Dorazili jsme
            }
        }
    }

    if (m_CoolingHoldFrames > 0) {
        --m_CoolingHoldFrames;
    }
    else {
        m_Temperature *= m_GridCoolingFactor;
        if (m_Temperature < 0.0f) {
            m_Temperature = 0.0f;
        }
    }

    return m_GridTransitionT < 1.0f || isMoving;
}

bool GraphLayout::updatePhysics(
    std::vector<Node>& nodes,
    const std::unordered_map<int, std::vector<int>>& bookToGenreMap,
    Vector2 centerPos,
    float dt,
    Node* draggedNode)
{
    std::vector<Vector2> displacements(nodes.size(), { 0.0f, 0.0f });

    // Z�KLAD: V�echny uzly jsou aktivn� (pokud nic net�hneme)
    std::vector<bool> isActive(nodes.size(), true);

    // Pokud t�hneme uzel, omez�me fyziku jen na propojen� a bl�zk� uzly
    if (draggedNode != nullptr) {
        float activeRadius = 800.0f; // Vzd�lenost, ve kter� uzly uh�baj�

        // Nejd��v v�e usp�me
        std::fill(isActive.begin(), isActive.end(), false);

        for (size_t i = 0; i < nodes.size(); ++i) {
            // 1. Z�na kolem kurzoru (aby ciz� uzly uh�baly)
            if (Vector2Distance(nodes[i].position, draggedNode->position) < activeRadius) {
                isActive[i] = true;
            }
        }

        // 2. Tvoje pravidlo: Probuzen� podle propojen� (p�es ��nry)
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

    // 1. ODPUZOV�N�
    for (size_t i = 0; i < nodes.size(); ++i) {
        for (size_t j = i + 1; j < nodes.size(); ++j) {
            // Pokud OBA sp�, nemus�me je �e�it
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

            // Aplikujeme s�lu pouze t�m uzl�m, kter� nesp�
            if (isActive[i]) displacements[i] = Vector2Add(displacements[i], repulseVector);
            if (isActive[j]) displacements[j] = Vector2Subtract(displacements[j], repulseVector);
        }
    }

    // 2. P�ITAHOV�N� (Pru�iny)
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
                // Pokud ob� strany sp�, ne�e��me pru�inu
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

    // 3. GRAVITACE KE ST�EDU
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

            // ZM�NA 1: Aplikujeme tlumen� teplotou hned p�i v�po�tu posunu
            Vector2 actualMove = Vector2Scale(displacements[i], dt * m_Temperature);

            // ZM�NA 2: Omez�me a� tento zchlazen� pohyb (aby uzly na za��tku neulet�ly)
            float moveLength = Vector2Length(actualMove);
            if (moveLength > 30.0f) {
                actualMove = Vector2Scale(Vector2Normalize(actualMove), 30.0f);
            }

            nodes[i].position = Vector2Add(nodes[i].position, actualMove);
            totalMovement += Vector2Length(actualMove);
        }
    }

    // 5. TVRD� KOLIZE
    totalMovement += resolveNodeOverlaps(20.0f, nodes, isActive);

    m_Temperature *= 0.98f;

    // Pokud u� graf skoro vychladl, natvrdo ho zmraz�me (vr�t�me false -> fyzika se vypne)
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
            // Kolizi ne�e��me, pokud se na sebe tla�� dva sp�c� uzly
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

                // Neaktivn� uzly se chovaj�, jako by byly locked (zam�en�), tak�e aktivn� se od nich odr��
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