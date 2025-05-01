#pragma once
#include <d3dx9.h>
#include "../other/color.h"
#include "../other/maths.h"

/*
* thanks nvidia for ready-to-use solution!
* thanks to flow hooks gui for paste ready code.
* im using this instead of my original method is because my original method
* doesn't work within rendering test environment, only works in game environment that injects into the game.
* credit: https://download.nvidia.com/developer/GPU_Gems/CD_Image/Natural_Effects/Water_Simulation/d3dfont.cpp
* credit: https://github.com/felipeczpaz/FlowHooks-GUI/blob/main/Render/D3DFont.cpp
*/

#define SAFE_RELEASE(pointer)	{ if(pointer) { (pointer)->Release(); (pointer) = nullptr; } }
#define SAFE_DELETE(pointer)	{ if(pointer) { delete (pointer); (pointer) = nullptr; } }

// font creation flags.
#define D3DFONT_ITALIC      (1 << 0)
#define D3DFONT_ZENABLE     (1 << 1)

// font rendering flags.
enum font_flags
{
    CD3DFONT_CENTERED_X = (1 << 0),
    CD3DFONT_CENTERED_Y = (1 << 1),
    CD3DFONT_TWOSIDED   = (1 << 2),
    CD3DFONT_FILTERED   = (1 << 3),
    CD3DFONT_DROPSHADOW = (1 << 4)
};




//-----------------------------------------------------------------------------
// Name: class CD3DFont
// Desc: Texture-based font class for doing text in a 3D scene.
//-----------------------------------------------------------------------------
class environment_font
{
    TCHAR   strFontName[80];            // Font properties
    DWORD   dwFontHeight;
    DWORD   dwFontFlags;
    DWORD   dwFontWeight;

    LPDIRECT3DDEVICE9       pd3dDevice; // A D3DDevice used for rendering
    LPDIRECT3DTEXTURE9      pTexture;   // The d3d texture for this font
    LPDIRECT3DVERTEXBUFFER9 pVB;        // VertexBuffer for rendering text
    DWORD   dwTexWidth;                 // Texture dimensions
    DWORD   dwTexHeight;
    FLOAT   fTextScale;
    FLOAT   fTexCoords[128 - 32][4];
    DWORD   dwSpacing;                  // Character pixel spacing per side

    // Stateblocks for setting and restoring render states
    LPDIRECT3DSTATEBLOCK9 pStateBlockSaved;
    LPDIRECT3DSTATEBLOCK9 pStateBlockDrawString;

public:
    // 2D text drawing functions
    HRESULT text(int x, int y, const char* strText, color dwColor, DWORD dwFlags = 0L);
    HRESULT text(FLOAT x, FLOAT y, const char* strText, color dwColor, DWORD dwFlags = 0L);
    HRESULT text_scaled(FLOAT x, FLOAT y, FLOAT fXScale, FLOAT fYScale, const char* strText, color dwColor, DWORD dwFlags = 0L);

    // text size function.
    dimension text_size(const char* text);

    // Function to get extent of text
    HRESULT GetTextExtent(const char* strText, SIZE* pSize);

    // Initializing and destroying device-dependent objects
    HRESULT setup_device_objects(LPDIRECT3DDEVICE9 pd3dDevice);
    HRESULT restore_device_objects();
    HRESULT invalidate_device_objects();
    HRESULT delete_device_objects();

    // Constructor / destructor
    environment_font(const TCHAR* strFontName, DWORD dwHeight, DWORD dwWeight, DWORD dwFlags = 0L);
    ~environment_font();
};