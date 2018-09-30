//-----------------------------------------------------------------------------
// File:   dxtex.h
//
// Author: chenanzhi
//
// Date:  2017-9-30
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef __dxtex_H__
#define __dxtex_H__

#include <windows.h>
#include "formats.h"

class CDxtex
{
public:
	CDxtex();
	~CDxtex();

public:
	bool OnOpenDDSFile(const char * lpszPathName);
	bool OnSaveDDSFile(const char * lpszPathName);

	bool IsNeedCompress();
	bool IsNeedResize();
	bool HasAlpha();
	
	bool GenerateMipMaps(void);
    HRESULT ChangeFormat(LPDIRECT3DBASETEXTURE9 ptexCur, D3DFORMAT fmt, 
                         LPDIRECT3DBASETEXTURE9* pptexNew);
    HRESULT Compress(D3DFORMAT fmt);
    HRESULT Resize(DWORD dwWidthNew, DWORD dwHeightNew);
	HRESULT CreatTexture(DWORD dwWidthNew, DWORD dwWidthNewh, LPDIRECT3DBASETEXTURE9 Src, LPDIRECT3DBASETEXTURE9* pDes);
    DWORD NumMips(void);
    LPDIRECT3DBASETEXTURE9 PtexOrig(void) { return m_ptexOrig; }
    LPDIRECT3DBASETEXTURE9 PtexNew(void) { return m_ptexNew; }
    DWORD DwWidth(void) { return m_dwWidth; }
    DWORD DwHeight(void) { return m_dwHeight; }
    DWORD DwDepth(void) { return m_dwDepth; }
    DWORD DwDepthAt(LONG lwMip);
    bool IsCubeMap(void) { return (m_dwCubeMapFlags > 0); }
    bool IsVolumeMap(void) { return (m_dwDepth > 0); }
	D3DFORMAT GetFormat(LPDIRECT3DBASETEXTURE9 ptex);
private:
    LPDIRECT3DBASETEXTURE9 m_ptexOrig;
    LPDIRECT3DBASETEXTURE9 m_ptexNew;
    DWORD m_dwWidth;
    DWORD m_dwHeight;
    DWORD m_dwDepth; // For volume textures
    DWORD m_numMips;
    DWORD m_dwCubeMapFlags;
   
    HRESULT LoadAlphaIntoSurface(const char* strPath, LPDIRECT3DSURFACE9 psurf);
    HRESULT LoadVolumeSliceFromSurface(LPDIRECT3DVOLUME9 pVolume, UINT iSlice, LPDIRECT3DSURFACE9 pSurf);
    HRESULT LoadSurfaceFromVolumeSlice(LPDIRECT3DVOLUME9 pVolume, UINT iSlice, LPDIRECT3DSURFACE9 psurf);
    HRESULT BltAllLevels(D3DCUBEMAP_FACES FaceType, LPDIRECT3DBASETEXTURE9 ptexSrc, 
        LPDIRECT3DBASETEXTURE9 ptexDest);
   
    HRESULT EnsureAlpha(LPDIRECT3DBASETEXTURE9* pptex);
};

/////////////////////////////////////////////////////////////////////////////


#endif // __dxtex_H__
