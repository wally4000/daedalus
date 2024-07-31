

#include "Base/Types.h"



#include <GL/glew.h>

#include "SysGL/GL.h"
#include <SDL2/SDL_ttf.h>

#include <iostream>

#include "Graphics/GraphicsContext.h"

#include "Graphics/ColourValue.h"
#include "UI/DrawText.h"

#include "UI/Menu.h"

SDL_Window * gWindow = nullptr;
SDL_Renderer * gSdlRenderer = nullptr;
SDL_GLContext gContext = nullptr;
// u32 mDumpNextScreen = false;
extern bool gTakeScreenshotSS;
extern void HandleEndOfFrame();



class GraphicsContextGL : public CGraphicsContext
{
public:
	virtual ~GraphicsContextGL();


	virtual bool Initialise();
	virtual bool IsInitialised() const { return true; }

	virtual void ClearAllSurfaces();
	virtual void ClearZBuffer();
	virtual void ClearColBuffer(const c32 & colour);
	virtual void ClearToBlack();
	virtual void ClearColBufferAndDepth(const c32 & colour);
	virtual	void BeginFrame();
	virtual void EndFrame();
	virtual void UpdateFrame( bool wait_for_vbl );

	virtual void GetScreenSize(u32 * width, u32 * height) const;
	virtual void ViewportType(u32 * width, u32 * height) const;

	virtual void SetDebugScreenTarget( ETargetSurface buffer [[maybe_unused]] ) {}
	void DumpNextScreen() { mDumpNextScreen = 2; }
	virtual void DumpScreenShot();

	virtual void UItoGL();

private:
	virtual void 		SaveScreenshot (const std::filesystem::path filename, s32 x, s32 y, u32 width, u32 height);

	u32 mDumpNextScreen = false;
};

template<> bool CSingleton< CGraphicsContext >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == nullptr);

	mpInstance = std::make_shared<GraphicsContextGL>();
	return mpInstance->Initialise();
}


GraphicsContextGL::~GraphicsContextGL()
{
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;
	SDL_Quit();
}

extern bool initgl();
bool GraphicsContextGL::Initialise()
{

	//Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER ) < 0 )
	{
		printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
		return false;
	}

	if (TTF_Init() < 0)
	{
		printf( "SDL could not initialize TTF Font! SDL Error: %s\n", SDL_GetError() );
		return false;
	}


    // Decide GL+GLSL versions

#if defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#else
       // Simplified context creation
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);


	//Create window
	gWindow = SDL_CreateWindow( "Daedalus", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

	//Create context
	gContext = SDL_GL_CreateContext( gWindow );

	SDL_GL_MakeCurrent(gWindow, gContext);

	SDL_GL_SetSwapInterval(1);

	GLenum err = glewInit();
	if (err != GLEW_OK || !GLEW_VERSION_3_2)
	{
		SDL_DestroyWindow(gWindow);
		gWindow = NULL;
		//SDL_Quit();
		return false;
	}

	CDrawText::Initialise();

	//ClearColBufferAndDepth(0,0,0,0);
	UpdateFrame(false);
	return initgl();
}

void GraphicsContextGL::UItoGL(){
// 	if(gSdlRenderer != nullptr){
// 				// SDL_GL_SwapWindow(gWindow);
// 		SDL_DestroyRenderer(gSdlRenderer);
// 		// SDL_RenderClear(gSdlRenderer);
// 	// SDL_RenderFlush(gSdlRenderer);
// 		UpdateFrame(false);
// 	// SDL_RenderPresent(gSdlRenderer);

// 	}

// 	// SDL_DestroyRenderer(gSdlRenderer);
// 	// gSdlRenderer = nullptr;
// 	// SDL_DestroyWindow(gWindow);
// 	// gWindow = NULL;
// 	// GraphicsContextGL::Initialise();
	SDL_SetRenderDrawColor(gSdlRenderer, 0, 0, 0, 255);
        SDL_RenderClear(gSdlRenderer);
		SDL_GL_MakeCurrent(gWindow, gContext);
		SDL_GL_SwapWindow(gWindow);
}

void GraphicsContextGL::GetScreenSize(u32 * width, u32 * height) const
{
	int window_width, window_height;
	SDL_GL_GetDrawableSize(gWindow, &window_width, &window_height);

	*width  = window_width;
	*height = window_height;
}

void GraphicsContextGL::ViewportType(u32 * width, u32 * height) const
{
	GetScreenSize(width, height);
}

void GraphicsContextGL::ClearAllSurfaces()
{
	// FIXME: this should clear/flip a couple of times to ensure the front and backbuffers are cleared.
	// Not sure if it's necessary...
	GraphicsContextGL::UItoGL();
	ClearToBlack();
}

void GraphicsContextGL::ClearToBlack()
{
	glDepthMask(GL_TRUE);
	glClearDepth( 1.0f );
	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

void GraphicsContextGL::ClearZBuffer()
{
	glDepthMask(GL_TRUE);
	glClearDepth( 1.0f );
	glClear( GL_DEPTH_BUFFER_BIT );
}

void GraphicsContextGL::ClearColBuffer(const c32 & colour)
{
	glClearColor( colour.GetRf(), colour.GetGf(), colour.GetBf(), colour.GetAf() );
	glClear( GL_COLOR_BUFFER_BIT );
}

void GraphicsContextGL::ClearColBufferAndDepth(const c32 & colour)
{
	glDepthMask(GL_TRUE);
	glClearDepth( 1.0f );
	glClearColor( colour.GetRf(), colour.GetGf(), colour.GetBf(), colour.GetAf() );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

void GraphicsContextGL::BeginFrame()
{
	// Get window size (may be different than the requested size)
	u32 width, height;
	GetScreenSize(&width, &height);

	// Special case: avoid division by zero below
	height = height > 0 ? height : 1;

	glViewport( 0, 0, width, height );
	glScissor( 0, 0, width, height );
}

void GraphicsContextGL::EndFrame()
{
	HandleEndOfFrame();
}

void GraphicsContextGL::UpdateFrame( bool wait_for_vbl [[maybe_unused]] )
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (gSdlRenderer == nullptr) {
		SDL_GL_SwapWindow(gWindow);
	}
	
//	if( gCleanSceneEnabled ) //TODO: This should be optional
//	{
	//	ClearColBuffer( c32(0xff000000) ); // ToDo : Use gFillColor instead?
	//}
	if( mDumpNextScreen )
	{
		mDumpNextScreen--;
		if (!mDumpNextScreen)
		{
			if(gTakeScreenshotSS)	// We are taking a screenshot for savestate
			{
				gTakeScreenshotSS = false;
				StoreSaveScreenData();

			}
			else
			{
				DumpScreenShot();
			}
		}
	}

}


void GraphicsContextGL::SaveScreenshot( const std::filesystem::path filename, s32 x, s32 y, u32 width, u32 height)
{
    std::cout << "Saving Screenshot" << std::endl;


    SDL_Surface* sshot = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (!sshot)
    {
        std::cerr << "Error: Unable to create SDL surface: " << SDL_GetError() << std::endl;
        return;
    }

    std::cout << "Reading Pixels" << std::endl;
    glReadPixels(x, SCREEN_HEIGHT - y - height, width, height, GL_RGBA, GL_UNSIGNED_BYTE, sshot->pixels);

    std::cout << "Saving BMP" << std::endl;
    if (SDL_SaveBMP(sshot, filename.string().c_str()) != 0)
    {
        std::cerr << "Error: Unable to save BMP: " << SDL_GetError() << std::endl;
    }

    SDL_FreeSurface(sshot);
}

void GraphicsContextGL::DumpScreenShot()
{
	std::cout << "Taking Screenshot" << std::endl;
	std::filesystem::path filename = "hello.bmp";

	SaveScreenshot(filename, 1920/2, 1080/2, 1920, 1080 );
}