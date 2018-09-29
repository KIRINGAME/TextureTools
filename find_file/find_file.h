#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include <chrono>

using namespace std;

#pragma warning ( disable : 4996 )

void WriteLog( const char* _szFmt , ... );

const char* GetRootPath( void );

bool GetFileData( const char* _szPath , vector< char >& Data_ );

void WriteFileData( const char* _szPath , const void* _pData , unsigned int _nData );

template< typename T , DWORD N >
struct AutoBuf
{
	const static DWORD nSize = N;
	typedef T	type;
	type*		pData;
	AutoBuf( const T& _initor )
	{ 
		pData = new type[ N ];
		for( DWORD i = 0 ; i < nSize ; ++i )
			pData[ i ] = _initor;
	}
	~AutoBuf()
	{ 
		delete[] pData;
	}
};

//inline bool CheckFileExt( const char* _szFile , const char* _szExt )
//{
//	int sf = strlen( _szFile );
//	int se = strlen( _szExt );
//	if( sf < se )
//		return false;
//	const char* fe = _szFile + sf - se;
//	return stricmp( fe , _szExt ) == 0;
//}

class FindFiles
{
protected:
	
	const char*	m_szPath; 
	bool		m_Recru;

	void DoPath( const char* _szDir, const char* _ext );

	virtual bool OnFindFile( const char* _szFile, const char* _ext ) = 0; 

public:
	
	void Find( const char* _szPath , const char* _ext, bool _Recru = true )
	{
		m_szPath	=	_szPath; 
		m_Recru		=	_Recru;
		DoPath( m_szPath, _ext );
	}
};



struct gx_time_printer
{
	const char*			sz;
	decltype(chrono::high_resolution_clock::now())	t0;

	gx_time_printer(const char* _sz) :sz(_sz) {
		t0 = chrono::high_resolution_clock::now();
	}

	~gx_time_printer() {
		auto t1 = chrono::high_resolution_clock::now();
		int dur = (int)chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count();
		float us = dur / 1000.0f;
		fprintf(stdout, "%s \t %f\n", sz,us);
	}
};

#define GX_TIMER(N) gx_time_printer timer_##__LINE__(N);