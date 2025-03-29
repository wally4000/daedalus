#pragma once

class IGraphicsContext : public CGraphicsContext
{
public:
    virtual ~GraphicsContext();

    virtual bool Initialise();
    virtual bool IsInitialised() const { return true; }

    virtual void ClearAllSurfaces();
    virtual void ClearZBuffer();
    virtual void ClearColBuffer(const c32 & colour);
    virtual void ClearToBlack();
    virtual void ClearColBufferAndDepth(const c32 & colour);
    virtual void BeginFrame();
    virtual void EndFrame();
    virtual void UpdateFrame( bool wait_for_vbl );

    virtual void GetScreenSize(u32 * width, u32 * height) const;
    virtual void ViewportType(u32 * width, u32 * height) const;

    virtual void SetDebugScreenTarget( ETargetSurface buffer [[maybe_unused]] ) {}
    virtual void DumpNextScreen() {}
    virtual void DumpScreenShot() {}
    virtual void UItoGL();
};