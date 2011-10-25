/////////////////////////////////////////////////////////////////////////////////
//
// Thor C++ Library
// Copyright (c) 2011 Jan Haller
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source distribution.
//
/////////////////////////////////////////////////////////////////////////////////

#include <Thor/Math/Triangulation.hpp>
#include <Thor/Vectors/VectorAlgebra2D.hpp>
#include <Thor/Config.hpp>

#include THOR_TR1_HEADER(functional)
#include <numeric>
#include <stack>


namespace thor
{
namespace detail
{

	// Type definitions
	typedef std::tr1::array<TriangleIterator, 3>			TriangleItrArray;
	typedef std::pair<unsigned int, unsigned int>			UintPair;
	typedef std::pair<TriangleIterator, TriangleIterator>	TriangleItrPair;

	// ---------------------------------------------------------------------------------------------------------------------------


	// Functor operator() definitions
	bool CompareRawVertexPtrs::operator() (const Vertex* lhs, const Vertex* rhs) const
	{
		return std::make_pair(lhs->GetPosition().x, lhs->GetPosition().y)
			<  std::make_pair(rhs->GetPosition().x, rhs->GetPosition().y);
	}

	bool CompareVertexPtrs::operator() (const AdvancedVertex* lhs, const AdvancedVertex* rhs) const
	{
		return CompareRawVertexPtrs()(&lhs->GetUserVertex(), &rhs->GetUserVertex());
	}

	bool CompareEdges::operator() (const AdvancedEdge& lhs, const AdvancedEdge& rhs) const
	{
		// Automatically compare each coordinate, delegate work to std::pair
		return std::make_pair(
			std::make_pair(lhs[0].GetPosition().x, lhs[0].GetPosition().y),
			std::make_pair(lhs[1].GetPosition().x, lhs[1].GetPosition().y))
		< std::make_pair(
			std::make_pair(rhs[0].GetPosition().x, rhs[0].GetPosition().y),
			std::make_pair(rhs[1].GetPosition().x, rhs[1].GetPosition().y));
	}

	// ---------------------------------------------------------------------------------------------------------------------------


	Circle::Circle(sf::Vector2f midPoint, float squaredRadius)
	: midPoint(midPoint)
	, squaredRadius(squaredRadius)
	{
	}

	// ---------------------------------------------------------------------------------------------------------------------------


	AdvancedVertex::AdvancedVertex(const Vertex& userVertex, TriangleIterator surroundingTriangle)
	: mUserVertex(&userVertex)
	, mSurroundingTriangle(surroundingTriangle)
	{
	}

	sf::Vector2f AdvancedVertex::GetPosition() const
	{
		return mUserVertex->GetPosition();
	}
	
	void AdvancedVertex::SetSurroundingTriangle(TriangleIterator target)
	{
		mSurroundingTriangle = target;
	}
	
	TriangleIterator AdvancedVertex::GetSurroundingTriangle() const
	{
		return mSurroundingTriangle;
	}
	
	const Vertex& AdvancedVertex::GetUserVertex() const
	{
		return *mUserVertex;
	}

	// ---------------------------------------------------------------------------------------------------------------------------


	AdvancedEdge::AdvancedEdge(const Vertex& startPoint, const Vertex& endPoint)
	: Edge<Vertex>(startPoint, endPoint)
	{
		OrderCorners();
	}

	void AdvancedEdge::OrderCorners()
	{
		if (SumVectorComponents(mCorners[0]->GetPosition()) > SumVectorComponents(mCorners[1]->GetPosition()))
		{
			std::swap(mCorners[0], mCorners[1]);
		}
	}

	// ---------------------------------------------------------------------------------------------------------------------------


	OptTriangleIterator::OptTriangleIterator()
	: valid(false)
	, target()
	{
	}
	
	OptTriangleIterator::OptTriangleIterator(TriangleIterator target)
	: valid(true)
	, target(target)
	{
	}

	OptTriangleIterator::OptTriangleIterator(const OptTriangleIterator& origin)
	: valid(origin.valid)
	, target()
	{
		// Don't copy target if invalid, since the iterator is singular (default-constructed)
		// and almost all operations on singular iterators evoke undefined behavior.
		if (valid)
			target = origin.target;
	}
	
	OptTriangleIterator& OptTriangleIterator::operator= (const OptTriangleIterator& origin)
	{
		// Don't assign target if invalid, since the iterator is singular (default-constructed)
		// and almost all operations on singular iterators evoke undefined behavior.
		valid = origin.valid;
		if (valid)
			target = origin.target;
	
		return *this;
	}

	// ---------------------------------------------------------------------------------------------------------------------------


	AdvancedTriangle::AdvancedTriangle(const Vertex& corner0, const Vertex& corner1, const Vertex& corner2)
	: Triangle<Vertex>(corner0, corner1, corner2)
	, mRemainingVertices()
	, mAdjacentTriangles()
	, mFlagged(false)
	{
	}
	
	void AdvancedTriangle::AddVertex(AdvancedVertex& vertexRef)
	{
		mRemainingVertices.insert(&vertexRef);
	}
	
	void AdvancedTriangle::RemoveVertex(AdvancedVertex& vertexRef)
	{
		unsigned int erased = mRemainingVertices.erase(&vertexRef);
		assert(erased == 1);
	}
	
	void AdvancedTriangle::RemoveVertex(VertexPtrIterator vertexItr)
	{
		unsigned int size = mRemainingVertices.size();
		mRemainingVertices.erase(vertexItr);
		assert(size == mRemainingVertices.size()+1);
	}
	
	VertexPtrIterator AdvancedTriangle::Begin()
	{
		return mRemainingVertices.begin();
	}
	
	VertexPtrIterator AdvancedTriangle::End()
	{
		return mRemainingVertices.end();
	}
	
	void AdvancedTriangle::SetAdjacentTriangle(unsigned int index, const OptTriangleIterator& adjacentTriangle)
	{
		mAdjacentTriangles[index] = adjacentTriangle;
	}
	
	OptTriangleIterator AdvancedTriangle::GetAdjacentTriangle(unsigned int index) const
	{
		return mAdjacentTriangles[index];
	}
	
	void AdvancedTriangle::Flag(bool flagged)
	{
		mFlagged = flagged;
	}
	
	bool AdvancedTriangle::IsFlagged() const
	{
		return mFlagged;
	}

	// ---------------------------------------------------------------------------------------------------------------------------


	// Returns the position of the triangle's vertex identified by index.
	sf::Vector2f At(const AdvancedTriangle& triangle, unsigned int index)
	{
		return triangle[index].GetPosition();
	}

	bool ClockwiseOrientation(sf::Vector2f v0, sf::Vector2f v1, sf::Vector2f v2)
	{
		return CrossProduct(v1 - v0, v2 - v0).z <= 0;
	}

	Circle ComputeCircumcircle(const AdvancedTriangle& triangle)
	{
		assert(At(triangle, 0) != At(triangle, 1) && At(triangle, 0) != At(triangle, 2));
		
		// Compute midpoint of two sides
		sf::Vector2f p = 0.5f * (At(triangle, 0) + At(triangle, 1));
		sf::Vector2f q = 0.5f * (At(triangle, 0) + At(triangle, 2));
		
		// Compute perpendicular bisectors of the sides
		sf::Vector2f v = PerpendicularVector(p - At(triangle, 0));
		sf::Vector2f w = PerpendicularVector(q - At(triangle, 0));
	
		// Now we have the lines p + s*v and q + t*w with s and t being real numbers. The intersection is:
		sf::Vector2f intersection(
			v.x * (p.y * w.x + q.x * w.y - q.y * w.x) - p.x * v.y * w.x,
			w.y * (p.y * v.x + q.x * v.y - p.x * v.y) - q.y * v.y * w.x);
		intersection /= v.x * w.y - v.y * w.x;

		// Alternative to calculating intersection (slower):
		//	sf::Vector3f crossP = CrossProduct(v,w);
		//	sf::Vector2f intersection = p + v * DotProduct(CrossProduct(q-p, w), crossP) / SquaredLength(crossP);

		return Circle(intersection, SquaredLength(intersection - At(triangle, 0)));
	}

	// Checks if edge intersects any constrained edge in constrainedEdges.
	bool IntersectsEdge(const AdvancedEdge& edge, const EdgeSet& constrainedEdges)
	{
		THOR_CONST_FOREACH(EdgeSet, constrainedEdges, itr)
		{
			if (Intersect(edge, *itr))
				return true;
		}
	
		return false;
	}

	// Inserts a new triangle built of the 3 corners and returns an iterator to it.
	TriangleIterator InsertTriangle(TriangleList& triangles, const Vertex& corner0, const Vertex& corner1, const Vertex& corner2)
	{
		triangles.push_back(AdvancedTriangle(corner0, corner1, corner2));
		return --triangles.end();
	}

	// Checks whether vertex is inside the triangle (center,corner1,corner2).                       c2
	// To be exact, this function only checks if vertex is beyond the two vectors                   /    
	// corner1-center, corner2-center, but since the original triangle is split                v   /
	// up into three new triangles, this doesn't matter. The example returns true.        c1-----ce
	bool IsVertexInSection(const AdvancedVertex& vertex, sf::Vector2f center,
		sf::Vector2f corner1, sf::Vector2f corner2)
	{
		assert(ClockwiseOrientation(corner1, corner2, center));

		return CrossProduct(corner1 - center, vertex.GetPosition() - center).z <  0.f
			&& CrossProduct(corner2 - center, vertex.GetPosition() - center).z >= 0.f;
	}

	// The same as above, but with only 2 sections. Returns true when the vertex
	// is located on the "left" of the vector corner2-corner1                                    v
	// The example on the right would return true.                                           c1------c2
	bool IsVertexInSection(const AdvancedVertex& vertex, sf::Vector2f corner1, sf::Vector2f corner2)
	{
		return CrossProduct(corner2 - corner1, vertex.GetPosition() - corner1).z >= 0.f;
	}

	// Moves the vertex pointed by sourceItr in sourceTriangle
	void TransferVertex(TriangleIterator sourceTriangle, VertexPtrIterator sourceItr, TriangleIterator destTriangle)
	{
		// Let the vertex point to the new triangle
		(*sourceItr)->SetSurroundingTriangle(destTriangle);
	
		// Move vertex from source to dest
		destTriangle->AddVertex(**sourceItr);
		sourceTriangle->RemoveVertex(sourceItr);
	}

	// Updates the adjacent triangle's references back to oldTriangle. They are replaced by references to newTriangle (which can be invalid).
	// @param other: The adjacent triangle of which the back-references are updated. May be invalid.
	void UpdateAdjacentBackReferences(TriangleIterator oldTriangle, OptTriangleIterator newTriangle, OptTriangleIterator other)
	{
		if (other.valid)
		{
			// Find out the index of other's adjacent that points to this triangle.
			for (unsigned int i = 0; i < 3; ++i)
			{
				OptTriangleIterator thisOne = other.target->GetAdjacentTriangle(i);
				if (thisOne.valid && thisOne.target == oldTriangle)
				{
					other.target->SetAdjacentTriangle(i, newTriangle);
					return;
				}
			}
				
			assert(false);
		}
	}

	// Sets up the adjacent triangles of each element in newTriangles according to the old triangle
	// (before the split into three new ones). Updates also the adjacents' back-references.
	void InitializeAdjacents(TriangleItrArray& newTriangles, unsigned int index, TriangleIterator oldTriangle)
	{
		unsigned int index1 = (index+1) % 3;
		unsigned int index2 = (index+2) % 3;
	
		OptTriangleIterator other = oldTriangle->GetAdjacentTriangle(index2);
	
		newTriangles[index]->SetAdjacentTriangle(0, newTriangles[index1]);
		newTriangles[index]->SetAdjacentTriangle(1, newTriangles[index2]);
		newTriangles[index]->SetAdjacentTriangle(2, other);
	
		UpdateAdjacentBackReferences(oldTriangle, newTriangles[index], other);
	}

	// Move all vertices in oldTriangle into the 3 new ones in newTriangles, according to their position.
	void TransferVertices3(TriangleIterator oldTriangleItr, TriangleItrArray& newTriangles, sf::Vector2f& newCornerPosition)
	{
		AdvancedTriangle& oldTriangle = *oldTriangleItr;

		// Determine in which sub-triangle all the remaining vertices of currentTriangle are transferred.
		// The transfer invalidates the current iterator itr, that's why it is incremented before the call.
		for (VertexPtrIterator itr = oldTriangle.Begin(); itr != oldTriangle.End(); )
		{
			if (IsVertexInSection(**itr, newCornerPosition, At(oldTriangle, 0), At(oldTriangle, 1)))
			{
				TransferVertex(oldTriangleItr, itr++, newTriangles[0]);
			}
			else if (IsVertexInSection(**itr, newCornerPosition, At(oldTriangle, 1), At(oldTriangle, 2)))
			{
				TransferVertex(oldTriangleItr, itr++, newTriangles[1]);
			}
			else
			{
				assert(IsVertexInSection(**itr, newCornerPosition,  At(oldTriangle, 2),  At(oldTriangle, 0)));
				TransferVertex(oldTriangleItr, itr++, newTriangles[2]);
			}
		}
	}

	// Of two given adjacent triangles, this function finds out which two corners are shared by both triangles, and which two are
	// contained by only one triangle.
	//
	// The output parameter sharedCornerIndices1 contains the subscripts (referring to the triangles) of the first shared corner,
	// sharedCornerIndices2 to the second shared corner, disjointCornerIndices to the corners that aren't part of the common edge.
	// The member first refers for each pair to the first triangle, second to the second one.
	void ArrangeCorners(const AdvancedTriangle& first, const AdvancedTriangle& second, 
		UintPair& sharedCornerIndices1, UintPair& sharedCornerIndices2, UintPair& disjointCornerIndices)
	{
		std::tr1::array<bool, 3> match;
	
		// The triangle's corners are numbered in clockwise order. For example, to compare ABC and BAD,          A
		// we need to reverse BAD to DAB. Here, the AB subsequences are equal in ABC and DAB.                 C  |  D
		// The variable j determines by how many elements the first sequence is rotated.                         B
		for (unsigned int j = 0; j < 3; ++j)
		{
			// Rotate vertex sequence in first until two of them are equal to second
			for (unsigned int i = 0; i < 3; ++i)
			{
				// j determines rotation, 2-i is the reversed second sequence
				match[i] = (At(first, (j+i) % 3) == At(second, 2-i));
			}
		
			// If 2 of 3 corners are equal, then we know the corner arrangement.
			if (std::accumulate(match.begin(), match.end(), 0) == 2)
			{
				unsigned int nbSharedCorners = 0;

				// Fill output parameters with the correct vertices (according to the function's interface description)
				for (unsigned int i = 0; i < 3; ++i)
				{
					unsigned int firstIndex = (j+i) % 3;
					unsigned int secondIndex = 2-i;
				
					// A corner that both adjacent triangles (first and second) have in common
					if (match[i])
					{
						assert(&first[firstIndex] == &second[secondIndex]);
					
						if (nbSharedCorners++ == 0)
						{
							sharedCornerIndices1.first  = firstIndex;
							sharedCornerIndices1.second = secondIndex;
						}
						else
						{
							sharedCornerIndices2.first  = firstIndex;
							sharedCornerIndices2.second = secondIndex;
						}
					}
				
					// A disjoint corner (contained in either first or second)
					else
					{
						assert(&first[firstIndex] != &second[secondIndex]);
					
						disjointCornerIndices.first  = firstIndex;
						disjointCornerIndices.second = secondIndex;
					}
				}
			
				// Ensure that the indices are clockwise oriented for both triangles.                       (sc1.f)|(sc1.s)
				// first:  sc1 -> sc2 -> dc                                               first ->   (dc.f)        |        (dc.s)   <- second
				// second: sc2 -> sc1 -> dc                                                                 (sc2.f)|(sc2.s)
				if (!ClockwiseOrientation(
					At(first, sharedCornerIndices1.first), 
					At(first, sharedCornerIndices2.first),
					At(first, disjointCornerIndices.first)))
				{
					std::swap(sharedCornerIndices1, sharedCornerIndices2);
				}
				
				return;
			}
		}
	
		// We get here when the triangles are not adjacent (and thus don't share two corners); this should not happen.
		assert(false);
	}

	// Helper function for TransferVertices2()
	void TransferVertices2Impl(TriangleIterator oldFirst, TriangleIterator oldSecond, TriangleIterator newFirst, TriangleIterator newSecond,
		const UintPair& disjointCornerIndices, TriangleIterator oldTriangle)
	{
		// Find out on which side of the new edge each vertex is located and push it into the appropriate new triangle.
		for (VertexPtrIterator itr = oldTriangle->Begin(); itr != oldTriangle->End(); )
		{
			if (IsVertexInSection(**itr, At(*oldFirst, disjointCornerIndices.first), At(*oldSecond, disjointCornerIndices.second)))
			{
				TransferVertex(oldTriangle, itr++, newFirst);
			}
			else
			{
				assert(IsVertexInSection(**itr, At(*oldSecond, disjointCornerIndices.second), At(*oldFirst, disjointCornerIndices.first)));
				TransferVertex(oldTriangle, itr++, newSecond);
			}
		}	
	}

	// Moves all vertices in oldTriangle to either newFirst or newSecond, depending on which side of the new
	// edge they are situated.
	void TransferVertices2(TriangleIterator oldFirst, TriangleIterator oldSecond, TriangleIterator newFirst, TriangleIterator newSecond,
		const UintPair& disjointCornerIndices)
	{
		TransferVertices2Impl(oldFirst, oldSecond, newFirst, newSecond, disjointCornerIndices, oldFirst);
		TransferVertices2Impl(oldFirst, oldSecond, newFirst, newSecond, disjointCornerIndices, oldSecond);	
	}

	// Copies the adjacent triangle from oldTriangle at oldIndex to newTriangle at newIndex (only one reference).
	// Additionally, the referencee is updated so that it points to newTriangle instead of oldTriangle.
	void UpdateAdjacentRelation(TriangleIterator oldTriangle, unsigned int oldIndex, TriangleIterator newTriangle, unsigned int newIndex)
	{
		OptTriangleIterator other = oldTriangle->GetAdjacentTriangle(oldIndex);

		// Update this triangle's references to adjacent triangles.
		newTriangle->SetAdjacentTriangle(newIndex, other);
	
		// Update adjacent triangles's references to this triangle.
		UpdateAdjacentBackReferences(oldTriangle, newTriangle, other);	
	}

	// Performs an edge flip, i.e. both triangles are merged and the resulting quadrilateral is split again, but into two different 
	// triangles  (choose the other diagonal as cutting edge).
	// @param oldFirst				Iterator to the old first triangle. This iterator is invalidated during the operation.
	// @param oldSecond				Like oldFirst, but iterator to the second triangle.
	// @param sharedCornerIndices1	Indices of the corners in oldFirst and oldSecond that refer to the shared corners.
	// @param sharedCornerIndices2	Like 1, but the other shared corner.
	// @param disjointCornerIndices Indices of the non-shared, disjoint corners.
	// @return						Pair of the newly created triangles
	TriangleItrPair FlipEdges(TriangleList& triangles, TriangleIterator oldFirst, TriangleIterator oldSecond,
		const UintPair& sharedCornerIndices1, const UintPair& sharedCornerIndices2, const UintPair& disjointCornerIndices)
	{
		// Create the new triangles which are going to outlive this function
		TriangleIterator newFirst  = InsertTriangle(triangles, 
			(*oldFirst)[sharedCornerIndices1.first],			// (sc1)
			(*oldSecond)[disjointCornerIndices.second],			// (dc.s)
			(*oldFirst)[disjointCornerIndices.first]);			// (dc.f)
	
		TriangleIterator newSecond = InsertTriangle(triangles,
			(*oldSecond)[sharedCornerIndices2.second],			// (sc2)
			(*oldFirst)[disjointCornerIndices.first],			// (dc.f)
			(*oldSecond)[disjointCornerIndices.second]);		// (dc.s)
		
		// Move each vertex to the new corresponding triangle
		TransferVertices2(oldFirst, oldSecond, newFirst, newSecond, disjointCornerIndices);
	
		// Adapt referenced adjacents - note that the old indices                     //                       (dc.f)              edge      
		// (non-shared corners) now form the end points of the new edge.              // oldFirst    (sc2.f)          (sc1.f)      flip            (1)|(2)
		UpdateAdjacentRelation(oldFirst, sharedCornerIndices1.first, newSecond, 2);	  //             ------------------------       ->       (0)      |      (0)
		UpdateAdjacentRelation(oldFirst, sharedCornerIndices2.first, newFirst, 1);    // oldSecond   (sc2.s)          (sc1.s)                      (2)|(1)     
		UpdateAdjacentRelation(oldSecond, sharedCornerIndices1.second, newSecond, 1); //                       (dc.s)
		UpdateAdjacentRelation(oldSecond, sharedCornerIndices2.second, newFirst, 2);  //                                                     newSecond|newFirst
	
		// Of course, the new triangles are adjacent to each other.
		newFirst->SetAdjacentTriangle(0, newSecond);
		newSecond->SetAdjacentTriangle(0, newFirst);
		
		// Mark old triangles for removal; they're not needed anymore.
		oldFirst->Flag();
		oldSecond->Flag();
	
		return TriangleItrPair(newFirst, newSecond);
	}

	// Returns true when the specified container holds the specified vertex.
	bool ContainsVertex(const VertexCtr& container, const Vertex& vertex)
	{
		// Note: Can't use std::find (comparing addresses, not values). Building a functor for std::find_if() is overkill here.
		THOR_CONST_FOREACH(VertexCtr, container, itr)
		{
			if (&*itr == &vertex)
				return true;
		}
	
		return false;
	}

	// Returns true if any of the boundary (dummy) vertices is part of first/second's shared edge.
	bool SharedBoundary(const VertexCtr& boundaryVertices, const AdvancedTriangle& first,
		const UintPair& sharedCornerIndices1, const UintPair& sharedCornerIndices2)
	{
		return ContainsVertex(boundaryVertices, first[sharedCornerIndices1.first])
			|| ContainsVertex(boundaryVertices, first[sharedCornerIndices2.first]);
	}

	// Returns true if any of the boundary (dummy) vertices is either first's or second's disjoint corner.
	bool DisjointBoundary(const VertexCtr& boundaryVertices, const AdvancedTriangle& first, const AdvancedTriangle& second,
		const UintPair& disjointCornerIndices)
	{
		return ContainsVertex(boundaryVertices, first[disjointCornerIndices.first])
			|| ContainsVertex(boundaryVertices, second[disjointCornerIndices.second]);
	}

	bool EnsureLocalDelaunay(TriangleList& triangles, TriangleIterator first, TriangleIterator second, const VertexCtr& boundaryVertices,
		const EdgeSet& constrainedEdges);

	// Applies the check for the Delaunay condition recursively to the triangles neighbors.
	// @return true if an edge flip is performed, false otherwise.
	bool EnsureLocalDelaunayAdjacent(TriangleList& triangles, TriangleIterator triangleItr, unsigned int adjacentIndex, const VertexCtr& boundaryVertices,
		const EdgeSet& constrainedEdges)
	{
		OptTriangleIterator itr = triangleItr->GetAdjacentTriangle(adjacentIndex);
	
		return itr.valid && EnsureLocalDelaunay(triangles, triangleItr,  itr.target, boundaryVertices, constrainedEdges);
	}

	// Flips edges and enforces the Delaunay condition at adjacent triangles
	void ChangeEdgeSituation(TriangleList& triangles, TriangleIterator first, TriangleIterator second, const VertexCtr& boundaryVertices,
		const EdgeSet& constrainedEdges, const UintPair& sharedCornerIndices1, const UintPair& sharedCornerIndices2, const UintPair& disjointCornerIndices)
	{
		TriangleItrPair newTriangles = FlipEdges(triangles, first, second, sharedCornerIndices1, sharedCornerIndices2, disjointCornerIndices);	
	
		// Ensure that the adjacent triangles locally conform to Delaunay, as well.
		// If one function call returns true, don't execute the others because iterators are invalidated.
		// On average, the recursion stops on O(1) time, the adjacent triangles are already Delaunay-conforming.
		EnsureLocalDelaunayAdjacent(triangles, newTriangles.first, 1, boundaryVertices, constrainedEdges);
		EnsureLocalDelaunayAdjacent(triangles, newTriangles.first, 2, boundaryVertices, constrainedEdges);
		EnsureLocalDelaunayAdjacent(triangles, newTriangles.second, 1, boundaryVertices, constrainedEdges);
		EnsureLocalDelaunayAdjacent(triangles, newTriangles.second, 2, boundaryVertices, constrainedEdges);		
	}

	// Checks whether the shared edge of two triangles must be moved (to the other diagonal of the quadrilateral)
	// and performs the necessary actions in this case, so that the triangulation locally conforms Delaunay.
	// Also, the adjacent triangles are checked and edge-flipped, if necessary. On average, this requires constant
	// time because the surrounding triangles already satisfy Delaunay before the flip of this triangle.
	// If an edge flip is performed, true is returned and the new triangles are stored in the input/output parameters first, second.
	// @return true if an edge flip is performed, false otherwise.
	bool EnsureLocalDelaunay(TriangleList& triangles, TriangleIterator first, TriangleIterator second, const VertexCtr& boundaryVertices,
		const EdgeSet& constrainedEdges)
	{
		// Note: If the merged quadrilateral is concave, the Delaunay condition will locally already be satisfied.	

		// Flagged triangles are going to be removed, don't take them into account
		if (first->IsFlagged() || second->IsFlagged())
			return false;

		// Find out which triangle indices refer to the corners that are shared by both triangles, and which to the disjoint ones.
		UintPair sharedCornerIndices1;
		UintPair sharedCornerIndices2;
		UintPair disjointCornerIndices;
		ArrangeCorners(*first, *second, sharedCornerIndices1, sharedCornerIndices2, disjointCornerIndices);
	
		// Check if we must flip edges because of the boundaries (the triangles there don't have to conform Delaunay, but the triangles inside do)
		bool disjointBoundary = DisjointBoundary(boundaryVertices, *first, *second, disjointCornerIndices);
		bool sharedBoundary = SharedBoundary(boundaryVertices, *first, sharedCornerIndices1, sharedCornerIndices2);
	
		// The following additional checks are not required if constrained edges are always part of a merged quadrilateral (=two adjacent triangles).
		// But in general, we may have constrained edges that span many triangles, and the local Delaunay condition doesn't capture them.
		bool sharedBlocking = IntersectsEdge(AdvancedEdge((*first)[sharedCornerIndices1.first], (*first)[sharedCornerIndices2.first]), constrainedEdges);
		bool disjointBlocking = IntersectsEdge(AdvancedEdge((*first)[disjointCornerIndices.first], (*second)[disjointCornerIndices.second]), constrainedEdges);
	
		// These two bools express whether the disjoint edge respectively the shared edge MUST be flipped.
		bool disjointEdgeEnforced = disjointBoundary || disjointBlocking;
		bool sharedEdgeEnforced = sharedBoundary || sharedBlocking;
	
		// If the Delaunay test concerns one of the initial vertices, we pretend that those vertices are never inside the circumcircle.
		// This is required because we don't want to perform edge flips at the boundary of the very big outer triangle.
		// The same applies to constrained edges as input of the Constrained Delaunay Triangulation.
		if (disjointEdgeEnforced && !sharedEdgeEnforced)
			return false;
	
		if (sharedEdgeEnforced && !disjointEdgeEnforced)
		 {
			// If the merged quadrilateral isn't convex, we may of course not flip edges (since the new edge would be located outside both triangles).
			if (ClockwiseOrientation(At(*first, disjointCornerIndices.first), At(*second, disjointCornerIndices.second), At(*first, sharedCornerIndices1.first))
			 || ClockwiseOrientation(At(*second, disjointCornerIndices.second), At(*first, disjointCornerIndices.first), At(*first, sharedCornerIndices2.first)))
				return false;
	 
			ChangeEdgeSituation(triangles, first, second, boundaryVertices, constrainedEdges, sharedCornerIndices1, sharedCornerIndices2, disjointCornerIndices);
			return true;
		 }
	
		// If the vertex of the other triangle is inside this triangle's circumcircle, the Delaunay condition is locally breached and we need to flip edges.
		// Independently, there can be an enforced edge flip (at the boundary, or because of the constraints).
		// The second && operand is actually not necessary, since the Delaunay condition is symmetric to both triangles. However, rounding errors may occur
		// at close points.
		Circle circle = ComputeCircumcircle(*first);
		Circle circle2 = ComputeCircumcircle(*second);
		if (SquaredLength(At(*second, disjointCornerIndices.second) - circle.midPoint) < circle.squaredRadius
		 && SquaredLength(At(*first, disjointCornerIndices.first) - circle2.midPoint) < circle2.squaredRadius)
		{
			ChangeEdgeSituation(triangles, first, second, boundaryVertices, constrainedEdges, sharedCornerIndices1, sharedCornerIndices2, disjointCornerIndices);
			return true;
		}
	
		// Otherwise, the triangles are Delaunay at the moment and no edge flip is required.
		return false;
	}

	// Returns vector.x + vector.y (required for vertex ordering)
	float SumVectorComponents(sf::Vector2f vector)
	{
		return vector.x + vector.y;
	}

	// Inserts the specified vertex into the list of triangles.
	void InsertPoint(TriangleList& triangles, AdvancedVertex& vertex, const VertexCtr& boundaryVertices, const EdgeSet& constrainedEdges)
	{
		TriangleIterator oldTriangleItr = vertex.GetSurroundingTriangle();
		AdvancedTriangle& oldTriangle = *oldTriangleItr;
	
		assert(ClockwiseOrientation(At(oldTriangle, 0), At(oldTriangle, 1), At(oldTriangle,2)));
	
		// Split triangle up into three new sub-triangles, each consisting of two old corners and vertex
		TriangleItrArray newTriangles = {
			InsertTriangle(triangles, oldTriangle[0], oldTriangle[1], vertex.GetUserVertex()),
			InsertTriangle(triangles, oldTriangle[1], oldTriangle[2], vertex.GetUserVertex()),
			InsertTriangle(triangles, oldTriangle[2], oldTriangle[0], vertex.GetUserVertex())};

		// Assign the adjacent triangles to the new triangles
		for (unsigned int i = 0; i < 3; ++i)
			InitializeAdjacents(newTriangles, i, oldTriangleItr);

		// Remove current vertex - as soon as it forms a triangle corner, it counts no longer as remaining vertex
		sf::Vector2f newCornerPosition = vertex.GetPosition();
		oldTriangle.RemoveVertex(vertex);
	
		// Move each vertex to its corresponding new surrounding triangle
		TransferVertices3(oldTriangleItr, newTriangles, newCornerPosition);
	
		// Remove the old big triangle, we have three new ones
		triangles.erase(oldTriangleItr);
	
		// For each newly created triangle, we must ensure that the Delaunay condition with its adjacent is kept up.
		// The variable adjacent (third argument of EnsureLocalDelaunay()) is the adjacent of the old triangle.
		// Corner number 2 is always the vertex inserted in this function, so the triangle on the opposite is the sought one.
		for (unsigned int i = 0; i < 3; ++i)
		{
			OptTriangleIterator adjacent = newTriangles[i]->GetAdjacentTriangle(2);
		
			if (adjacent.valid)
				EnsureLocalDelaunay(triangles, newTriangles[i], adjacent.target, boundaryVertices, constrainedEdges);
		}
	
		// Remove currently flagged triangles. Don't do this earlier because of iterator invalidations.
		triangles.remove_if(std::tr1::mem_fn(&AdvancedTriangle::IsFlagged));
	}

	// Creates 3 "dummy" vertices that form a huge triangle which later includes all other vertices
	// (every element of allVertices).
	void CreateBoundaryPoints(AdvancedVertexCtr& allVertices, VertexCtr& boundaryVertices, TriangleList& triangles)
	{
		// boundaryVertices contains three Vertex objects. Choose arbitrary, clockwise oriented positions.
		boundaryVertices.push_back(Vertex(-1, 0));
		boundaryVertices.push_back(Vertex( 0,+1));
		boundaryVertices.push_back(Vertex(+1, 0));

		// allVertices initially contains three AdvancedVertex objects (with meta-information for the algorithm)
		allVertices.push_back(AdvancedVertex(boundaryVertices[0], TriangleIterator()));
		allVertices.push_back(AdvancedVertex(boundaryVertices[1], TriangleIterator()));	
		allVertices.push_back(AdvancedVertex(boundaryVertices[2], TriangleIterator()));

		triangles.push_back(AdvancedTriangle(boundaryVertices[0], boundaryVertices[1], boundaryVertices[2]));
	
		for (unsigned int i = 0; i < 3; ++i)
			allVertices[i].SetSurroundingTriangle(triangles.begin());
	}

	// Sets the initial point positions so that the triangle of dummy vertices includes all other vertices.
	// Like this, we can start the algorithm seamlessly.
	void SetBoundaryPositions(const AdvancedVertexCtr& allVertices, VertexCtr& boundaryVertices)
	{
		// Find maximal coordinate in any direction
		float maxCoord = 0.f;
		THOR_CONST_FOREACH(AdvancedVertexCtr, allVertices, itr)
		{
			maxCoord = std::max(maxCoord, std::abs(itr->GetPosition().x));
			maxCoord = std::max(maxCoord, std::abs(itr->GetPosition().y));
		}

		// Reduce probability to have a 3 collinear points (which can't be triangulated) by slightly moving boundary points. Take arbitrary value...
		float epsilon = 0.000372f;

		// Overwrite 3 dummy vertices so that the resulting triangle certainly surrounds all other vertices.
		maxCoord *= 4.f;
		boundaryVertices[0] = Vertex(epsilon, maxCoord-epsilon);
		boundaryVertices[1] = Vertex(maxCoord+epsilon, -epsilon);
		boundaryVertices[2] = Vertex(-maxCoord-epsilon, -maxCoord+epsilon);
	}

	bool HasCorner(const AdvancedTriangle& triangle, const Vertex& corner)
	{
		return &triangle[0] == &corner
			|| &triangle[1] == &corner
			|| &triangle[2] == &corner;
	}

	bool Has1Of3Corners(const AdvancedTriangle& triangle, const VertexCtr& corners)
	{
		return HasCorner(triangle, corners[0])
			|| HasCorner(triangle, corners[1])
			|| HasCorner(triangle, corners[2]);
	}

	// Checks whether the edge formed of the two specified points is contained in constrainedEdges.
	bool IsEdgeConstrained(const EdgeSet& constrainedEdges, const Vertex& startPoint, const Vertex& endPoint)
	{
		AdvancedEdge adv(startPoint, endPoint);
		EdgeSet::const_iterator candidate = constrainedEdges.find(adv);
	
		// Just to make sure the set predicate really works. Otherwise: return candidate != ..end() && .. && ..;
		assert(candidate == constrainedEdges.end()
		 || adv[0].GetPosition() == (*candidate)[0].GetPosition()
		 && adv[1].GetPosition() == (*candidate)[1].GetPosition());
	
		return candidate != constrainedEdges.end();
	}

	// Checks if the adjacent triangle is in use (i.e. inside the polygon) and returns a valid OptTriangleIterator in this case. Otherwise, the
	// latter is invalid.
	OptTriangleIterator HasUnusedAdjacent(const AdvancedTriangle& triangle, unsigned int index, const EdgeSet& constrainedEdges)
	{
		if (IsEdgeConstrained(constrainedEdges, triangle[(index+1) % 3], triangle[(index+2) % 3]))
			return OptTriangleIterator();
		else
			return triangle.GetAdjacentTriangle(index);
	}

	// Iterative implementation of RemoveUnusedTriangles
	void RemoveOuterPolygonTrianglesImpl(TriangleIterator current, std::stack<TriangleIterator>& stack, const EdgeSet& constrainedEdges)
	{
		// Flagged triangles have already been passed, skip them
		if (current->IsFlagged())
			return;
		
		current->Flag();

		OptTriangleIterator adjacent;
		for (unsigned int i = 0; i < 3; ++i)
		{
			if ((adjacent = HasUnusedAdjacent(*current, i, constrainedEdges)).valid)
				stack.push(adjacent.target);
		}
	}

	// Removes the triangles that are "unused" (i.e. outside the polygon).
	// The parameter current refers to any triangle touching at least one boundary point. Starting at this triangle, adjacent triangles
	// are walked through recursively. When we reach an edge of the polygon (which is always a constrained edge, and vice versa),
	// we stop here and complete the remaining directions. Finally, all triangles that are outside of the polygon bounds are removed.
	void RemoveOuterPolygonTriangles(TriangleList& triangles, TriangleIterator current, const EdgeSet& constrainedEdges)
	{
		std::stack<TriangleIterator> stack;
		stack.push(current);

		while (!stack.empty())
		{
			// Get the top element of the stack (next triangle to traverse) and pop it
			current = stack.top();
			stack.pop();
		
			// Call function that may push new triangles onto the stack
			RemoveOuterPolygonTrianglesImpl(current, stack, constrainedEdges);
		}
	
		// Remove all triangles marked as unused.
		// We can't erase() during the stack iteration, because triangles' flags are still polled, so the iterators must be valid.
		triangles.remove_if(std::tr1::mem_fn(&AdvancedTriangle::IsFlagged));
	}

} // namespace detail
} // namespace thor
