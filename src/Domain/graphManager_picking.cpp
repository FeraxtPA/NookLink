#include "graphManager.h"

namespace {
Vector2 ResolveLocalNonOverlappingPosition(
    Vector2 desired,
    int draggedNodeId,
    float draggedRadius,
    const std::vector<Node>& nodes) {
  constexpr float kOverlapTolerance = 1.05f;
  constexpr int kMaxIterations = 10;

  for (int iter = 0; iter < kMaxIterations; ++iter) {
    bool adjusted = false;

    for (const Node& other : nodes) {
      if (other.id == draggedNodeId || !other.visible) {
        continue;
      }

      const float minDist = (draggedRadius + other.radius) * kOverlapTolerance;
      Vector2 delta = Vector2Subtract(desired, other.position);
      float dist = Vector2Length(delta);

      if (dist >= minDist) {
        continue;
      }

      if (dist < 0.001f) {
        delta = { 1.0f, 0.0f };
        dist = 1.0f;
      }

      const Vector2 dir = Vector2Scale(delta, 1.0f / dist);
      const float push = (minDist - dist);
      desired = Vector2Add(desired, Vector2Scale(dir, push));
      adjusted = true;
    }

    if (!adjusted) {
      break;
    }
  }

  return desired;
}
}

bool GraphManager::TryGrabNodeAt(Vector2 mousePos, bool isShiftPressed) {
  for (auto &node : m_Nodes) {
    if (Vector2Distance(mousePos, node.position) <= node.radius) {
      node.isDragged = true;
      if (isShiftPressed && node.type != NodeType::Genre) {
        node.locked = true;
      }
      // Update the dragged node reference
      setDraggedNode(node.id);
      return true;
    }
  }
  return false;
}

void GraphManager::releaseDraggedNode() {
  Node *dragged = getDraggedNode();
  if (dragged) {
    dragged->isDragged = false;
    m_DraggedNodeId = -1;
  }
}

bool GraphManager::updateDraggedNodePosition(Vector2 mousePos) {
  Node *dragged = getDraggedNode();
  if (dragged) {
    dragged->position = ResolveLocalNonOverlappingPosition(
        mousePos,
        dragged->id,
        dragged->radius,
        m_Nodes);
    if (dragged->type == NodeType::Genre) {
      updateGenrePosition(dragged->id, dragged->position);
    }
    return true;
  }
  return false;
}

bool GraphManager::tryUnlockNodeAt(Vector2 mousePos) {
  Node *node = getNodeAtPosition(mousePos);
  if (node && node->locked) {
    node->locked = false;
    return true;
  }
  return false;
}

Node *GraphManager::getNodeAtPosition(Vector2 mousePos) {
  for (auto &node : m_Nodes) {
    float dist = Vector2Distance(mousePos, node.position);
    if (dist <= node.radius) {
      return &node;
    }
  }
  return nullptr;
}
