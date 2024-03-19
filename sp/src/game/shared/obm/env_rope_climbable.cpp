#include "cbase.h"
#include "env_rope_climbable.h"
#include "debugoverlay_shared.h"
#include "rope_shared.h"
#include "vphysics/constraints.h"
#ifdef CLIENT_DLL
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imesh.h"
#include "tier2/beamsegdraw.h"
#endif

#ifdef GAME_DLL
IMPLEMENT_AUTO_LIST( IEnvRopeClimbableAutoList );

LINK_ENTITY_TO_CLASS( env_rope_climbable, CEnvRopeClimbable );

BEGIN_DATADESC( CEnvRopeClimbable )
	DEFINE_KEYFIELD( m_RopeLength, FIELD_FLOAT, "RopeLength" ),
	DEFINE_KEYFIELD( m_RopeMaterialName, FIELD_STRING, "RopeMaterial" ),
	DEFINE_KEYFIELD( m_RopeSegments, FIELD_INTEGER, "RopeSegments" )
END_DATADESC()
#endif

#ifdef CLIENT_DLL
#undef CEnvRopeClimbable
IMPLEMENT_CLIENTCLASS_DT( C_EnvRopeClimbable, DT_EnvRopeClimbable, CEnvRopeClimbable )
	RecvPropFloat( RECVINFO( m_RopeLength ) ),
	RecvPropString( RECVINFO( m_RopeMaterialName ) ),
	RecvPropInt( RECVINFO( m_RopeSegments ) ),
END_RECV_TABLE()
#define CEnvRopeClimbable C_EnvRopeClimbable
#else
IMPLEMENT_SERVERCLASS_ST( CEnvRopeClimbable, DT_EnvRopeClimbable )
	SendPropFloat( SENDINFO( m_RopeLength ) ),
	SendPropStringT( SENDINFO( m_RopeMaterialName ) ),
	SendPropInt( SENDINFO( m_RopeSegments ) ),
END_SEND_TABLE()
#endif

void RopePhysicsHelper::GetNodeForces( CSimplePhysics::CNode *pNodes, int iNode, Vector *pAccel )
{
	*pAccel = Vector( ROPE_GRAVITY );
}

void RopePhysicsHelper::ApplyConstraints( CSimplePhysics::CNode *pNodes, int nNodes )
{
	pNodes[0].m_vPos = m_pRope->GetAbsOrigin();
}

CEnvRopeClimbable::CEnvRopeClimbable()
{
	m_RopeLength = 1024.0f;
	m_RopeMaterialName = MAKE_STRING( "cable/cable.vmt" );
	m_RopeSegments = 8;
	m_Helper.m_pRope = this;
}

CEnvRopeClimbable::~CEnvRopeClimbable()
{
	if ( m_pPhysics )
	{
		delete m_pPhysics;
	}
}

void CEnvRopeClimbable::Spawn()
{
	BaseClass::Spawn();

#ifdef CLIENT_DLL
	m_pRopeMaterial = materials->FindMaterial( m_RopeMaterialName, TEXTURE_GROUP_OTHER );
#endif

	InitRopePhysics();
	
	SetNextThink( gpGlobals->curtime );
}

#ifdef GAME_DLL
void CEnvRopeClimbable::DebugDrawRope()
{
	CSimplePhysics::CNode *pFirst = m_pPhysics->GetFirstNode();
	const Vector vecOrigin = GetAbsOrigin();
	const Vector vecPlayerCenter = IsBeingClimbed() ? m_pClimbingPlayer->WorldSpaceCenter() : vec3_origin;

	if ( IsBeingClimbed() )
	{
		NDebugOverlay::Line( vecOrigin, vecPlayerCenter, 255, 0, 0, false, 0.0f );
	}

	CSimplePhysics::CNode *pPrev = pFirst;
	for ( int i = 1; i < m_pPhysics->NumNodes(); i++ )
	{
		CSimplePhysics::CNode *pNode = m_pPhysics->GetNode( i );

		if ( IsBeingClimbed() )
		{
			if ( pNode->m_vPos.DistToSqr( vecOrigin ) <= vecPlayerCenter.DistToSqr( vecOrigin ) )
			{
				continue;
			}
		}

		NDebugOverlay::Line( pPrev->m_vPos, pNode->m_vPos, 255, 0, 0, false, 0.0f );

		if ( !IsBeingClimbed() )
		{
			NDebugOverlay::Sphere( pNode->m_vPos, CLIMBABLE_ROPE_MAX_USE_RADIUS, 255, 0, 0, false, 0.0f );
		}

		pPrev = pNode;
	}
}
#endif

void CEnvRopeClimbable::Think()
{
	SimulateRopePhysics();
#ifdef GAME_DLL
	DebugDrawRope();
#endif
	SetNextThink( gpGlobals->curtime );
}

void CEnvRopeClimbable::InitRopePhysics()
{
	m_Nodes.SetCount( m_RopeSegments );
	m_Springs.SetCount( m_RopeSegments - 1 );
	m_SpringDistsSqr.SetCount( m_RopeSegments - 1 );

	Vector vecOrigin = GetAbsOrigin();

	for ( auto &node : m_Nodes )
	{
		node.Init( vecOrigin );
	}
	
	const float flSlack = 32.0f;
	
	// Deferred initialisation of RopePhysicsHelper.
	m_pPhysics = new CBaseRopePhysics( m_Nodes.Base(), m_RopeSegments, m_Springs.Base(), m_SpringDistsSqr.Base() );
	m_pPhysics->SetupSimulation( 0.0f, &m_Helper );
	m_pPhysics->ResetSpringLength( ( m_RopeLength + flSlack + ROPESLACK_FUDGEFACTOR ) / m_RopeSegments - 1 );

	m_pPhysics->Restart();
	m_pPhysics->Simulate( 3.0f );
}

void CEnvRopeClimbable::SimulateRopePhysics()
{
	m_pPhysics->Simulate( gpGlobals->frametime );
}

const CSimplePhysics::CNode *CEnvRopeClimbable::FindClosestUsableNode( const Vector &to ) /*const*/
{
	float flSmallestDistance = FLT_MAX;
	CSimplePhysics::CNode *pResult = nullptr;
	
	for ( int i = 0; i < m_pPhysics->NumNodes(); i++ )
	{
		CSimplePhysics::CNode *pNode = m_pPhysics->GetNode( i );
		if ( pNode->m_vPos.DistToSqr( to ) < flSmallestDistance )
		{
			pResult = pNode;
		}
	}

	// Just get the first usable node.
	if ( pResult && pResult->m_vPos.DistTo( to ) <= CLIMBABLE_ROPE_MAX_USE_RADIUS )
	{
		return pResult;
	}
	else
	{
		return nullptr;
	}
}

#ifdef GAME_DLL
void CEnvRopeClimbable::AttachPlayer( CBasePlayer *pPlayer )
{
	if ( !m_pClimbingPlayer )
	{
		DevMsg( "Player is climbing rope!\n" );
		m_pClimbingPlayer = pPlayer;
		m_pClimbingPlayer->SetMoveType( MOVETYPE_FLYGRAVITY );
	}
}

void CEnvRopeClimbable::DetachPlayer()
{
	if ( m_pClimbingPlayer )
	{
		DevMsg( "Player has detached from rope!\n" );
		m_pClimbingPlayer->SetMoveType( MOVETYPE_WALK );
		m_pClimbingPlayer = nullptr;
	}
}
#endif

#ifdef CLIENT_DLL
static void DrawRope( IMaterial *pMaterial, CUtlVector<BeamSeg_t> &vRopeSegments )
{
	CMatRenderContextPtr pRenderContext( materials );
	
	const int nSegmentCount = vRopeSegments.Count();
	const int nVertCount = nSegmentCount * 2;
	const int nIndexCount = ( nSegmentCount - 1 )  * 6;

	// Render the solid portion of the ropes.
	CMeshBuilder meshBuilder;
	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, nullptr, nullptr, pMaterial );
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, nVertCount, nIndexCount );

	CBeamSegDraw beamDraw;
	beamDraw.Start( pRenderContext, nSegmentCount, pMaterial, &meshBuilder );

	for ( BeamSeg_t &segment : vRopeSegments )
	{
		beamDraw.NextSeg( &segment );
	}

	beamDraw.End();

	meshBuilder.End();
	pMesh->Draw();
}

int CEnvRopeClimbable::DrawModel( int flags )
{
	return 0;
	
	CUtlVector<BeamSeg_t> segments;
	
	const Vector vecOrigin = GetAbsOrigin();
	const Vector vecPlayerCenter = IsBeingClimbed() ? m_pClimbingPlayer->WorldSpaceCenter() : vec3_origin;

	BeamSeg_t seg;
	seg.m_vColor.Init( 1.0f, 1.0f, 1.0f );
	seg.m_flAlpha = 1.0f;
	seg.m_flWidth = 1.0f;
	seg.m_flTexCoord = 0.0f;
	
	if ( IsBeingClimbed() )
	{
		seg.m_vPos = vecOrigin;
		segments.AddToTail( seg );
		
		seg.m_vPos = m_pClimbingPlayer->WorldSpaceCenter();
		segments.AddToTail( seg );
	}
	
	for ( CSimplePhysics::CNode &node : m_Nodes )
	{
		if ( IsBeingClimbed() )
		{
			if ( node.m_vPos.DistToSqr( vecOrigin ) <= vecPlayerCenter.DistToSqr( vecOrigin ) )
			{
				continue;
			}
		}
		
		seg.m_vPos = node.m_vPos;
		segments.AddToTail( seg );
	}
	
	DrawRope( m_pRopeMaterial, segments );

	return 0;
}
#endif
