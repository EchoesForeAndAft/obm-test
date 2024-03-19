#pragma once
#include "util_shared.h"
#include "rope_physics.h"
#include "baseentity_shared.h"
#include "predictable_entity.h"
#ifdef GAME_DLL
#endif

#define CLIMBABLE_ROPE_MAX_USE_RADIUS 36.0f

#ifdef GAME_DLL
DECLARE_AUTO_LIST( IEnvRopeClimbableAutoList );
#endif

#ifdef CLIENT_DLL
#define CEnvRopeClimbable C_EnvRopeClimbable
#define CRopePhysicsData C_RopePhysicsData
#endif

class CEnvRopeClimbable;

class RopePhysicsHelper : public CSimplePhysics::IHelper
{
public:

	CEnvRopeClimbable *m_pRope = nullptr;

	void GetNodeForces( CSimplePhysics::CNode *pNodes, int iNode, Vector *pAccel ) override;
	void ApplyConstraints( CSimplePhysics::CNode *pNodes, int nNodes ) override;

};

class CEnvRopeClimbable : public CBaseEntity
#ifdef GAME_DLL
, public IEnvRopeClimbableAutoList
#endif
{
	DECLARE_CLASS( CEnvRopeClimbable, CBaseEntity );
	DECLARE_NETWORKCLASS();
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif
public:

	CEnvRopeClimbable();
	~CEnvRopeClimbable();

	void Spawn() override;
	void Think() override;

#ifdef CLIENT_DLL
	int DrawModel( int flags ) override;
#endif
	
	void InitRopePhysics();
	void SimulateRopePhysics();
	
	inline CBaseRopePhysics *RopePhysics() { return m_pPhysics; }
	inline const CBaseRopePhysics *RopePhysics() const { return m_pPhysics; }

	const CSimplePhysics::CNode *FindClosestUsableNode( const Vector &to ) /*const*/;

#ifdef GAME_DLL
	void DebugDrawRope();
	void AttachPlayer( CBasePlayer *pPlayer );
	void DetachPlayer();
#endif

	inline bool IsBeingClimbed() const { return m_pClimbingPlayer != nullptr; }
	inline float RopeLength() const { return m_RopeLength; }

private:
	CNetworkVar( float, m_RopeLength );
	CNetworkVar( string_t, m_RopeMaterialName );
	CNetworkVar( int, m_RopeSegments );
	CNetworkHandle( CBasePlayer, m_pClimbingPlayer );

	CUtlVector<CSimplePhysics::CNode> m_Nodes;
	CUtlVector<CRopeSpring> m_Springs;
	CUtlVector<float> m_SpringDistsSqr;

	CBaseRopePhysics *m_pPhysics = nullptr;
	RopePhysicsHelper m_Helper;

#ifdef CLIENT_DLL
	IMaterial *m_pRopeMaterial = nullptr;
#endif
};
