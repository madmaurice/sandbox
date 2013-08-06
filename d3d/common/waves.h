//***************************************************************************************
// Performs the calculations for the wave simulation.  After the simulation has been
// updated, the client must copy the current solution into vertex buffers for rendering.
// This class only does the calculations, it does not do any drawing.
//***************************************************************************************

#ifndef _INCGUARD_WAVES_H
#define _INCGUARD_WAVES_H

#include <Windows.h>
#include <xnamath.h>
#include "types.h"

class Waves
{
public:
	Waves();
	~Waves();

	uint32 RowCount() const;
	uint32 ColumnCount() const;
	uint32 VertexCount() const;
	uint32 TriangleCount() const;
    float Width() const;
	float Depth() const;

	// Returns the solution at the ith grid point.
	const XMFLOAT3& operator[](int i) const { return m_currSolution[i]; }

    // Returns the solution normal at the ith grid point.
	const XMFLOAT3& Normal(int i)const { return m_normals[i]; }

    // Returns the unit tangent vector at the ith grid point in the local x-axis direction.
	const XMFLOAT3& TangentX(int i)const { return m_tangentX[i]; }

	void Init(uint32 m, uint32 n, float dx, float dt, float speed, float damping);
	void Update(float dt);
	void Disturb(uint32 i, uint32 j, float magnitude);

private:
	uint32 m_numRows;
	uint32 m_numCols;

	uint32 m_vertexCount;
	uint32 m_triangleCount;

	// Simulation constants we can precompute.
	float m_k1;
	float m_k2;
	float m_k3;

	float m_timeStep;
	float m_spatialStep;

	XMFLOAT3* m_prevSolution;
	XMFLOAT3* m_currSolution;
    XMFLOAT3* m_normals;
	XMFLOAT3* m_tangentX;
};

#endif // WAVES_H