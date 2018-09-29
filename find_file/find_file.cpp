#include "pch.h"
#include "find_file.h"

const char* GetRootPath( void )
{
	struct Initor
	{
		char szPath[ MAX_PATH ];
		Initor()
		{
			::GetModuleFileNameA( NULL , szPath , MAX_PATH );
			int len = strlen( szPath );
			for( int i = len - 1 ; i > 0 ; --i )
			{
				char c = szPath[ i ];
				if( c == '\\' || c == '/' )
				{
					szPath[ i + 1 ] = 0;
					break;
				}
			}

			for( int i = 0 ; szPath[ i ] ; ++i )
				if( szPath[ i ] == '\\' )
					szPath[ i ] = '/';
		}
	};
	static Initor g_Initor;
	return g_Initor.szPath;
}

void WriteLog( const char* _szFmt , ... )
{
	struct LogFile
	{
		RTL_CRITICAL_SECTION	cs;
		FILE*					pFile;

		LogFile()
		{
			char szAbsPath[ MAX_PATH ] = { 0 };
			strcat( szAbsPath , GetRootPath() );
			strcat( szAbsPath , "BuildShader.log" );
			pFile = fopen( szAbsPath , "wb" );
			::InitializeCriticalSectionAndSpinCount( &cs , 4096 );
		}
	
		void Write( const char* _szStr )
		{
			::EnterCriticalSection( &cs );
			fwrite( _szStr , 1 , strlen( _szStr ) , pFile );
			::LeaveCriticalSection( &cs );		
		}

		~LogFile()
		{
			::DeleteCriticalSection( &cs ); 
			fclose( pFile );
		}
	};

	static LogFile g_LogFile;

	va_list ap;   
	va_start( ap , _szFmt ); 
	char szBuf[ 4096 ];
	vsprintf( szBuf , _szFmt , ap );
	va_end(ap);
	OutputDebugStringA( szBuf );
	g_LogFile.Write(szBuf);
}

void WriteFileData( const char* _szPath , const void* _pData , unsigned int _nData )
{
	FILE* pFile = fopen( _szPath , "wb" );
	if( pFile )
	{
		fwrite( _pData , 1 , _nData , pFile );
		fclose( pFile );
	}
}

bool GetFileData( const char* _szPath , vector< char >& Data_ )
{
	Data_.clear();
	unsigned int fSize = 0;
	FILE* pF = fopen( _szPath , "rb" );
	if( pF != 0 )
	{
		fseek(pF, 0, SEEK_END);
		fSize = ftell(pF);
		fseek(pF, 0, SEEK_SET);
		if( fSize )
		{
			Data_.resize(fSize); 
			fread( &Data_[0] , fSize , 1 , pF );
		}
		fclose( pF );
		return true;
	}
	return false;
}

void FindFiles::DoPath(const char* _szDir, const char* _ext )
{
	WIN32_FIND_DATAA* pFD = new WIN32_FIND_DATAA;
	AutoBuf< char , MAX_PATH > szSearch( 0 );
	strcat(szSearch.pData, _szDir);
	strcat(szSearch.pData, "\\*");
	HANDLE hFind = FindFirstFile( szSearch.pData , pFD );
	if( INVALID_HANDLE_VALUE != hFind ) 
	{
		do
		{
			char* pName = pFD->cFileName;
			if( pName[0] == L'.' && pName[1] == 0 )
				continue;
			if( pName[0] == L'.' && pName[1] == L'.' && pName[2] == 0 )
				continue;
			if (pFD->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
				continue;
			AutoBuf< char , MAX_PATH > szFile( 0 );
			strcat(szFile.pData,_szDir);
			strcat(szFile.pData,"\\");
			strcat(szFile.pData,pName);
			if ((pFD->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && m_Recru)
			{
				DoPath(szFile.pData, _ext);
			}
			else
			{
				if (strstr(pName, _ext))
				{
					if (!OnFindFile(szFile.pData, _ext))
						break;
				}
			}
		}
		while( FindNextFileA( hFind , pFD ) != 0 );
		FindClose( hFind );
	}
	delete pFD;
}