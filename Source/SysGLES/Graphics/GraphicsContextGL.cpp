#include "Base/Types.h"

#include <stdio.h>

#include "SysGLES/GL.h"
#include <SDL2/SDL_ttf.h>
#include <iostream>

#include "Graphics/GraphicsContext.h"
#include "SysGLES/Graphics/GraphicsContextGL.h"

#include "Graphics/ColourValue.h"
#include "UI/DrawText.h"

#include "UI/Menu.h"

SDL_Window*     gWindow      = nullptr;
SDL_Renderer*   gSdlRenderer = nullptr;
SDL_GLContext   gContext     = nullptr;

extern void HandleEndOfFrame();



IGraphicsContext::~IGraphicsContext()
{
    // Destroy context and window
    if (gContext)
    {
        SDL_GL_DeleteContext(gContext);
        gContext = nullptr;
    }
    if (gWindow)
    {
        SDL_DestroyWindow(gWindow);
        gWindow = nullptr;
    }

    // Shutdown SDL
    SDL_Quit();
}


extern bool initgl();

bool IGraphicsContext::Initialise()
{
    //Initialize SDL
    if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) < 0 )
    {
        printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
        return false;
    }

    if (TTF_Init() < 0)
    {
        printf( "SDL could not initialize TTF Font! SDL Error: %s\n", SDL_GetError() );
        return false;
    }

    // Request an OpenGL ES 3.1 context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    #ifdef DAEDALUS_DEBUG_CONSOLE
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    #endif

    // 24-bit depth, double buffer
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // Create the window
    gWindow = SDL_CreateWindow( 
        "Daedalus - GLES 3.1", 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        SCREEN_WIDTH, 
        SCREEN_HEIGHT, 
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!gWindow)
    {
        printf("Failed to create SDL Window! SDL Error: %s\n", SDL_GetError());
        return false;
    }

    // Create the GL ES context
    gContext = SDL_GL_CreateContext(gWindow);
    if (!gContext)
    {
        printf("Failed to create GL Context! SDL Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(gWindow);
        gWindow = nullptr;
        return false;
    }

    // Make the context current
    SDL_GL_MakeCurrent(gWindow, gContext);

    // Enable vsync (if desired)
    SDL_GL_SetSwapInterval(1);

    // Now that we have an ES context, initialize your function loader if needed
    // If 'initgl()' is just a stub or calls a custom loader, do it here:
    if (!initgl())
    {
        printf("initgl() reported failure.\n");
        SDL_DestroyWindow(gWindow);
        gWindow = nullptr;
        return false;
    }

    // If you have an extension check or custom setup, do it here.
    // E.g., glGetString(GL_VERSION), etc.
    // printf("GL_VERSION: %s\n", glGetString(GL_VERSION));

    // Initialize text rendering
    CDrawText::Initialise();

    // Clear + swap once
    UpdateFrame(false);

    return true;
}

void IGraphicsContext::UItoGL()
{
    if (gSdlRenderer != nullptr)
    {
        SDL_RenderPresent(gSdlRenderer);
        SDL_RenderFlush(gSdlRenderer);
        SDL_DestroyRenderer(gSdlRenderer);
        gSdlRenderer = nullptr;

        // If you destroyed the window, re-initialize context
        if (gWindow)
        {
            SDL_DestroyWindow(gWindow);
            SDL_GL_DeleteContext(gContext);
            gContext = nullptr;
            gWindow = nullptr;
        }
        IGraphicsContext::Initialise();
    }
}

void IGraphicsContext::GetScreenSize(u32* width, u32* height) const
{
    int window_width  = 0;
    int window_height = 0;
    SDL_GL_GetDrawableSize(gWindow, &window_width, &window_height);

    *width  = static_cast<u32>(window_width);
    *height = static_cast<u32>(window_height);
}

void IGraphicsContext::ViewportType(u32* width, u32* height) const
{
    GetScreenSize(width, height);
}

void IGraphicsContext::ClearAllSurfaces()
{
    // FIXME: this should clear/flip a couple of times to ensure 
    // the front and backbuffers are cleared. 
    // Not sure if it's necessary...
    IGraphicsContext::UItoGL();
    ClearToBlack();
}

void IGraphicsContext::ClearToBlack()
{
    glDepthMask(GL_TRUE);
    glClearDepthf(1.0f);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void IGraphicsContext::ClearZBuffer()
{
    glDepthMask(GL_TRUE);
    glClearDepthf(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void IGraphicsContext::ClearColBuffer(const c32 & colour)
{
    glClearColor(colour.GetRf(), colour.GetGf(), colour.GetBf(), colour.GetAf());
    glClear(GL_COLOR_BUFFER_BIT);
}

void IGraphicsContext::ClearColBufferAndDepth(const c32 & colour)
{
    glDepthMask(GL_TRUE);
    glClearDepthf(1.0f);
    glClearColor(colour.GetRf(), colour.GetGf(), colour.GetBf(), colour.GetAf());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void IGraphicsContext::BeginFrame()
{
    // Get window size (may be different than the requested size)
    u32 width, height;
    GetScreenSize(&width, &height);

   	// Special case: avoid division by zero below
	height = height > 0 ? height : 1;

    glViewport(0, 0, width, height);
    glScissor(0, 0, width, height);
}

void IGraphicsContext::EndFrame()
{
    HandleEndOfFrame();
}

void IGraphicsContext::UpdateFrame(bool wait_for_vbl [[maybe_unused]])
{
    // Bind default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // If no SDL renderer is active, swap the window
    if (!gSdlRenderer)
    {
        SDL_GL_SwapWindow(gWindow);
    }
    // else: If gSdlRenderer != nullptr, you’re likely rendering via SDL2, not GL.
}

