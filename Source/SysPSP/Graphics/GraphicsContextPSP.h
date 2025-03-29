#pragma once

#include "Graphics/GraphicsContext.h"
class IGraphicsContext : public CGraphicsContext
{
public:
	IGraphicsContext();
	virtual ~IGraphicsContext();

	bool				Initialise();
	bool				IsInitialised() const { return mInitialised; }

	void				SwitchToChosenDisplay();
	void				SwitchToLcdDisplay();
	void				StoreSaveScreenData();

	void				ClearAllSurfaces();

	void				ClearToBlack();
	void				ClearZBuffer();
	void				ClearColBuffer(const c32 & colour);
	void				ClearColBufferAndDepth(const c32 & colour);


	void				BeginFrame();
	void				EndFrame();
	void				UpdateFrame( bool wait_for_vbl );
	void				GetScreenSize( u32 * width, u32 * height ) const;

	void				SetDebugScreenTarget( ETargetSurface buffer );

	void				ViewportType( u32 * d_width, u32 * d_height ) const;
	void				DumpScreenShot();
	void				DumpNextScreen()			{ mDumpNextScreen = 2; }
	void 				CleanupDisplayLists();
private:
	void				SaveScreenshot( const std::filesystem::path filename, s32 x, s32 y, u32 width, u32 height );

private:
	bool				mInitialised;

	void *				mpBuffers[2];
	void *				mpCurrentBackBuffer;

	void *				save_disp_rel;
	void *				save_draw_rel;
	void *				save_depth_rel;

	u32					mDumpNextScreen;

};