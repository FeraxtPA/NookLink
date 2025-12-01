#pragma once
#include "nodeRenderer.h"
#include <raymath.h>
#include <algorithm> 
class GraphLayout
{
public:
	GraphLayout() = default;

	//Right now it checks every node against every other node - could be optimized later somehow
	void  resolveNodeOverlaps(float padding, std::vector<Node>& m_Nodes);

	void applySpringConstraints(
		std::vector<Node>& nodes,
		const std::unordered_map<int, std::vector<int>>& bookToGenreMap,
		const std::unordered_map<int, int>& genreToBookCount,
		float baseSpringLength,
		float springConstant);
		
private:

	
};