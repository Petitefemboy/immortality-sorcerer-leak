#pragma once

#include "../Valve/vector.hpp"
#include "../Valve/Matrix.hpp"
#include "../Valve/qangle.hpp"

class Quaternion {
public:
   float x, y, z, w;
};

typedef float RadianEuler[3];

#define MAX_QPATH  260

#define BONE_CALCULATE_MASK             0x1F
#define BONE_PHYSICALLY_SIMULATED       0x01    // bone is physically simulated when physics are active
#define BONE_PHYSICS_PROCEDURAL         0x02    // procedural when physics is active
#define BONE_ALWAYS_PROCEDURAL          0x04    // bone is always procedurally animated
#define BONE_SCREEN_ALIGN_SPHERE        0x08    // bone aligns to the screen, not constrained in motion.
#define BONE_SCREEN_ALIGN_CYLINDER      0x10    // bone aligns to the screen, constrained by it's own axis.

#define BONE_USED_MASK                  0x0007FF00
#define BONE_USED_BY_ANYTHING           0x0007FF00
#define BONE_USED_BY_HITBOX             0x00000100    // bone (or child) is used by a hit box
#define BONE_USED_BY_ATTACHMENT         0x00000200    // bone (or child) is used by an attachment point
#define BONE_USED_BY_VERTEX_MASK        0x0003FC00
#define BONE_USED_BY_VERTEX_LOD0        0x00000400    // bone (or child) is used by the toplevel model via skinned vertex
#define BONE_USED_BY_VERTEX_LOD1        0x00000800    
#define BONE_USED_BY_VERTEX_LOD2        0x00001000  
#define BONE_USED_BY_VERTEX_LOD3        0x00002000
#define BONE_USED_BY_VERTEX_LOD4        0x00004000
#define BONE_USED_BY_VERTEX_LOD5        0x00008000
#define BONE_USED_BY_VERTEX_LOD6        0x00010000
#define BONE_USED_BY_VERTEX_LOD7        0x00020000
#define BONE_USED_BY_BONE_MERGE         0x00040000    // bone is available for bone merge to occur against it

#define BONE_USED_BY_VERTEX_AT_LOD(lod) ( BONE_USED_BY_VERTEX_LOD0 << (lod) )
#define BONE_USED_BY_ANYTHING_AT_LOD(lod) ( ( BONE_USED_BY_ANYTHING & ~BONE_USED_BY_VERTEX_MASK ) | BONE_USED_BY_VERTEX_AT_LOD(lod) )

#define MAX_NUM_LODS 8
#define MAXSTUDIOBONES		128		// total bones actually used

#define BONE_TYPE_MASK                  0x00F00000
#define BONE_FIXED_ALIGNMENT            0x00100000    // bone can't spin 360 degrees, all interpolation is normalized around a fixed orientation

#define BONE_HAS_SAVEFRAME_POS          0x00200000    // Vector48
#define BONE_HAS_SAVEFRAME_ROT64        0x00400000    // Quaternion64
#define BONE_HAS_SAVEFRAME_ROT32        0x00800000    // Quaternion32

#define BONE_USED_BY_SERVER BONE_USED_BY_HITBOX | BONE_USED_BY_VERTEX_LOD0 | BONE_USED_BY_VERTEX_LOD1 | BONE_USED_BY_VERTEX_LOD2 | BONE_USED_BY_VERTEX_LOD3 | BONE_USED_BY_VERTEX_LOD4 | BONE_USED_BY_VERTEX_LOD5 | BONE_USED_BY_VERTEX_LOD6 | BONE_USED_BY_VERTEX_LOD7

#define Assert( _exp ) ((void)0)
#define CHECK_VALID( _v)

// https://github.com/SteamDatabase/GameTracking-CSGO/blob/master/Protobufs/fatdemo.proto#L3
enum EHitGroup {
  Hitgroup_Generic = 0,
  Hitgroup_Head = 1,
  Hitgroup_Chest = 2,
  Hitgroup_Stomach = 3,
  Hitgroup_LeftArm = 4,
  Hitgroup_RightArm = 5,
  Hitgroup_LeftLeg = 6,
  Hitgroup_RightLeg = 7,
  Hitgroup_Neck = 8,
  Hitgroup_Miss = 9,
  Hitgroup_Gear = 10,
};

enum modtype_t
{
  mod_bad = 0,
  mod_brush,
  mod_sprite,
  mod_studio
};

enum Hitboxes
{
  HITBOX_HEAD,
  HITBOX_NECK,
  HITBOX_LOWER_NECK,
  HITBOX_PELVIS,
  HITBOX_STOMACH,
  HITBOX_LOWER_CHEST,
  HITBOX_CHEST,
  HITBOX_UPPER_CHEST,
  HITBOX_RIGHT_THIGH,
  HITBOX_LEFT_THIGH,
  HITBOX_RIGHT_CALF,
  HITBOX_LEFT_CALF,
  HITBOX_RIGHT_FOOT,
  HITBOX_LEFT_FOOT,
  HITBOX_RIGHT_HAND,
  HITBOX_LEFT_HAND,
  HITBOX_RIGHT_UPPER_ARM,
  HITBOX_RIGHT_FOREARM,
  HITBOX_LEFT_UPPER_ARM,
  HITBOX_LEFT_FOREARM,
  HITBOX_MAX
};

typedef unsigned short MDLHandle_t;

struct mstudiobone_t
{
  int sznameindex;
  inline char *const pszName( void ) const { return ( ( char * )this ) + sznameindex; }
  int parent;
  int bonecontroller[6];

  Vector pos;
  Quaternion quat;
  RadianEuler rot;

  Vector posscale;
  Vector rotscale;

  matrix3x4_t poseToBone;
  Quaternion qAlignment;
  int flags;
  int proctype;
  int procindex;
  mutable int physicsbone;
  inline void *pProcedure( ) const { if( procindex == 0 ) return NULL; else return  ( void * )( ( ( unsigned char * )this ) + procindex ); };
  int surfacepropidx;
  inline char *const pszSurfaceProp( void ) const { return ( ( char * )this ) + surfacepropidx; }
  inline int GetSurfaceProp( void ) const { return surfacepropLookup; }

  int contents;
  int surfacepropLookup;
  int unused[7];
};


struct mstudiobbox_t
{
  int         bone;
  int         group;
  Vector      bbmin;
  Vector      bbmax;
  int         szhitboxnameindex;
  Vector      rotation;//QAngle      m_angAngles;
  float       m_flRadius;
  int32_t     m_iPad02[4];

  char *getHitboxName( ) {
	 if( szhitboxnameindex == 0 )
		return "";

	 return ( ( char* )this ) + szhitboxnameindex;
  }
};

struct mstudiohitboxset_t
{
  int    sznameindex;
  int    numhitboxes;
  int    hitboxindex;

  inline char *const pszName( void ) const {
	 return ( ( char* )this ) + sznameindex;
  }

  inline mstudiobbox_t *pHitbox( int i ) const {
	 if( i > numhitboxes ) return nullptr;
	 return ( mstudiobbox_t* )( ( uint8_t* )this + hitboxindex ) + i;
  }
};

struct model_t
{
   void*				fnHandle;               //0x0000 
   char				szName[ 260 ];          //0x0004 
   __int32			nLoadFlags;             //0x0108 
   __int32			nServerCount;           //0x010C 
   __int32			type;                   //0x0110 
   __int32			flags;                  //0x0114 
   Vector			vecMins;                //0x0118 
   Vector			vecMaxs;                //0x0124 
   float				radius;                 //0x0130 
   __int32		 __pad00134;					//0x0134
   MDLHandle_t		studio;						//0x0138
   char				pad[ 0x16 ];            //0x13A
};//Size=0x0150


class studiohdr_t
{
public:

	__int32 id;                     //0x0000 
	__int32 version;                //0x0004 
	long    checksum;               //0x0008 
	char    szName[ 64 ];             //0x000C 
	__int32 length;                 //0x004C 
	Vector  vecEyePos;              //0x0050 
	Vector  vecIllumPos;            //0x005C 
	Vector  vecHullMin;             //0x0068 
	Vector  vecHullMax;             //0x0074 
	Vector  vecBBMin;               //0x0080 
	Vector  vecBBMax;               //0x008C 
	__int32 flags;                  //0x0098 
	__int32 numbones;               //0x009C 
	__int32 boneindex;              //0x00A0 
	__int32 numbonecontrollers;     //0x00A4 
	__int32 bonecontrollerindex;    //0x00A8 
	__int32 numhitboxsets;          //0x00AC 
	__int32 hitboxsetindex;         //0x00B0 
	int					numlocalanim;			// animations/poses
	int					localanimindex;		// animation descriptions
	int					numlocalseq;				// sequences
	int					localseqindex;

	//private:
	mutable int			activitylistversion;	// initialization flag - have the sequences been indexed?
	mutable int			eventsindexed;

	int					GetSequenceActivity( int iSequence );
	void				SetSequenceActivity( int iSequence, int iActivity );
	int					GetActivityListVersion( void );
	void				SetActivityListVersion( int version ) const;
	int					GetEventListVersion( void );
	void				SetEventListVersion( int version );

	// raw textures
	int					numtextures;
	int					textureindex;


	// raw textures search paths
	int					numcdtextures;
	int					cdtextureindex;

	// replaceable textures tables
	int					numskinref;
	int					numskinfamilies;
	int					skinindex;

	int					numbodyparts;
	int					bodypartindex;

	// queryable attachable points
  //private:
	int					numlocalattachments;
	int					localattachmentindex;
	//public:
	int					GetNumAttachments( void ) const;
	int					GetAttachmentBone( int i );
	// used on my tools in hlmv, not persistant
	void				SetAttachmentBone( int iAttachment, int iBone );

	// animation node to animation node transition graph
  //private:
	int					numlocalnodes;
	int					localnodeindex;
	int					localnodenameindex;

	//public:
	int					EntryNode( int iSequence );
	int					ExitNode( int iSequence );
	char* pszNodeName( int iNode );
	int					GetTransition( int iFrom, int iTo ) const;

	int					numflexdesc;
	int					flexdescindex;

	int					numflexcontrollers;
	int					flexcontrollerindex;

	int					numflexrules;
	int					flexruleindex;

	int					numikchains;
	int					ikchainindex;

	inline const char* pszName( void ) const { return szName; }

	mstudiohitboxset_t* pHitboxSet( int i ) {
		if( i > numhitboxsets ) return nullptr;
		return ( mstudiohitboxset_t* )( ( uint8_t* )this + hitboxsetindex ) + i;
	}
	mstudiobone_t* pBone( int i ) {
		if( i > numbones ) return nullptr;
		return ( mstudiobone_t* )( ( uint8_t* )this + boneindex ) + i;
	}

};//Size=0x00D4