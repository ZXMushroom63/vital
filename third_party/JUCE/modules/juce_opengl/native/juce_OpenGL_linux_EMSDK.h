/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 6 End-User License
   Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).

   End User License Agreement: www.juce.com/juce-6-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

namespace juce
{
#define GLES_DEBUG(dbgtext)   { juce::String tempDbgBuf ("GLES: "); tempDbgBuf << dbgtext; Logger::writeToLog (tempDbgBuf); DBG (tempDbgBuf); }
extern XContext windowHandleXContext;

//=========================================================================
//==============================================================================
class OpenGLContext::NativeContext
{
private:
    struct DummyComponent  : public Component
    {
        DummyComponent (OpenGLContext::NativeContext& nativeParentContext)
            : native (nativeParentContext)
        {
        }

        void handleCommandMessage (int commandId) override
        {
            if (commandId == 0)
                native.triggerRepaint();
        }

        OpenGLContext::NativeContext& native;
    };

public:
    NativeContext (Component& comp,
                   const OpenGLPixelFormat& cPixelFormat,
                   void* shareContext,
                   bool /*useMultisampling*/,
                   OpenGLVersion)
        : component (comp), contextToShareWith (shareContext), dummy (*this)
    {
        GLES_DEBUG("Creating GLES context.");
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

        if (display == EGL_NO_DISPLAY) {
            GLES_DEBUG("No display found, exiting...");
            return;
        }

        if (! eglInitialize(display, nullptr, nullptr)) {
            GLES_DEBUG("Failed to init display");
            return;
        }

        const EGLint attribs[] = {
            EGL_RED_SIZE,       cPixelFormat.redBits,
            EGL_GREEN_SIZE,     cPixelFormat.greenBits,
            EGL_BLUE_SIZE,      cPixelFormat.blueBits,
            EGL_ALPHA_SIZE,     cPixelFormat.alphaBits,
            EGL_DEPTH_SIZE,     cPixelFormat.depthBufferBits,
            EGL_STENCIL_SIZE,   cPixelFormat.stencilBufferBits,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT | EGL_OPENGL_ES3_BIT, // Target ES2 or ES3 (WebGL 1/2)
            EGL_NONE
        };

        EGLint numConfigs;
        if (! eglChooseConfig(display, attribs, &config, 1, &numConfigs) || numConfigs == 0)
        {
            GLES_DEBUG("Using simple fallback config");
            const EGLint fallbackAttribs[] = { EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE };
            eglChooseConfig(display, fallbackAttribs, &config, 1, &numConfigs);
        }

        // embeddedWindow = X11Symbols::getInstance()->xCreateWindow (display, windowH,
        //                                                            glBounds.getX(), glBounds.getY(),
        //                                                            (unsigned int) jmax (1, glBounds.getWidth()),
        //                                                            (unsigned int) jmax (1, glBounds.getHeight()),
        //                                                            0, bestVisual->depth,
        //                                                            InputOutput,
        //                                                            bestVisual->visual,
        //                                                            CWBorderPixel | CWColormap | CWEventMask,
        //                                                            &swa);

        // X11Symbols::getInstance()->xSaveContext (display, (XID) embeddedWindow, windowHandleXContext, (XPointer) peer);

        // X11Symbols::getInstance()->xMapWindow (display, embeddedWindow);
        // X11Symbols::getInstance()->xFreeColormap (display, colourMap);

        // X11Symbols::getInstance()->xSync (display, False);

        // juce_LinuxAddRepaintListener (peer, &dummy);
        GLES_DEBUG("assigned _ctx");
        _ctx = this;
    }

    ~NativeContext()
    {
        // none
    }

    bool initialiseOnRenderThread (OpenGLContext& c)
    {
        GLES_DEBUG("FINALLY! It has initialised.");;
        if (display == EGL_NO_DISPLAY) {
            GLES_DEBUG("Attempted init with no display");
            return false;
        }

        EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
        context = eglCreateContext (display, config, (EGLContext)contextToShareWith, contextAttribs);
        if (context == EGL_NO_CONTEXT)
        {
            // webgl 1 fallback
            contextAttribs[1] = 2;
            context = eglCreateContext (display, config, (EGLContext)contextToShareWith, contextAttribs);
            GLES_DEBUG("Fallback occured. If context creation fails, the render loop will crash.");
        }

        if (context == EGL_NO_CONTEXT)
            GLES_DEBUG("Context creation failed.");
            return false;

        surface = eglCreateWindowSurface(
            display,
            config,
            (EGLNativeWindowType)0, //find default canvas in DOM
            nullptr
        );

        if (surface == EGL_NO_SURFACE)
            GLES_DEBUG("Target <canvas> not found .");
            return false;

        if (!makeActive())
            GLES_DEBUG("Failed to activate context.");
            return false;


        c.makeActive();
        openglContext = &c;
        GLES_DEBUG("Successfully created opengl context!");
        return true;
    }

    void shutdownOnRenderThread()
    {
        GLES_DEBUG("Shutting down renderer.");
        openglContext = nullptr;
        deactivateCurrentContext();
        

        if (surface != EGL_NO_SURFACE) {
            eglDestroySurface(display, surface);
            surface = EGL_NO_SURFACE;
        }

        if (context != EGL_NO_CONTEXT) {
            eglDestroyContext(display, context);
            context = EGL_NO_CONTEXT;
        }

        eglTerminate(display);
        display = EGL_NO_DISPLAY;
    }

    bool makeActive() const noexcept
    {
        if (context != EGL_NO_CONTEXT && surface != EGL_NO_SURFACE)
            return eglMakeCurrent(display, surface, surface, context);

        return false;
    }

    bool isActive() const noexcept
    {
        return eglGetCurrentContext() == context && context != EGL_NO_CONTEXT;
    }

    static void deactivateCurrentContext()
    {
        eglMakeCurrent (eglGetDisplay(EGL_DEFAULT_DISPLAY), EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }

    void swapBuffers()
    {
        GLES_DEBUG("swapping buffers!");
        if (display != EGL_NO_DISPLAY && surface != EGL_NO_SURFACE)
            eglSwapBuffers (display, surface);
    }

    void updateWindowPosition (Rectangle<int> newBounds)
    {
        bounds = newBounds; //.translated(0, 48); arbitrary fix for KDE on nixos
        emscripten_set_canvas_element_size("#canvas", newBounds.getWidth(), newBounds.getHeight());
    }

    bool setSwapInterval (int numFramesPerSwap)
    {
        if (numFramesPerSwap == swapFrames)
            return true;

        if (display != EGL_NO_DISPLAY)
            swapFrames = numFramesPerSwap;
            eglSwapInterval(display, swapFrames);
            return true;

        return false;
    }

    int getSwapInterval() const                 { return swapFrames; }
    bool createdOk() const noexcept             { return true; }
    void* getRawContext() const noexcept        { return (void*)context; }
    GLuint getFrameBufferID() const noexcept    { return 0; }

    void triggerRepaint()
    {
        if (openglContext != nullptr) {
            openglContext->triggerRepaint();
        }
    }

    struct Locker { Locker (NativeContext&) {} };
    static OpenGLContext::NativeContext* _ctx;
    OpenGLContext* openglContext = nullptr;

private:
    Component& component;
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLConfig config  = nullptr;

    int swapFrames = 1;
    Rectangle<int> bounds;
    void* contextToShareWith;

    DummyComponent dummy;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NativeContext)
};

//==============================================================================
bool OpenGLHelpers::isContextActive()
{
    return eglGetCurrentContext() != EGL_NO_CONTEXT;
}

extern "C" {
    OpenGLContext::NativeContext* OpenGLContext::NativeContext::_ctx = nullptr;

    EMSCRIPTEN_KEEPALIVE
    void testNewBounds() {
        OpenGLContext::NativeContext* ctx = OpenGLContext::NativeContext::_ctx;
        if (ctx != nullptr) {
            ctx->updateWindowPosition(Rectangle<int>(0, 0,
               400, 300));
        }
    }

    EMSCRIPTEN_KEEPALIVE
    void testRepaint() {
        OpenGLContext::NativeContext* ctx = OpenGLContext::NativeContext::_ctx;
        if (ctx != nullptr) {
            GLES_DEBUG("forcing repaint.");
            ctx->triggerRepaint();
        }
    }
}

} // namespace juce