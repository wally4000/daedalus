// Profiler.h: interface for the CProfiler class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#ifndef UTILITY_PROFILER_H_
#define UTILITY_PROFILER_H_

#ifdef DAEDALUS_ENABLE_PROFILING

#include "Base/Singleton.h"

struct SProfileItemHandle;

class CProfiler
{
	public:
		CProfiler();
		virtual ~CProfiler();

		void					Display();
		void					Update();

		SProfileItemHandle		AddItem( const char * p_str );

		void					Enter( SProfileItemHandle handle );
		void					Exit( SProfileItemHandle handle );

	protected:
		class CProfilerImpl * mpImpl;
};

std::unique_ptr<CProfiler> gProfiler;
//*************************************************************************************
//
//*************************************************************************************
struct SProfileItemHandle
{
	explicit SProfileItemHandle( u32 handle )
		:	Handle( handle )
	{
	}

	u32		Handle;
};

//*************************************************************************************
//
//*************************************************************************************
class CAutoProfile
{
public:
	CAutoProfile( SProfileItemHandle handle )
		:	mHandle( handle )
	{
		gProfiler->Enter( mHandle );
	}

	~CAutoProfile()
	{
		gProfiler->Exit( mHandle );
	}

private:
	SProfileItemHandle		mHandle;
};
#endif

//*************************************************************************************
//
//*************************************************************************************

#ifdef DAEDALUS_ENABLE_PROFILING

#define DAEDALUS_PROFILE( x )											\
	static	SProfileItemHandle		_profile_item( gProfiler->AddItem( x ) );	\
	CAutoProfile					_auto_profile( _profile_item );

#else

#define DAEDALUS_PROFILE( x )

#endif

#endif // UTILITY_PROFILER_H_
