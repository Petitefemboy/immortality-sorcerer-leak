//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#ifndef BITVEC_H
#define BITVEC_H
#ifdef _WIN32
#pragma once
#endif

#include <limits.h>
#include <vcruntime_string.h>

class CBitVecAccessor
{
public:
	CBitVecAccessor(unsigned int* pDWords, int iBit);

	void		operator=(int val);
	operator unsigned int();

private:
	unsigned int* m_pDWords;
	int	m_iBit;
};


//-----------------------------------------------------------------------------
// Support functions
//-----------------------------------------------------------------------------

#define LOG2_BITS_PER_INT	5
#define BITS_PER_INT		32

#if _WIN32 && !defined(_X360)
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#endif

inline int FirstBitInWord(unsigned int elem, int offset)
{
#if _WIN32
	if (!elem)
		return -1;
#if _X360
	// this implements CountTrailingZeros() / BitScanForward()
	unsigned int mask = elem - 1;
	unsigned int comp = ~elem;
	elem = mask & comp;
	return (32 - _CountLeadingZeros(elem)) + offset;
#else
	unsigned long out;
	_BitScanForward(&out, elem);
	return out + offset;
#endif

#else
	static unsigned firstBitLUT[256] =
	{
		0,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,
		3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,
		3,0,1,0,2,0,1,0,7,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,
		3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0
	};
	unsigned elemByte;

	elemByte = (elem & 0xFF);
	if (elemByte)
		return offset + firstBitLUT[elemByte];

	elem >>= 8;
	offset += 8;
	elemByte = (elem & 0xFF);
	if (elemByte)
		return offset + firstBitLUT[elemByte];

	elem >>= 8;
	offset += 8;
	elemByte = (elem & 0xFF);
	if (elemByte)
		return offset + firstBitLUT[elemByte];

	elem >>= 8;
	offset += 8;
	elemByte = (elem & 0xFF);
	if (elemByte)
		return offset + firstBitLUT[elemByte];

	return -1;
#endif
}

//-------------------------------------

inline unsigned GetEndMask(int numBits)
{
	static unsigned bitStringEndMasks[] =
	{
		0xffffffff,
		0x00000001,
		0x00000003,
		0x00000007,
		0x0000000f,
		0x0000001f,
		0x0000003f,
		0x0000007f,
		0x000000ff,
		0x000001ff,
		0x000003ff,
		0x000007ff,
		0x00000fff,
		0x00001fff,
		0x00003fff,
		0x00007fff,
		0x0000ffff,
		0x0001ffff,
		0x0003ffff,
		0x0007ffff,
		0x000fffff,
		0x001fffff,
		0x003fffff,
		0x007fffff,
		0x00ffffff,
		0x01ffffff,
		0x03ffffff,
		0x07ffffff,
		0x0fffffff,
		0x1fffffff,
		0x3fffffff,
		0x7fffffff,
	};

	return bitStringEndMasks[numBits % BITS_PER_INT];
}


inline int GetBitForBitnum(int bitNum)
{
	static int bitsForBitnum[] =
	{
		(1 << 0),
		(1 << 1),
		(1 << 2),
		(1 << 3),
		(1 << 4),
		(1 << 5),
		(1 << 6),
		(1 << 7),
		(1 << 8),
		(1 << 9),
		(1 << 10),
		(1 << 11),
		(1 << 12),
		(1 << 13),
		(1 << 14),
		(1 << 15),
		(1 << 16),
		(1 << 17),
		(1 << 18),
		(1 << 19),
		(1 << 20),
		(1 << 21),
		(1 << 22),
		(1 << 23),
		(1 << 24),
		(1 << 25),
		(1 << 26),
		(1 << 27),
		(1 << 28),
		(1 << 29),
		(1 << 30),
		(1 << 31),
	};

	return bitsForBitnum[(bitNum) & (BITS_PER_INT - 1)];
}

inline int GetBitForBitnumByte(int bitNum)
{
	static int bitsForBitnum[] =
	{
		(1 << 0),
		(1 << 1),
		(1 << 2),
		(1 << 3),
		(1 << 4),
		(1 << 5),
		(1 << 6),
		(1 << 7),
	};

	return bitsForBitnum[bitNum & 7];
}

inline int CalcNumIntsForBits(int numBits) { return (numBits + (BITS_PER_INT - 1)) / BITS_PER_INT; }

#ifdef _X360
#define BitVec_Bit( bitNum ) GetBitForBitnum( bitNum )
#define BitVec_BitInByte( bitNum ) GetBitForBitnumByte( bitNum )
#else
#define BitVec_Bit( bitNum ) ( 1 << ( (bitNum) & (BITS_PER_INT-1) ) )
#define BitVec_BitInByte( bitNum ) ( 1 << ( (bitNum) & 7 ) )
#endif
#define BitVec_Int( bitNum ) ( (bitNum) >> LOG2_BITS_PER_INT )


//-----------------------------------------------------------------------------
// template CBitVecT
//
// Defines the operations relevant to any bit array. Simply requires a base
// class that implements GetNumBits(), Base(), GetNumDWords() & ValidateOperand()
//
// CVarBitVec and CBitVec<int> are the actual classes generally used
// by clients
//

template <class BASE_OPS>
class CBitVecT : public BASE_OPS
{
public:
	CBitVecT();
	CBitVecT(int numBits);			// Must be initialized with the number of bits

	void	Init(int val = 0);

	// Access the bits like an array.
	CBitVecAccessor	operator[](int i);

	// Do NOT override bitwise operators (see note in header)
	void	And(const CBitVecT& andStr, CBitVecT* out) const;
	void	Or(const CBitVecT& orStr, CBitVecT* out) const;
	void	Xor(const CBitVecT& orStr, CBitVecT* out) const;

	void	Not(CBitVecT* out) const;

	void	CopyTo(CBitVecT* out) const;
	void	Copy(const CBitVecT<BASE_OPS>& other, int nBits = -1);
	bool	Compare(const CBitVecT<BASE_OPS>& other, int nBits = -1) const;

	bool	IsAllClear(void) const;		// Are all bits zero?
	bool	IsAllSet(void) const;		// Are all bits one?

	unsigned int	Get(unsigned int bitNum) const;
	bool 	IsBitSet(int bitNum) const;
	void 	Set(int bitNum);
	void 	Set(int bitNum, bool bNewVal);
	void 	Clear(int bitNum);

	bool	TestAndSet(int bitNum);

	void 	Set(unsigned int offset, unsigned int mask);
	void 	Clear(unsigned int offset, unsigned int mask);
	unsigned int	Get(unsigned int offset, unsigned int mask);

	void	SetAll(void);			// Sets all bits
	void	ClearAll(void);			// Clears all bits

	unsigned int	GetDWord(int i) const;
	void	SetDWord(int i, unsigned int val);

	CBitVecT<BASE_OPS>& operator=(const CBitVecT<BASE_OPS>& other) { other.CopyTo(this); return *this; }
	bool			operator==(const CBitVecT<BASE_OPS>& other) { return Compare(other); }
	bool			operator!=(const CBitVecT<BASE_OPS>& other) { return !operator==(other); }

	static void GetOffsetMaskForBit(unsigned int bitNum, unsigned int* pOffset, unsigned int* pMask) { *pOffset = BitVec_Int(bitNum); *pMask = BitVec_Bit(bitNum); }
};

//-----------------------------------------------------------------------------
// class CVarBitVecBase
//
// Defines the operations necessary for a variable sized bit array

class CVarBitVecBase
{
public:
	bool	IsFixedSize() const { return false; }
	int		GetNumBits(void) const { return m_numBits; }
	void	Resize(int numBits, bool bClearAll = false);		// resizes bit array

	int 	GetNumDWords() const { return m_numInts; }
	unsigned int* Base() { return m_pInt; }
	const unsigned int* Base() const { return m_pInt; }

	void Attach(unsigned int* pBits, int numBits);
	bool Detach(unsigned int** ppBits, int* pNumBits);

	int		FindNextSetBit(int iStartBit) const; // returns -1 if no set bit was found

protected:
	CVarBitVecBase();
	CVarBitVecBase(int numBits);
	CVarBitVecBase(const CVarBitVecBase& from);
	CVarBitVecBase& operator=(const CVarBitVecBase& from);
	~CVarBitVecBase(void);

	void 		ValidateOperand(const CVarBitVecBase& operand) const { /*Assert(GetNumBits() == operand.GetNumBits());*/ }

	unsigned	GetEndMask() const { return ::GetEndMask(GetNumBits()); }

private:

	unsigned short	m_numBits;					// Number of bits in the bitstring
	unsigned short	m_numInts;					// Number of ints to needed to store bitstring
	unsigned int			m_iBitStringStorage;		// If the bit string fits in one int, it goes here
	unsigned int* m_pInt;					// Array of ints containing the bitstring

	void	AllocInts(int numInts);	// Free the allocated bits
	void	ReallocInts(int numInts);
	void	FreeInts(void);			// Free the allocated bits
};

//-----------------------------------------------------------------------------
// class CFixedBitVecBase
//
// Defines the operations necessary for a fixed sized bit array. 
// 

template <int bits> struct BitCountToEndMask_t { };
template <> struct BitCountToEndMask_t< 0> { enum { MASK = 0xffffffff }; };
template <> struct BitCountToEndMask_t< 1> { enum { MASK = 0x00000001 }; };
template <> struct BitCountToEndMask_t< 2> { enum { MASK = 0x00000003 }; };
template <> struct BitCountToEndMask_t< 3> { enum { MASK = 0x00000007 }; };
template <> struct BitCountToEndMask_t< 4> { enum { MASK = 0x0000000f }; };
template <> struct BitCountToEndMask_t< 5> { enum { MASK = 0x0000001f }; };
template <> struct BitCountToEndMask_t< 6> { enum { MASK = 0x0000003f }; };
template <> struct BitCountToEndMask_t< 7> { enum { MASK = 0x0000007f }; };
template <> struct BitCountToEndMask_t< 8> { enum { MASK = 0x000000ff }; };
template <> struct BitCountToEndMask_t< 9> { enum { MASK = 0x000001ff }; };
template <> struct BitCountToEndMask_t<10> { enum { MASK = 0x000003ff }; };
template <> struct BitCountToEndMask_t<11> { enum { MASK = 0x000007ff }; };
template <> struct BitCountToEndMask_t<12> { enum { MASK = 0x00000fff }; };
template <> struct BitCountToEndMask_t<13> { enum { MASK = 0x00001fff }; };
template <> struct BitCountToEndMask_t<14> { enum { MASK = 0x00003fff }; };
template <> struct BitCountToEndMask_t<15> { enum { MASK = 0x00007fff }; };
template <> struct BitCountToEndMask_t<16> { enum { MASK = 0x0000ffff }; };
template <> struct BitCountToEndMask_t<17> { enum { MASK = 0x0001ffff }; };
template <> struct BitCountToEndMask_t<18> { enum { MASK = 0x0003ffff }; };
template <> struct BitCountToEndMask_t<19> { enum { MASK = 0x0007ffff }; };
template <> struct BitCountToEndMask_t<20> { enum { MASK = 0x000fffff }; };
template <> struct BitCountToEndMask_t<21> { enum { MASK = 0x001fffff }; };
template <> struct BitCountToEndMask_t<22> { enum { MASK = 0x003fffff }; };
template <> struct BitCountToEndMask_t<23> { enum { MASK = 0x007fffff }; };
template <> struct BitCountToEndMask_t<24> { enum { MASK = 0x00ffffff }; };
template <> struct BitCountToEndMask_t<25> { enum { MASK = 0x01ffffff }; };
template <> struct BitCountToEndMask_t<26> { enum { MASK = 0x03ffffff }; };
template <> struct BitCountToEndMask_t<27> { enum { MASK = 0x07ffffff }; };
template <> struct BitCountToEndMask_t<28> { enum { MASK = 0x0fffffff }; };
template <> struct BitCountToEndMask_t<29> { enum { MASK = 0x1fffffff }; };
template <> struct BitCountToEndMask_t<30> { enum { MASK = 0x3fffffff }; };
template <> struct BitCountToEndMask_t<31> { enum { MASK = 0x7fffffff }; };

//-------------------------------------

template <int NUM_BITS>
class CFixedBitVecBase
{
public:
	bool	IsFixedSize() const { return true; }
	int		GetNumBits(void) const { return NUM_BITS; }
	void	Resize(int numBits, bool bClearAll = false) { /*Assert(numBits == NUM_BITS);*/ if (bClearAll) memset(m_Ints, 0, NUM_INTS * sizeof(unsigned int)); }// for syntatic consistency (for when using templates)

	int 			GetNumDWords() const { return NUM_INTS; }
	unsigned int* Base() { return m_Ints; }
	const unsigned int* Base() const { return m_Ints; }

	int		FindNextSetBit(int iStartBit) const; // returns -1 if no set bit was found

protected:
	CFixedBitVecBase() {}
	CFixedBitVecBase(int numBits) { /*Assert(numBits == NUM_BITS);*/ } // doesn't make sense, really. Supported to simplify templates & allow easy replacement of variable 

	void 		ValidateOperand(const CFixedBitVecBase<NUM_BITS>& operand) const { } // no need, compiler does so statically

public: // for test code
	unsigned	GetEndMask() const { return static_cast<unsigned>(BitCountToEndMask_t<NUM_BITS% BITS_PER_INT>::MASK); }

private:
	enum
	{
		NUM_INTS = (NUM_BITS + (BITS_PER_INT - 1)) / BITS_PER_INT
	};

	unsigned int m_Ints[(NUM_BITS + (BITS_PER_INT - 1)) / BITS_PER_INT];
};

//-----------------------------------------------------------------------------
//
// The actual classes used
//

// inheritance instead of typedef to allow forward declarations
class CVarBitVec : public CBitVecT<CVarBitVecBase>
{
public:
	CVarBitVec()
	{
	}

	CVarBitVec(int numBits)
		: CBitVecT<CVarBitVecBase>(numBits)
	{
	}
};

//-----------------------------------------------------------------------------

template < int NUM_BITS >
class CBitVec : public CBitVecT< CFixedBitVecBase<NUM_BITS> >
{
public:
	CBitVec()
	{
	}

	CBitVec(int numBits)
		: CBitVecT< CFixedBitVecBase<NUM_BITS> >(numBits)
	{
	}
};


//-----------------------------------------------------------------------------

typedef CBitVec<32> CDWordBitVec;

//-----------------------------------------------------------------------------

inline CVarBitVecBase::CVarBitVecBase()
{
	memset(this, 0, sizeof(*this));
}

//-----------------------------------------------------------------------------

inline CVarBitVecBase::CVarBitVecBase(int numBits)
{
	//Assert(numBits);
	m_numBits = numBits;

	// Figure out how many ints are needed
	m_numInts = CalcNumIntsForBits(numBits);
	m_pInt = NULL;
	AllocInts(m_numInts);
}

//-----------------------------------------------------------------------------

inline CVarBitVecBase::CVarBitVecBase(const CVarBitVecBase& from)
{
	if (from.m_numInts)
	{
		m_numBits = from.m_numBits;
		m_numInts = from.m_numInts;
		m_pInt = NULL;
		AllocInts(m_numInts);
		memcpy(m_pInt, from.m_pInt, m_numInts * sizeof(int));
	}
	else
		memset(this, 0, sizeof(*this));
}

//-----------------------------------------------------------------------------

inline CVarBitVecBase& CVarBitVecBase::operator=(const CVarBitVecBase& from)
{
	Resize(from.GetNumBits());
	if (m_pInt)
		memcpy(m_pInt, from.m_pInt, m_numInts * sizeof(int));
	return (*this);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Input  :
// Output :
//-----------------------------------------------------------------------------

inline CVarBitVecBase::~CVarBitVecBase(void)
{
	FreeInts();
}

//-----------------------------------------------------------------------------

inline void CVarBitVecBase::Attach(unsigned int* pBits, int numBits)
{
	FreeInts();
	m_numBits = numBits;
	m_numInts = CalcNumIntsForBits(numBits);
	if (m_numInts > 1)
	{
		m_pInt = pBits;
	}
	else
	{
		m_iBitStringStorage = *pBits;
		m_pInt = &m_iBitStringStorage;
		free(pBits);
	}
}

//-----------------------------------------------------------------------------

inline bool CVarBitVecBase::Detach(unsigned int** ppBits, int* pNumBits)
{
	if (!m_numBits)
	{
		return false;
	}

	*pNumBits = m_numBits;
	if (m_numInts > 1)
	{
		*ppBits = m_pInt;
	}
	else
	{
		*ppBits = (unsigned int*)malloc(sizeof(unsigned int));
		**ppBits = m_iBitStringStorage;
		free(m_pInt);
	}

	memset(this, 0, sizeof(*this));
	return true;
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline CBitVecT<BASE_OPS>::CBitVecT()
{
	// undef this is ints are not 4 bytes
	// generate a compile error if sizeof(int) is not 4 (HACK: can't use the preprocessor so use the compiler)

	// COMPILE_TIME_Assert(sizeof(int) == 4);

	// Initialize bitstring by clearing all bits
	ClearAll();
}

//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline CBitVecT<BASE_OPS>::CBitVecT(int numBits)
	: BASE_OPS(numBits)
{
	// undef this is ints are not 4 bytes
	// generate a compile error if sizeof(int) is not 4 (HACK: can't use the preprocessor so use the compiler)

	//COMPILE_TIME_Assert(sizeof(int) == 4);

	// Initialize bitstring by clearing all bits
	ClearAll();
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline CBitVecAccessor CBitVecT<BASE_OPS>::operator[](int i)
{
	//Assert(i >= 0 && i < GetNumBits());
	return CBitVecAccessor(Base(), i);
}


//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Init(int val)
{
	if (Base())
		memset(Base(), (val) ? 0xff : 0, GetNumDWords() * sizeof(int));
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline unsigned int CBitVecT<BASE_OPS>::Get(unsigned int bitNum) const
{
	//Assert(bitNum < (unsigned int)GetNumBits());
	const unsigned int* pInt = Base() + BitVec_Int(bitNum);
	return (*pInt & BitVec_Bit(bitNum));
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline bool CBitVecT<BASE_OPS>::IsBitSet(int bitNum) const
{
	//Assert(bitNum >= 0 && bitNum < GetNumBits());
	const unsigned int* pInt = Base() + BitVec_Int(bitNum);
	return ((*pInt & BitVec_Bit(bitNum)) != 0);
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Set(int bitNum)
{
	//Assert(bitNum >= 0 && bitNum < GetNumBits());
	unsigned int* pInt = Base() + BitVec_Int(bitNum);
	*pInt |= BitVec_Bit(bitNum);
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline bool CBitVecT<BASE_OPS>::TestAndSet(int bitNum)
{
	//Assert(bitNum >= 0 && bitNum < GetNumBits());
	unsigned int bitVecBit = BitVec_Bit(bitNum);
	unsigned int* pInt = Base() + BitVec_Int(bitNum);
	bool bResult = ((*pInt & bitVecBit) != 0);
	*pInt |= bitVecBit;
	return bResult;
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Clear(int bitNum)
{
	//Assert(bitNum >= 0 && bitNum < GetNumBits());
	unsigned int* pInt = Base() + BitVec_Int(bitNum);
	*pInt &= ~BitVec_Bit(bitNum);
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Set(int bitNum, bool bNewVal)
{
	unsigned int* pInt = Base() + BitVec_Int(bitNum);
	unsigned int bitMask = BitVec_Bit(bitNum);
	if (bNewVal)
	{
		*pInt |= bitMask;
	}
	else
	{
		*pInt &= ~bitMask;
	}
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Set(unsigned int offset, unsigned int mask)
{
	unsigned int* pInt = Base() + offset;
	*pInt |= mask;
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Clear(unsigned int offset, unsigned int mask)
{
	unsigned int* pInt = Base() + offset;
	*pInt &= ~mask;
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline unsigned int CBitVecT<BASE_OPS>::Get(unsigned int offset, unsigned int mask)
{
	unsigned int* pInt = Base() + offset;
	return (*pInt & mask);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::And(const CBitVecT& addStr, CBitVecT* out) const
{
	ValidateOperand(addStr);
	ValidateOperand(*out);

	unsigned int* pDest = out->Base();
	const unsigned int* pOperand1 = Base();
	const unsigned int* pOperand2 = addStr.Base();

	for (int i = GetNumDWords() - 1; i >= 0; --i)
	{
		pDest[i] = pOperand1[i] & pOperand2[i];
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Or(const CBitVecT& orStr, CBitVecT* out) const
{
	ValidateOperand(orStr);
	ValidateOperand(*out);

	unsigned int* pDest = out->Base();
	const unsigned int* pOperand1 = Base();
	const unsigned int* pOperand2 = orStr.Base();

	for (int i = GetNumDWords() - 1; i >= 0; --i)
	{
		pDest[i] = pOperand1[i] | pOperand2[i];
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Xor(const CBitVecT& xorStr, CBitVecT* out) const
{
	unsigned int* pDest = out->Base();
	const unsigned int* pOperand1 = Base();
	const unsigned int* pOperand2 = xorStr.Base();

	for (int i = GetNumDWords() - 1; i >= 0; --i)
	{
		pDest[i] = pOperand1[i] ^ pOperand2[i];
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Not(CBitVecT* out) const
{
	ValidateOperand(*out);

	unsigned int* pDest = out->Base();
	const unsigned int* pOperand = Base();

	for (int i = GetNumDWords() - 1; i >= 0; --i)
	{
		pDest[i] = ~(pOperand[i]);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Copy a bit string
// Input  :
// Output :
//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::CopyTo(CBitVecT* out) const
{
	out->Resize(GetNumBits());

	ValidateOperand(*out);
	//Assert(out != this);

	memcpy(out->Base(), Base(), GetNumDWords() * sizeof(int));
}

//-----------------------------------------------------------------------------
// Purpose: Are all bits zero?
// Input  :
// Output :
//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline bool CBitVecT<BASE_OPS>::IsAllClear(void) const
{
	// Number of available bits may be more than the number
	// actually used, so make sure to mask out unused bits
	// before testing for zero
	(const_cast<CBitVecT*>(this))->Base()[GetNumDWords() - 1] &= CBitVecT<BASE_OPS>::GetEndMask(); // external semantics of const retained

	for (int i = GetNumDWords() - 1; i >= 0; --i)
	{
		if (Base()[i] != 0)
		{
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Are all bits set?
// Input  :
// Output :
//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline bool CBitVecT<BASE_OPS>::IsAllSet(void) const
{
	// Number of available bits may be more than the number
	// actually used, so make sure to mask out unused bits
	// before testing for set bits
	(const_cast<CBitVecT*>(this))->Base()[GetNumDWords() - 1] |= ~CBitVecT<BASE_OPS>::GetEndMask();  // external semantics of const retained

	for (int i = GetNumDWords() - 1; i >= 0; --i)
	{
		if (Base()[i] != ~0)
		{
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Sets all bits
// Input  :
// Output :
//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::SetAll(void)
{
	if (Base())
		memset(Base(), 0xff, GetNumDWords() * sizeof(int));
}

//-----------------------------------------------------------------------------
// Purpose: Clears all bits
// Input  :
// Output :
//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::ClearAll(void)
{
	if (Base())
		memset(Base(), 0, GetNumDWords() * sizeof(int));
}

//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Copy(const CBitVecT<BASE_OPS>& other, int nBits)
{
	if (nBits == -1)
	{
		nBits = other.GetNumBits();
	}

	Resize(nBits);

	ValidateOperand(other);
	//Assert(&other != this);

	memcpy(Base(), other.Base(), GetNumDWords() * sizeof(unsigned int));
}

//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline bool CBitVecT<BASE_OPS>::Compare(const CBitVecT<BASE_OPS>& other, int nBits) const
{
	if (nBits == -1)
	{
		if (other.GetNumBits() != GetNumBits())
		{
			return false;
		}

		nBits = other.GetNumBits();
	}

	if (nBits > other.GetNumBits() || nBits > GetNumBits())
	{
		return false;
	}

	(const_cast<CBitVecT*>(this))->Base()[GetNumDWords() - 1] &= CBitVecT<BASE_OPS>::GetEndMask(); // external semantics of const retained
	(const_cast<CBitVecT*>(&other))->Base()[GetNumDWords() - 1] &= other.CBitVecT<BASE_OPS>::GetEndMask(); // external semantics of const retained

	int nBytes = PAD_NUMBER(nBits, 8) >> 3;

	return (memcmp(Base(), other.Base(), nBytes) == 0);
}

//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline unsigned int CBitVecT<BASE_OPS>::GetDWord(int i) const
{
	//Assert(i >= 0 && i < GetNumDWords());
	return Base()[i];
}

//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::SetDWord(int i, unsigned int val)
{
	//Assert(i >= 0 && i < GetNumDWords());
	Base()[i] = val;
}

//-----------------------------------------------------------------------------

inline unsigned GetStartBitMask(int startBit)
{
	static unsigned int g_StartMask[32] =
	{
		0xffffffff,
		0xfffffffe,
		0xfffffffc,
		0xfffffff8,
		0xfffffff0,
		0xffffffe0,
		0xffffffc0,
		0xffffff80,
		0xffffff00,
		0xfffffe00,
		0xfffffc00,
		0xfffff800,
		0xfffff000,
		0xffffe000,
		0xffffc000,
		0xffff8000,
		0xffff0000,
		0xfffe0000,
		0xfffc0000,
		0xfff80000,
		0xfff00000,
		0xffe00000,
		0xffc00000,
		0xff800000,
		0xff000000,
		0xfe000000,
		0xfc000000,
		0xf8000000,
		0xf0000000,
		0xe0000000,
		0xc0000000,
		0x80000000,
	};

	return g_StartMask[startBit & 31];
}

inline int CVarBitVecBase::FindNextSetBit(int startBit) const
{
#if 0
	if (startBit < GetNumBits())
	{
		int wordIndex = BitVec_Int(startBit);
		unsigned int startMask = GetStartBitMask(startBit);
		int lastWord = GetNumDWords() - 1;

		// handle non dword lengths
		if ((GetNumBits() % BITS_PER_INT) != 0)
		{
			unsigned int elem = Base()[wordIndex];
			elem &= startMask;
			if (wordIndex == lastWord)
			{
				elem &= (GetEndMask());
				// there's a bit remaining in this word
				if (elem)
					return FirstBitInWord(elem, wordIndex << 5);
			}
			else
			{
				// there's a bit remaining in this word
				if (elem)
					return FirstBitInWord(elem, wordIndex << 5);

				// iterate the words
				for (int i = wordIndex + 1; i < lastWord; i++)
				{
					elem = Base()[i];
					if (elem)
						return FirstBitInWord(elem, i << 5);
				}
				elem = Base()[lastWord] & GetEndMask();
				if (elem)
					return FirstBitInWord(elem, lastWord << 5);
			}
		}
		else
		{
			const unsigned int* RESTRICT pCurElem = Base() + wordIndex;
			unsigned int elem = *pCurElem;
			elem &= startMask;
			do
			{
				if (elem)
					return FirstBitInWord(elem, wordIndex << 5);
				++pCurElem;
				elem = *pCurElem;
				++wordIndex;
			} while (wordIndex <= lastWord);
		}

	}
#endif
	return -1;
}

template <int NUM_BITS>
inline int CFixedBitVecBase<NUM_BITS>::FindNextSetBit(int startBit) const
{
#if 0
	if (startBit < NUM_BITS)
	{
		int wordIndex = BitVec_Int(startBit);
		unsigned int startMask = GetStartBitMask(startBit);

		// handle non dword lengths
		if ((NUM_BITS % BITS_PER_INT) != 0)
		{
			unsigned int elem = Base()[wordIndex];
			elem &= startMask;
			if (wordIndex == NUM_INTS - 1)
			{
				elem &= (GetEndMask());
				// there's a bit remaining in this word
				if (elem)
					return FirstBitInWord(elem, wordIndex << 5);
			}
			else
			{
				// there's a bit remaining in this word
				if (elem)
					return FirstBitInWord(elem, wordIndex << 5);

				// iterate the words
				for (int i = wordIndex + 1; i < NUM_INTS - 1; i++)
				{
					elem = Base()[i];
					if (elem)
						return FirstBitInWord(elem, i << 5);
				}
				elem = Base()[NUM_INTS - 1] & GetEndMask();
				if (elem)
					return FirstBitInWord(elem, (NUM_INTS - 1) << 5);
			}
		}
		else
		{
			const unsigned int* RESTRICT pCurElem = Base() + wordIndex;
			unsigned int elem = *pCurElem;
			elem &= startMask;
			do
			{
				if (elem)
					return FirstBitInWord(elem, wordIndex << 5);
				++pCurElem;
				elem = *pCurElem;
				++wordIndex;
			} while (wordIndex <= NUM_INTS - 1);
		}

	}
#endif
	return -1;
}

//-----------------------------------------------------------------------------
// Unrolled loops for some common sizes

template<>
__forceinline void CBitVecT< CFixedBitVecBase<256> >::And(const CBitVecT& addStr, CBitVecT* out) const
{
	unsigned int* pDest = out->Base();
	const unsigned int* pOperand1 = Base();
	const unsigned int* pOperand2 = addStr.Base();

	pDest[0] = pOperand1[0] & pOperand2[0];
	pDest[1] = pOperand1[1] & pOperand2[1];
	pDest[2] = pOperand1[2] & pOperand2[2];
	pDest[3] = pOperand1[3] & pOperand2[3];
	pDest[4] = pOperand1[4] & pOperand2[4];
	pDest[5] = pOperand1[5] & pOperand2[5];
	pDest[6] = pOperand1[6] & pOperand2[6];
	pDest[7] = pOperand1[7] & pOperand2[7];
}

template<>
__forceinline bool CBitVecT< CFixedBitVecBase<256> >::IsAllClear(void) const
{
	const unsigned int* pInts = Base();
	return (pInts[0] == 0 && pInts[1] == 0 && pInts[2] == 0 && pInts[3] == 0 && pInts[4] == 0 && pInts[5] == 0 && pInts[6] == 0 && pInts[7] == 0);
}

template<>
__forceinline void CBitVecT< CFixedBitVecBase<256> >::CopyTo(CBitVecT* out) const
{
	unsigned int* pDest = out->Base();
	const unsigned int* pInts = Base();

	pDest[0] = pInts[0];
	pDest[1] = pInts[1];
	pDest[2] = pInts[2];
	pDest[3] = pInts[3];
	pDest[4] = pInts[4];
	pDest[5] = pInts[5];
	pDest[6] = pInts[6];
	pDest[7] = pInts[7];
}

template<>
__forceinline void CBitVecT< CFixedBitVecBase<128> >::And(const CBitVecT& addStr, CBitVecT* out) const
{
	unsigned int* pDest = out->Base();
	const unsigned int* pOperand1 = Base();
	const unsigned int* pOperand2 = addStr.Base();

	pDest[0] = pOperand1[0] & pOperand2[0];
	pDest[1] = pOperand1[1] & pOperand2[1];
	pDest[2] = pOperand1[2] & pOperand2[2];
	pDest[3] = pOperand1[3] & pOperand2[3];
}

template<>
__forceinline bool CBitVecT< CFixedBitVecBase<128> >::IsAllClear(void) const
{
	const unsigned int* pInts = Base();
	return (pInts[0] == 0 && pInts[1] == 0 && pInts[2] == 0 && pInts[3] == 0);
}

template<>
__forceinline void CBitVecT< CFixedBitVecBase<128> >::CopyTo(CBitVecT* out) const
{
	unsigned int* pDest = out->Base();
	const unsigned int* pInts = Base();

	pDest[0] = pInts[0];
	pDest[1] = pInts[1];
	pDest[2] = pInts[2];
	pDest[3] = pInts[3];
}

template<>
inline void CBitVecT< CFixedBitVecBase<96> >::And(const CBitVecT& addStr, CBitVecT* out) const
{
	unsigned int* pDest = out->Base();
	const unsigned int* pOperand1 = Base();
	const unsigned int* pOperand2 = addStr.Base();

	pDest[0] = pOperand1[0] & pOperand2[0];
	pDest[1] = pOperand1[1] & pOperand2[1];
	pDest[2] = pOperand1[2] & pOperand2[2];
}

template<>
inline bool CBitVecT< CFixedBitVecBase<96> >::IsAllClear(void) const
{
	const unsigned int* pInts = Base();
	return (pInts[0] == 0 && pInts[1] == 0 && pInts[2] == 0);
}

template<>
inline void CBitVecT< CFixedBitVecBase<96> >::CopyTo(CBitVecT* out) const
{
	unsigned int* pDest = out->Base();
	const unsigned int* pInts = Base();

	pDest[0] = pInts[0];
	pDest[1] = pInts[1];
	pDest[2] = pInts[2];
}

template<>
inline void CBitVecT< CFixedBitVecBase<64> >::And(const CBitVecT& addStr, CBitVecT* out) const
{
	unsigned int* pDest = out->Base();
	const unsigned int* pOperand1 = Base();
	const unsigned int* pOperand2 = addStr.Base();

	pDest[0] = pOperand1[0] & pOperand2[0];
	pDest[1] = pOperand1[1] & pOperand2[1];
}

template<>
inline bool CBitVecT< CFixedBitVecBase<64> >::IsAllClear(void) const
{
	const unsigned int* pInts = Base();
	return (pInts[0] == 0 && pInts[1] == 0);
}

template<>
inline void CBitVecT< CFixedBitVecBase<64> >::CopyTo(CBitVecT* out) const
{
	unsigned int* pDest = out->Base();
	const unsigned int* pInts = Base();

	pDest[0] = pInts[0];
	pDest[1] = pInts[1];
}

template<>
inline void CBitVecT< CFixedBitVecBase<32> >::And(const CBitVecT& addStr, CBitVecT* out) const
{
	unsigned int* pDest = out->Base();
	const unsigned int* pOperand1 = Base();
	const unsigned int* pOperand2 = addStr.Base();

	pDest[0] = pOperand1[0] & pOperand2[0];
}

template<>
inline bool CBitVecT< CFixedBitVecBase<32> >::IsAllClear(void) const
{
	const unsigned int* pInts = Base();

	return (pInts[0] == 0);
}

template<>
inline void CBitVecT< CFixedBitVecBase<32> >::CopyTo(CBitVecT* out) const
{
	unsigned int* pDest = out->Base();
	const unsigned int* pInts = Base();

	pDest[0] = pInts[0];
}

//-----------------------------------------------------------------------------

template <>
inline unsigned int CBitVecT< CFixedBitVecBase<32> >::Get(unsigned int bitNum) const
{
	return (*Base() & BitVec_Bit(bitNum));
}

//-----------------------------------------------------------------------------

template <>
inline bool CBitVecT< CFixedBitVecBase<32> >::IsBitSet(int bitNum) const
{
	return ((*Base() & BitVec_Bit(bitNum)) != 0);
}

//-----------------------------------------------------------------------------

template <>
inline void CBitVecT< CFixedBitVecBase<32> >::Set(int bitNum)
{
	*Base() |= BitVec_Bit(bitNum);
}

//-----------------------------------------------------------------------------

template <>
inline void CBitVecT< CFixedBitVecBase<32> >::Clear(int bitNum)
{
	*Base() &= ~BitVec_Bit(bitNum);
}

//-----------------------------------------------------------------------------

template <>
inline void CBitVecT< CFixedBitVecBase<32> >::Set(int bitNum, bool bNewVal)
{
	unsigned int bitMask = BitVec_Bit(bitNum);
	if (bNewVal)
	{
		*Base() |= bitMask;
	}
	else
	{
		*Base() &= ~bitMask;
	}
}


//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Resizes the bit string to a new number of bits
// Input  : resizeNumBits - 
//-----------------------------------------------------------------------------
inline void CVarBitVecBase::Resize(int resizeNumBits, bool bClearAll)
{
	//Assert(resizeNumBits >= 0 && resizeNumBits <= USHRT_MAX);

	int newIntCount = CalcNumIntsForBits(resizeNumBits);
	if (newIntCount != GetNumDWords())
	{
		if (Base())
		{
			ReallocInts(newIntCount);
			if (!bClearAll && resizeNumBits >= GetNumBits())
			{
				Base()[GetNumDWords() - 1] &= GetEndMask();
				memset(Base() + GetNumDWords(), 0, (newIntCount - GetNumDWords()) * sizeof(int));
			}
		}
		else
		{
			// Figure out how many ints are needed
			AllocInts(newIntCount);
			// Initialize bitstring by clearing all bits
			bClearAll = true;
		}

		m_numInts = newIntCount;
	}
	else if (!bClearAll && resizeNumBits >= GetNumBits() && Base())
	{
		Base()[GetNumDWords() - 1] &= GetEndMask();
	}

	if (bClearAll && Base())
	{
		memset(Base(), 0, newIntCount * sizeof(int));
	}

	// store the new size and end mask
	m_numBits = resizeNumBits;
}

//-----------------------------------------------------------------------------
// Purpose: Allocate the storage for the ints
// Input  : numInts - 
//-----------------------------------------------------------------------------
inline void CVarBitVecBase::AllocInts(int numInts)
{
	//Assert(!m_pInt);

	if (numInts == 0)
		return;

	if (numInts == 1)
	{
		m_pInt = &m_iBitStringStorage;
		return;
	}

	m_pInt = (unsigned int*)malloc(numInts * sizeof(int));
}


//-----------------------------------------------------------------------------
// Purpose: Reallocate the storage for the ints
// Input  : numInts - 
//-----------------------------------------------------------------------------
#if 0
inline void CVarBitVecBase::ReallocInts(int numInts)
{
	//Assert(Base());
	if (numInts == 0)
	{
		FreeInts();
		return;
	}

	if (m_pInt == &m_iBitStringStorage)
	{
		if (numInts != 1)
		{
			m_pInt = ((unsigned int*)malloc(numInts * sizeof(int)));
			*m_pInt = m_iBitStringStorage;
		}

		return;
	}

	if (numInts == 1)
	{
		m_iBitStringStorage = *m_pInt;
		free(m_pInt);
		m_pInt = &m_iBitStringStorage;
		return;
	}

	m_pInt = (unsigned int*)realloc(m_pInt, numInts * sizeof(int));
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Free storage allocated with AllocInts
//-----------------------------------------------------------------------------
inline void CVarBitVecBase::FreeInts(void)
{
	if (m_numInts > 1)
	{
		free(m_pInt);
	}
	m_pInt = NULL;
}

// ------------------------------------------------------------------------ //
// CBitVecAccessor inlines.
// ------------------------------------------------------------------------ //

inline CBitVecAccessor::CBitVecAccessor(unsigned int* pDWords, int iBit)
{
	m_pDWords = pDWords;
	m_iBit = iBit;
}


inline void CBitVecAccessor::operator=(int val)
{
	if (val)
		m_pDWords[m_iBit >> 5] |= (1 << (m_iBit & 31));
	else
		m_pDWords[m_iBit >> 5] &= ~(unsigned long)(1 << (m_iBit & 31));
}

inline CBitVecAccessor::operator unsigned int()
{
	return m_pDWords[m_iBit >> 5] & (1 << (m_iBit & 31));
}


//=============================================================================

#endif // BITVEC_H