//-----------------------------------------------------------------------------
// File:   dxtex.cpp
//
// Author: chenanzhi
//
// Date:  2017-9-30
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "pch.h"
#include "dxtex.h"
#include <string>

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IDirect3DDevice9* GetD3DDev()
{
	struct myD3D
	{
		IDirect3D9*								pD3D;
		IDirect3DDevice9*						pDev;

		myD3D()
		{
			pD3D = Direct3DCreate9(D3D_SDK_VERSION);
			if (!pD3D)
				return;

			D3DPRESENT_PARAMETERS	D3DPP;
			ZeroMemory(&D3DPP, sizeof(D3DPP));

			D3DPP.Windowed = TRUE;
			D3DPP.SwapEffect = D3DSWAPEFFECT_DISCARD;
			D3DPP.BackBufferFormat = D3DFMT_UNKNOWN;
			D3DPP.EnableAutoDepthStencil = TRUE;
			D3DPP.AutoDepthStencilFormat = D3DFMT_D24X8;
			D3DPP.BackBufferWidth = GetSystemMetrics(SM_CXFULLSCREEN);
			D3DPP.BackBufferHeight = GetSystemMetrics(SM_CYFULLSCREEN);
			D3DPP.hDeviceWindow = ::GetDesktopWindow();
			D3DPP.FullScreen_RefreshRateInHz = 0;
			D3DPP.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

			pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, ::GetDesktopWindow(), D3DCREATE_HARDWARE_VERTEXPROCESSING, &D3DPP, &pDev);
		}

		~myD3D()
		{
			if (pD3D)
				pD3D->Release();
			if (pDev)
				pDev->Release();
		}
	};

	static myD3D g_myD3D;

	return g_myD3D.pDev;
}

// Helper function that tells whether the given D3DFMT has a working alpha channel
bool FormatContainsAlpha( D3DFORMAT fmt )
{
    bool bHasAlpha = false;

    for( int i=0; i < fmtInfoArraySize; i++ )
    {
        if( fmtInfoArray[i].fmt == fmt )
        {
            bHasAlpha = fmtInfoArray[i].bHasAlpha;
            break;
        }
    }

    return bHasAlpha;
}

/////////////////////////////////////////////////////////////////////////////
// CDxtex construction/destruction

CDxtex::CDxtex()
{
    m_ptexOrig = NULL;
    m_ptexNew = NULL;
    m_dwWidth = 0;
    m_dwHeight = 0;
    m_dwDepth = 0;
    m_numMips = 0;
    m_dwCubeMapFlags = 0;
}


CDxtex::~CDxtex()
{
    ReleasePpo(&m_ptexOrig);
    ReleasePpo(&m_ptexNew);
}

bool CDxtex::OnOpenDDSFile(const char * lpszPathName)
{
    LPDIRECT3DDEVICE9 pd3ddev = GetD3DDev();
    D3DXIMAGE_INFO imageinfo;
    D3DXIMAGE_INFO imageinfo2;

    if( FAILED( D3DXGetImageInfoFromFile( lpszPathName, &imageinfo ) ) )
    {
		fprintf(stderr, "Error: The file '%s' is not a valid file. convert DDS failed!\n", lpszPathName);
        return false;
    }

	if (imageinfo.ImageFileFormat != D3DXIFF_DDS && imageinfo.ImageFileFormat != D3DXIFF_PNG)
	{
		fprintf(stderr, "Error: The file '%s' is not a valid DDS or PNG file. convert DDS failed!\n", lpszPathName);
		return false;
	}

    switch( imageinfo.ResourceType )
    {
    case D3DRTYPE_TEXTURE:
        if( FAILED( D3DXCreateTextureFromFileEx( pd3ddev, lpszPathName, 
            imageinfo.Width, imageinfo.Height, imageinfo.MipLevels, 0,
            imageinfo.Format, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_FILTER_NONE, 0, 
            &imageinfo2, NULL, (LPDIRECT3DTEXTURE9*)&m_ptexOrig ) ) )
        {
            return false;
        }
        m_dwWidth = imageinfo2.Width;
        m_dwHeight = imageinfo2.Height;
        m_dwDepth = 0;
        m_numMips = imageinfo2.MipLevels;
        break;

    case D3DRTYPE_VOLUMETEXTURE:
        if( FAILED( D3DXCreateVolumeTextureFromFileEx( pd3ddev, lpszPathName, 
            imageinfo.Width, imageinfo.Height, imageinfo.Depth, imageinfo.MipLevels,
            0, imageinfo.Format, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_FILTER_NONE,
            0, &imageinfo2, NULL, (LPDIRECT3DVOLUMETEXTURE9*)&m_ptexOrig ) ) )
        {
            return false;
        }
        m_dwWidth = imageinfo2.Width;
        m_dwHeight = imageinfo2.Height;
        m_dwDepth = imageinfo2.Depth;
        m_numMips = imageinfo2.MipLevels;
        break;

    case D3DRTYPE_CUBETEXTURE:
        if( FAILED( D3DXCreateCubeTextureFromFileEx( pd3ddev, lpszPathName, 
            imageinfo.Width, imageinfo.MipLevels, 0, imageinfo.Format, 
            D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_FILTER_NONE, 
            0, &imageinfo2, NULL, (LPDIRECT3DCUBETEXTURE9*)&m_ptexOrig ) ) )
        {
            return false;
        }
        m_dwWidth = imageinfo2.Width;
        m_dwHeight = imageinfo2.Height;
        m_dwDepth = 0;
        m_numMips = imageinfo2.MipLevels;
        m_dwCubeMapFlags = DDS_CUBEMAP_ALLFACES;
        break;

    default:
        return false;
    }

    return true;
}


bool CDxtex::OnSaveDDSFile(const char * lpszPathName)
{
    LPDIRECT3DBASETEXTURE9 ptex;
    ptex = (m_ptexNew == NULL ? m_ptexOrig : m_ptexNew);
    
    if( FAILED( D3DXSaveTextureToFile( lpszPathName, D3DXIFF_DDS, ptex, NULL ) ) )
    {
        return false;
    }

    return true;
}

bool CDxtex::IsNeedCompress()
{
	D3DFORMAT format = GetFormat(m_ptexOrig);
	if (D3DFMT_DXT1 == format ||
		D3DFMT_DXT2 == format ||
		D3DFMT_DXT3 == format ||
		D3DFMT_DXT4 == format ||
		D3DFMT_DXT5 == format)
	{
		return false;
	}

	return true;
}

bool CDxtex::IsNeedResize()
{
	return m_dwWidth != m_dwHeight;
}

bool CDxtex::HasAlpha()
{
	D3DFORMAT format = GetFormat(m_ptexOrig);
	return FormatContainsAlpha(format);
}

D3DFORMAT CDxtex::GetFormat(LPDIRECT3DBASETEXTURE9 ptex)
{
    LPDIRECT3DTEXTURE9 pmiptex = NULL;
    LPDIRECT3DCUBETEXTURE9 pcubetex = NULL;
    LPDIRECT3DVOLUMETEXTURE9 pvoltex = NULL;
    D3DFORMAT fmt = D3DFMT_UNKNOWN;

    if (IsVolumeMap())
        pvoltex = (LPDIRECT3DVOLUMETEXTURE9)ptex;
    else if (IsCubeMap())
        pcubetex = (LPDIRECT3DCUBETEXTURE9)ptex;
    else
        pmiptex = (LPDIRECT3DTEXTURE9)ptex;

    if (pvoltex != NULL)
    {
        D3DVOLUME_DESC vd;
        pvoltex->GetLevelDesc(0, &vd);
        fmt = vd.Format;
    }
    else if (pcubetex != NULL)
    {
        D3DSURFACE_DESC sd;
        pcubetex->GetLevelDesc(0, &sd);
        fmt = sd.Format;
    }
    else if( pmiptex != NULL )
    {
        D3DSURFACE_DESC sd;
        pmiptex->GetLevelDesc(0, &sd);
        fmt = sd.Format;
    }
    return fmt;
}



// If *pptex's current format has less than 4 bits of alpha, change
// it to a similar format that has at least 4 bits of alpha.
HRESULT CDxtex::EnsureAlpha(LPDIRECT3DBASETEXTURE9* pptex)
{
    HRESULT hr;
    D3DFORMAT fmtCur = GetFormat(*pptex);
    D3DFORMAT fmtNew = D3DFMT_UNKNOWN;
    LPDIRECT3DBASETEXTURE9 ptex = NULL;

    switch (fmtCur)
    {
    case D3DFMT_X8R8G8B8:
    case D3DFMT_R8G8B8:
        fmtNew = D3DFMT_A8R8G8B8;
        break;

    case D3DFMT_X1R5G5B5:
    case D3DFMT_R5G6B5:
        fmtNew = D3DFMT_A1R5G5B5;
        break;

    case D3DFMT_X8B8G8R8:
        fmtNew = D3DFMT_A8B8G8R8;
        break;

    case D3DFMT_L8:
        fmtNew = D3DFMT_A8L8;
        break;

    default:
        break;
    }

    if( fmtNew != D3DFMT_UNKNOWN )
    {
        if (FAILED(hr = ChangeFormat(m_ptexOrig, fmtNew, &ptex)))
            return hr;
        ReleasePpo(&m_ptexOrig);
        m_ptexOrig = ptex;
    }

    return S_OK;
}

HRESULT CDxtex::ChangeFormat(LPDIRECT3DBASETEXTURE9 ptexCur, D3DFORMAT fmtTo, 
                                LPDIRECT3DBASETEXTURE9* pptexNew)
{
    HRESULT hr;
    LPDIRECT3DDEVICE9 pd3ddev = GetD3DDev();
    LPDIRECT3DTEXTURE9 pmiptex;
    LPDIRECT3DCUBETEXTURE9 pcubetex;
    LPDIRECT3DVOLUMETEXTURE9 pvoltex;
    D3DFORMAT fmtFrom;
    LPDIRECT3DTEXTURE9 pmiptexNew;
    LPDIRECT3DCUBETEXTURE9 pcubetexNew;
    LPDIRECT3DVOLUMETEXTURE9 pvoltexNew;

    if (IsVolumeMap())
    {
        pvoltex = (LPDIRECT3DVOLUMETEXTURE9)ptexCur;
        D3DVOLUME_DESC vd;
        pvoltex->GetLevelDesc(0, &vd);
        fmtFrom = vd.Format;
    }
    else if (IsCubeMap())
    {
        pcubetex = (LPDIRECT3DCUBETEXTURE9)ptexCur;
        D3DSURFACE_DESC sd;
        pcubetex->GetLevelDesc(0, &sd);
        fmtFrom = sd.Format;
    }
    else
    {
        pmiptex = (LPDIRECT3DTEXTURE9)ptexCur;
        D3DSURFACE_DESC sd;
        pmiptex->GetLevelDesc(0, &sd);
        fmtFrom = sd.Format;
    }

    if (fmtFrom == D3DFMT_DXT2 || fmtFrom == D3DFMT_DXT4)
    {
        if (fmtTo == D3DFMT_DXT1)
        {
            //AfxMessageBox("Warning: The source image contains premultiplied alpha, and the RGB values will be copied to the destination without ""unpremultiplying"" them, so the resulting colors may be affected.");
        }
        else if (fmtTo != D3DFMT_DXT2 && fmtTo != D3DFMT_DXT4)
        {
            //AfxMessageBox("This operation cannot be performed because the source image uses premultiplied alpha.");
            return S_OK;
        }
    }

    if (IsVolumeMap())
    {
        hr = pd3ddev->CreateVolumeTexture(m_dwWidth, m_dwHeight, m_dwDepth, m_numMips,
            0, fmtTo, D3DPOOL_SYSTEMMEM, &pvoltexNew, NULL);
        if (FAILED(hr))
            return hr;
        *pptexNew = pvoltexNew;
        if (FAILED(BltAllLevels(D3DCUBEMAP_FACE_FORCE_DWORD, ptexCur, *pptexNew)))
            return hr;
    }
    else if (IsCubeMap())
    {
        hr = pd3ddev->CreateCubeTexture(m_dwWidth, m_numMips, 
             0, fmtTo, D3DPOOL_MANAGED, &pcubetexNew, NULL);
        if (FAILED(hr))
            return hr;
        *pptexNew = pcubetexNew;
        if (FAILED(hr = BltAllLevels(D3DCUBEMAP_FACE_NEGATIVE_X, ptexCur, *pptexNew)))
            return hr;
        if (FAILED(hr = BltAllLevels(D3DCUBEMAP_FACE_POSITIVE_X, ptexCur, *pptexNew)))
            return hr;
        if (FAILED(hr = BltAllLevels(D3DCUBEMAP_FACE_NEGATIVE_Y, ptexCur, *pptexNew)))
            return hr;
        if (FAILED(hr = BltAllLevels(D3DCUBEMAP_FACE_POSITIVE_Y, ptexCur, *pptexNew)))
            return hr;
        if (FAILED(hr = BltAllLevels(D3DCUBEMAP_FACE_NEGATIVE_Z, ptexCur, *pptexNew)))
            return hr;
        if (FAILED(hr = BltAllLevels(D3DCUBEMAP_FACE_POSITIVE_Z, ptexCur, *pptexNew)))
            return hr;
    }
    else
    {
        if ((fmtTo == D3DFMT_DXT1 || fmtTo == D3DFMT_DXT2 ||
            fmtTo == D3DFMT_DXT3 || fmtTo == D3DFMT_DXT4 ||
            fmtTo == D3DFMT_DXT5) && (m_dwWidth % 4 != 0 || m_dwHeight % 4 != 0))
        {
            //AfxMessageBox("This operation cannot be performed because DXTn textures must have dimensions that are multiples of 4.");
            return E_FAIL;
        }

        hr = pd3ddev->CreateTexture(m_dwWidth, m_dwHeight, m_numMips, 
             0, fmtTo, D3DPOOL_MANAGED, &pmiptexNew, NULL);
        if (FAILED(hr))
            return hr;
        *pptexNew = pmiptexNew;
        if (FAILED(BltAllLevels(D3DCUBEMAP_FACE_FORCE_DWORD, ptexCur, *pptexNew)))
            return hr;
    }
    return S_OK;
}




HRESULT CDxtex::Compress(D3DFORMAT fmtTo)
{
    HRESULT hr;
    LPDIRECT3DBASETEXTURE9 ptexNew = NULL;

    if (FAILED(hr = ChangeFormat(m_ptexOrig, fmtTo, &ptexNew)))
        return hr;

    ReleasePpo(&m_ptexNew);
    m_ptexNew = ptexNew;
    
    return S_OK;
}


bool CDxtex::GenerateMipMaps()
{
    LONG lwTempH;
    LONG lwTempW;
    LONG lwPowsW;
    LONG lwPowsH;
    LPDIRECT3DTEXTURE9 pddsNew = NULL;
    D3DFORMAT fmt;
    HRESULT hr;
    LPDIRECT3DDEVICE9 pd3ddev = GetD3DDev();
    LPDIRECT3DTEXTURE9 pmiptex = NULL;
    LPDIRECT3DCUBETEXTURE9 pcubetex = NULL;
    LPDIRECT3DVOLUMETEXTURE9 pvoltex = NULL;
    LPDIRECT3DTEXTURE9 pmiptexNew = NULL;
    LPDIRECT3DCUBETEXTURE9 pcubetexNew = NULL;
    LPDIRECT3DVOLUMETEXTURE9 pvoltexNew = NULL;
    LPDIRECT3DSURFACE9 psurfSrc;
    LPDIRECT3DSURFACE9 psurfDest;
    LPDIRECT3DVOLUME9 pvolSrc;
    LPDIRECT3DVOLUME9 pvolDest;

    if (IsVolumeMap())
        pvoltex = (LPDIRECT3DVOLUMETEXTURE9)m_ptexOrig;
    else if (IsCubeMap())
        pcubetex = (LPDIRECT3DCUBETEXTURE9)m_ptexOrig;
    else
        pmiptex = (LPDIRECT3DTEXTURE9)m_ptexOrig;

    if (pvoltex != NULL)
    {
        D3DVOLUME_DESC vd;
        pvoltex->GetLevelDesc(0, &vd);
        fmt = vd.Format;
    }
    else if (pcubetex != NULL)
    {
        D3DSURFACE_DESC sd;
        pcubetex->GetLevelDesc(0, &sd);
        fmt = sd.Format;
    }
    else
    {
        D3DSURFACE_DESC sd;
        pmiptex->GetLevelDesc(0, &sd);
        fmt = sd.Format;
    }

    lwTempW = m_dwWidth;
    lwTempH = m_dwHeight;
    lwPowsW = 0;
    lwPowsH = 0;
    while (lwTempW > 0)
    {
        lwPowsW++;
        lwTempW = lwTempW / 2;
    }
    while (lwTempH > 0)
    {
        lwPowsH++;
        lwTempH = lwTempH / 2;
    }
    m_numMips = lwPowsW > lwPowsH ? lwPowsW : lwPowsH;

    // Create destination mipmap surface - same format as source
    if (pvoltex != NULL)
    {
        if (FAILED(hr = pd3ddev->CreateVolumeTexture(m_dwWidth, m_dwHeight, m_dwDepth, 
            m_numMips, 0, fmt, D3DPOOL_SYSTEMMEM, &pvoltexNew, NULL)))
        {
            goto LFail;
        }
        hr = pvoltex->GetVolumeLevel(0, &pvolSrc);
        hr = pvoltexNew->GetVolumeLevel(0, &pvolDest);
        hr = D3DXLoadVolumeFromVolume(pvolDest, NULL, NULL, pvolSrc, NULL, NULL, 
            D3DX_DEFAULT, 0);
        ReleasePpo(&pvolSrc);
        ReleasePpo(&pvolDest);
        hr = D3DXFilterVolumeTexture(pvoltexNew, NULL, 0, D3DX_DEFAULT);
    }
    else if (pmiptex != NULL)
    {
        if (FAILED(hr = pd3ddev->CreateTexture(m_dwWidth, m_dwHeight, m_numMips, 
             0, fmt, D3DPOOL_MANAGED, &pmiptexNew, NULL)))
        {
            goto LFail;
        }
        hr = pmiptex->GetSurfaceLevel(0, &psurfSrc);
        hr = pmiptexNew->GetSurfaceLevel(0, &psurfDest);
        hr = D3DXLoadSurfaceFromSurface(psurfDest, NULL, NULL, psurfSrc, NULL, NULL, 
            D3DX_DEFAULT, 0);
        ReleasePpo(&psurfSrc);
        ReleasePpo(&psurfDest);
        hr = D3DXFilterTexture(pmiptexNew, NULL, 0, D3DX_DEFAULT);
    }
    else
    {
        if (FAILED(hr = pd3ddev->CreateCubeTexture(m_dwWidth, m_numMips, 
             0, fmt, D3DPOOL_MANAGED, &pcubetexNew, NULL)))
        {
            goto LFail;
        }
        hr = pcubetex->GetCubeMapSurface(D3DCUBEMAP_FACE_POSITIVE_X, 0, &psurfSrc);
        hr = pcubetexNew->GetCubeMapSurface(D3DCUBEMAP_FACE_POSITIVE_X, 0, &psurfDest);
        hr = D3DXLoadSurfaceFromSurface(psurfDest, NULL, NULL, psurfSrc, NULL, NULL, 
            D3DX_DEFAULT, 0);
        ReleasePpo(&psurfSrc);
        ReleasePpo(&psurfDest);
        hr = pcubetex->GetCubeMapSurface(D3DCUBEMAP_FACE_NEGATIVE_X, 0, &psurfSrc);
        hr = pcubetexNew->GetCubeMapSurface(D3DCUBEMAP_FACE_NEGATIVE_X, 0, &psurfDest);
        hr = D3DXLoadSurfaceFromSurface(psurfDest, NULL, NULL, psurfSrc, NULL, NULL, 
            D3DX_DEFAULT, 0);
        ReleasePpo(&psurfSrc);
        ReleasePpo(&psurfDest);
        hr = pcubetex->GetCubeMapSurface(D3DCUBEMAP_FACE_POSITIVE_Y, 0, &psurfSrc);
        hr = pcubetexNew->GetCubeMapSurface(D3DCUBEMAP_FACE_POSITIVE_Y, 0, &psurfDest);
        hr = D3DXLoadSurfaceFromSurface(psurfDest, NULL, NULL, psurfSrc, NULL, NULL, 
            D3DX_DEFAULT, 0);
        ReleasePpo(&psurfSrc);
        ReleasePpo(&psurfDest);
        hr = pcubetex->GetCubeMapSurface(D3DCUBEMAP_FACE_NEGATIVE_Y, 0, &psurfSrc);
        hr = pcubetexNew->GetCubeMapSurface(D3DCUBEMAP_FACE_NEGATIVE_Y, 0, &psurfDest);
        hr = D3DXLoadSurfaceFromSurface(psurfDest, NULL, NULL, psurfSrc, NULL, NULL, 
            D3DX_DEFAULT, 0);
        ReleasePpo(&psurfSrc);
        ReleasePpo(&psurfDest);
        hr = pcubetex->GetCubeMapSurface(D3DCUBEMAP_FACE_POSITIVE_Z, 0, &psurfSrc);
        hr = pcubetexNew->GetCubeMapSurface(D3DCUBEMAP_FACE_POSITIVE_Z, 0, &psurfDest);
        hr = D3DXLoadSurfaceFromSurface(psurfDest, NULL, NULL, psurfSrc, NULL, NULL, 
            D3DX_DEFAULT, 0);
        ReleasePpo(&psurfSrc);
        ReleasePpo(&psurfDest);
        hr = pcubetex->GetCubeMapSurface(D3DCUBEMAP_FACE_NEGATIVE_Z, 0, &psurfSrc);
        hr = pcubetexNew->GetCubeMapSurface(D3DCUBEMAP_FACE_NEGATIVE_Z, 0, &psurfDest);
        hr = D3DXLoadSurfaceFromSurface(psurfDest, NULL, NULL, psurfSrc, NULL, NULL, 
            D3DX_DEFAULT, 0);
        ReleasePpo(&psurfSrc);
        ReleasePpo(&psurfDest);
        hr = D3DXFilterCubeTexture(pcubetexNew, NULL, 0, D3DX_DEFAULT);
    }

    ReleasePpo(&m_ptexOrig);
    if (pvoltexNew != NULL)
        m_ptexOrig = pvoltexNew;
    else if (pcubetexNew != NULL)
        m_ptexOrig = pcubetexNew;
    else
        m_ptexOrig = pmiptexNew;

    if (m_ptexNew != NULL)
    {
        // Rather than filtering down the (probably-compressed) m_ptexNew 
        // top level, compress each mip level from the (probably-uncompressed)
        // m_ptexOrig levels.
        if (pvoltexNew != NULL)
        {
            D3DVOLUME_DESC vd;
            ((LPDIRECT3DVOLUMETEXTURE9)m_ptexNew)->GetLevelDesc(0, &vd);
            fmt = vd.Format;
        }
        else if (pcubetexNew != NULL)
        {
            D3DSURFACE_DESC sd;
            ((LPDIRECT3DTEXTURE9)m_ptexNew)->GetLevelDesc(0, &sd);
            fmt = sd.Format;
        }
        else
        {
            D3DSURFACE_DESC sd;
            ((LPDIRECT3DCUBETEXTURE9)m_ptexNew)->GetLevelDesc(0, &sd);
            fmt = sd.Format;
        }
        Compress(fmt);
    }
   
    return true;

LFail:
    ReleasePpo(&pddsNew);

	return false;
}

DWORD CDxtex::NumMips(void)
{
    return m_numMips;
}

HRESULT CDxtex::LoadAlphaIntoSurface(const char* strPath, LPDIRECT3DSURFACE9 psurf)
{
    HRESULT hr;
    D3DSURFACE_DESC sd;
    LPDIRECT3DDEVICE9 pd3ddev = GetD3DDev();
    LPDIRECT3DTEXTURE9 ptexAlpha;
    LPDIRECT3DSURFACE9 psurfAlpha;
    LPDIRECT3DSURFACE9 psurfTarget;

    psurf->GetDesc(&sd);

    // Load the alpha BMP into psurfAlpha, a new A8R8G8B8 surface
    hr = D3DXCreateTextureFromFileEx(pd3ddev, strPath, sd.Width, sd.Height, 1, 0, 
        D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, D3DX_DEFAULT, 
        D3DX_DEFAULT, 0, NULL, NULL, &ptexAlpha);
    hr = ptexAlpha->GetSurfaceLevel(0, &psurfAlpha);

    // Copy the target surface into an A8R8G8B8 surface
    hr = pd3ddev->CreateOffscreenPlainSurface(sd.Width, sd.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &psurfTarget, NULL);
    hr = D3DXLoadSurfaceFromSurface(psurfTarget, NULL, NULL, psurf, NULL, NULL, 
        D3DX_DEFAULT, 0);

    // Fill in the alpha channels of psurfTarget based on the blue channel of psurfAlpha
    D3DLOCKED_RECT lrSrc;
    D3DLOCKED_RECT lrDest;

    hr = psurfAlpha->LockRect(&lrSrc, NULL, D3DLOCK_READONLY);
    hr = psurfTarget->LockRect(&lrDest, NULL, 0);

    DWORD xp;
    DWORD yp;
    DWORD* pdwRowSrc = (DWORD*)lrSrc.pBits;
    DWORD* pdwRowDest = (DWORD*)lrDest.pBits;
    DWORD* pdwSrc;
    DWORD* pdwDest;
    DWORD dwAlpha;
    LONG dataBytesPerRow = 4 * sd.Width;

    for (yp = 0; yp < sd.Height; yp++)
    {
        pdwSrc = pdwRowSrc;
        pdwDest = pdwRowDest;
        for (xp = 0; xp < sd.Width; xp++)
        {
            dwAlpha = *pdwSrc << 24;
            *pdwDest &= 0x00ffffff;
            *pdwDest |= dwAlpha;

            pdwSrc++;
            pdwDest++;
        }
        pdwRowSrc += lrSrc.Pitch / 4;
        pdwRowDest += lrDest.Pitch / 4;
    }

    psurfAlpha->UnlockRect();
    psurfTarget->UnlockRect();

    // Copy psurfTarget back into real surface
    hr = D3DXLoadSurfaceFromSurface(psurf, NULL, NULL, psurfTarget, NULL, NULL, 
        D3DX_DEFAULT, 0);
    
    // Release allocated interfaces
    ReleasePpo(&psurfTarget);
    ReleasePpo(&psurfAlpha);
    ReleasePpo(&ptexAlpha);

    return S_OK;
}

HRESULT CDxtex::LoadVolumeSliceFromSurface(LPDIRECT3DVOLUME9 pVolume, UINT iSlice, LPDIRECT3DSURFACE9 psurf)
{
    HRESULT hr;
    D3DSURFACE_DESC sd;
    D3DVOLUME_DESC vd;
    D3DLOCKED_RECT lr;
    D3DBOX boxSrc;
    D3DBOX boxDest;

    psurf->GetDesc(&sd);
    pVolume->GetDesc(&vd);

    boxSrc.Left = 0;
    boxSrc.Right = sd.Width;
    boxSrc.Top = 0;
    boxSrc.Bottom = sd.Height;
    boxSrc.Front = 0;
    boxSrc.Back = 1;

    boxDest.Left = 0;
    boxDest.Right = vd.Width;
    boxDest.Top = 0;
    boxDest.Bottom = vd.Height;
    boxDest.Front = iSlice;
    boxDest.Back = iSlice + 1;

    hr = psurf->LockRect(&lr, NULL, 0);
    if (FAILED(hr))
        return hr;

    hr = D3DXLoadVolumeFromMemory(pVolume, NULL, &boxDest, lr.pBits, sd.Format, lr.Pitch, 
        0, NULL, &boxSrc, D3DX_DEFAULT, 0);

    psurf->UnlockRect();

    return hr;
}


HRESULT CDxtex::LoadSurfaceFromVolumeSlice(LPDIRECT3DVOLUME9 pVolume, UINT iSlice, LPDIRECT3DSURFACE9 psurf)
{
    HRESULT hr;
    D3DVOLUME_DESC vd;
    D3DLOCKED_BOX lb;
    D3DBOX box;
    RECT rc;

    pVolume->GetDesc(&vd);

    box.Left = 0;
    box.Right = vd.Width;
    box.Top = 0;
    box.Bottom = vd.Height;
    box.Front = iSlice;
    box.Back = iSlice + 1;

    rc.left = 0;
    rc.right = vd.Width;
    rc.top = 0;
    rc.bottom = vd.Height;

    hr = pVolume->LockBox(&lb, &box, 0);
    if (FAILED(hr))
        return hr;

    hr = D3DXLoadSurfaceFromMemory(psurf, NULL, NULL, lb.pBits, vd.Format, lb.RowPitch, 
        NULL, &rc, D3DX_DEFAULT, 0);

    pVolume->UnlockBox();

    return hr;
}


HRESULT CDxtex::BltAllLevels(D3DCUBEMAP_FACES FaceType, 
    LPDIRECT3DBASETEXTURE9 ptexSrc, LPDIRECT3DBASETEXTURE9 ptexDest)
{
    HRESULT hr;
    LPDIRECT3DTEXTURE9 pmiptexSrc = NULL;
    LPDIRECT3DTEXTURE9 pmiptexDest = NULL;
    LPDIRECT3DCUBETEXTURE9 pcubetexSrc = NULL;
    LPDIRECT3DCUBETEXTURE9 pcubetexDest = NULL;
    LPDIRECT3DVOLUMETEXTURE9 pvoltexSrc = NULL;
    LPDIRECT3DVOLUMETEXTURE9 pvoltexDest = NULL;

    if (IsVolumeMap())
    {
        pvoltexSrc = (LPDIRECT3DVOLUMETEXTURE9)ptexSrc;
        pvoltexDest = (LPDIRECT3DVOLUMETEXTURE9)ptexDest;
    }
    else if (IsCubeMap())
    {
        pcubetexSrc = (LPDIRECT3DCUBETEXTURE9)ptexSrc;
        pcubetexDest = (LPDIRECT3DCUBETEXTURE9)ptexDest;
    }
    else
    {
        pmiptexSrc = (LPDIRECT3DTEXTURE9)ptexSrc;
        pmiptexDest = (LPDIRECT3DTEXTURE9)ptexDest;
    }
	int iOldLevelMax = ptexSrc->GetLevelCount();
	int iLevelMax = ptexDest->GetLevelCount();
	int iOldLevel = iOldLevelMax - iLevelMax;
	int iLevel = 0;
	for (; iLevel < iLevelMax && iOldLevel < iOldLevelMax; iLevel++, iOldLevel++)
    {
        if (IsVolumeMap())
        {
            LPDIRECT3DVOLUME9 pvolSrc = NULL;
            LPDIRECT3DVOLUME9 pvolDest = NULL;
            hr = pvoltexSrc->GetVolumeLevel(iOldLevel, &pvolSrc);
            hr = pvoltexDest->GetVolumeLevel(iLevel, &pvolDest);
            hr = D3DXLoadVolumeFromVolume(pvolDest, NULL, NULL, 
                pvolSrc, NULL, NULL, D3DX_DEFAULT, 0);
            ReleasePpo(&pvolSrc);
            ReleasePpo(&pvolDest);
        }
        else if (IsCubeMap())
        {
			//if (D3DCUBEMAP_FACE_FORCE_DWORD == FaceType)
			//{
			//	for (int i = D3DCUBEMAP_FACE_POSITIVE_X; i <= D3DCUBEMAP_FACE_NEGATIVE_Z; i++)
			//	{
			//		LPDIRECT3DSURFACE9 psurfSrc = NULL;
			//		LPDIRECT3DSURFACE9 psurfDest = NULL;
			//		hr = pcubetexSrc->GetCubeMapSurface((_D3DCUBEMAP_FACES)i, 0, &psurfSrc);
			//		hr = pcubetexDest->GetCubeMapSurface((_D3DCUBEMAP_FACES)i, iLevel, &psurfDest);
			//		hr = D3DXLoadSurfaceFromSurface(psurfDest, NULL, NULL, psurfSrc, NULL, NULL, D3DX_DEFAULT, 0);
			//		ReleasePpo(&psurfSrc);
			//		ReleasePpo(&psurfDest);
			//	}
			//}

			//LPDIRECT3DSURFACE9 psurfSrc = NULL;
			//LPDIRECT3DSURFACE9 psurfDest = NULL;
			//hr = pcubetexSrc->GetCubeMapSurface(FaceType, iOldLevel, &psurfSrc);
			//hr = pcubetexDest->GetCubeMapSurface(FaceType, iLevel, &psurfDest);
			//hr = D3DXLoadSurfaceFromSurface(psurfDest, NULL, NULL, psurfSrc, NULL, NULL, D3DX_DEFAULT, 0);
			//ReleasePpo(&psurfSrc);
			//ReleasePpo(&psurfDest);
			return E_FAIL;
        }
        else
        {
            LPDIRECT3DSURFACE9 psurfSrc = NULL;
            LPDIRECT3DSURFACE9 psurfDest = NULL;
            hr = pmiptexSrc->GetSurfaceLevel(iOldLevel, &psurfSrc);
            hr = pmiptexDest->GetSurfaceLevel(iLevel, &psurfDest);
            hr = D3DXLoadSurfaceFromSurface(psurfDest, NULL, NULL, 
                psurfSrc, NULL, NULL, D3DX_DEFAULT, 0);
            ReleasePpo(&psurfSrc);
            ReleasePpo(&psurfDest);
        }
    }

    return S_OK;
}


HRESULT CDxtex::Resize(DWORD dwWidthNew, DWORD dwHeightNew)
{
    HRESULT hr;
    LPDIRECT3DTEXTURE9 pmiptexNew;
    LPDIRECT3DDEVICE9 pd3ddev = GetD3DDev();
	hr = pd3ddev->CreateTexture(dwWidthNew, dwHeightNew, 0, 0, GetFormat(m_ptexOrig), D3DPOOL_MANAGED, &pmiptexNew, NULL);
    if (FAILED(hr))
        return hr;
	hr = BltAllLevels(D3DCUBEMAP_FACE_FORCE_DWORD, m_ptexOrig, pmiptexNew);
	if (FAILED(hr))
        return hr;
    ReleasePpo(&m_ptexOrig);
    m_ptexOrig = pmiptexNew;

    if( m_ptexNew != NULL )
    {
		hr = pd3ddev->CreateTexture(dwWidthNew, dwHeightNew, 0, 0, GetFormat(m_ptexOrig), D3DPOOL_MANAGED, &pmiptexNew, NULL);
        if (FAILED(hr))
            return hr;
		hr = BltAllLevels(D3DCUBEMAP_FACE_FORCE_DWORD, m_ptexNew, pmiptexNew);
		if (FAILED(hr))
			return hr;
        ReleasePpo(&m_ptexNew);
        m_ptexNew = pmiptexNew;
    }

	m_numMips = pmiptexNew->GetLevelCount();
    m_dwWidth = dwWidthNew;
    m_dwHeight = dwHeightNew;
   
    return S_OK;
}

DWORD CDxtex::DwDepthAt(LONG lwMip)
{
    DWORD dwDepth = m_dwDepth;
    while (lwMip > 0 && dwDepth > 1)
    {
        dwDepth /= 2;
        lwMip--;
    }
    return dwDepth;
}


