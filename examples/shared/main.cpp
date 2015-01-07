/*-----------------------------------------------------------------------
    Copyright (c) 2013, Tristan Lorach. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Neither the name of its contributors may be used to endorse 
       or promote products derived from this software without specific
       prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    feedback to lorachnroll@gmail.com (Tristan Lorach)
*/ //--------------------------------------------------------------------

#if defined(__APPLE__)
    #include <GLUT/glut.h>
#else
    #include <stdio.h>
    #include <stdlib.h>
    #include <GL/glew.h>
#ifdef WIN32
    #include <GL/wglew.h>
#   ifdef MEMORY_LEAKS_CHECK
#   pragma message("build will Check for Memory Leaks!")
#   define _CRTDBG_MAP_ALLOC
#   include <stdlib.h>
#   include <crtdbg.h>
#   endif
#else
    #include <GL/glxew.h>
#endif
#   include <vector>
#ifndef NOGLUT
    #include <GL/glut.h>
#else
    #include <windows.h>
    #include <commdlg.h>
    #include "resources.h"
    // Mouse wheel support
    #if !defined WM_MOUSEWHEEL
    #   define   WM_MOUSEWHEEL 0x020A
    #endif    // WM_MOUSEWHEEL
    #if !defined WHEEL_DELTA
    #define      WHEEL_DELTA 120
    #endif    // WHEEL_DELTA
#endif
#endif
//-------------- Optional User interface
#ifdef USESVCUI
#include "ISvcUI.h"
#endif
//-------------- UI
#ifdef USESVCUI
nvSvc::ISvcFactory* g_pFactUI       = NULL;
IWindowHandler *    g_pWinHandler   = NULL;
IWindowConsole*     g_pConsole      = NULL;
IWindowLog*         g_pLog          = NULL;
IProgressBar*       g_pProgress     = NULL;
#endif
namespace main
{
    void logMessage(int level, const char * fmt, ...);
}
#   define  LOGI(...)  { main::logMessage(0, __VA_ARGS__); }
#   define  LOGW(...)  { main::logMessage(1, __VA_ARGS__); }
#   define  LOGE(...)  { main::logMessage(2, __VA_ARGS__); }
#   define  LOGOK(...)  { main::logMessage(7, __VA_ARGS__); }
//-------------- 
namespace main
{
int     g_winSz[2] = {1000, 800};
#ifdef NOGLUT
HDC         g_hDC       = NULL;
HGLRC       g_hRC       = NULL;
HWND        g_hWnd      = NULL;
HINSTANCE   g_hInstance = NULL;
static int  g_curX      = 0;
static int  g_curY      = 0;
#endif
bool        g_bCtrl     = false;
bool        g_bShift    = false;
int         g_renderCnt     = 0;
//------------------------------------------------------------------------------
extern int  g_winSz[2];
extern void reshape(int w, int h);
extern void display();
extern void motion(int x, int y);
extern void mousewheel(short button);
extern void mouse(int button, int state, int x, int y);
extern void keyboard(unsigned char key, int x, int y);
extern void specialkeys(int key, int x, int y);
extern void idle();
extern void initGL(int argc, const char ** argv);
extern void menu(int m);
extern void shutdown();

//------------------------------------------------------------------------------
const char* openAFile(char *filters)
{
    static char fname[300];
    fname[0] = '\0';
    OPENFILENAME ofn = {
        sizeof(OPENFILENAMEA),//  DWORD         lStructSize;
        g_hWnd,//  HWND          hwndOwner;
        g_hInstance,//  HINSTANCE     hInstance;
        filters,//  LPCTSTR       lpstrFilter;
        NULL,//  LPTSTR        lpstrCustomFilter;
        0,//  DWORD         nMaxCustFilter;
        0,//  DWORD         nFilterIndex;
        fname,//  LPTSTR        lpstrFile;
        300-1,//  DWORD         nMaxFile;
        NULL,//  LPTSTR        lpstrFileTitle;
        0,//  DWORD         nMaxFileTitle;
        NULL,//  LPCTSTR       lpstrInitialDir;
        "Load",//  LPCTSTR       lpstrTitle;
        OFN_ENABLESIZING,//  DWORD         Flags;
        0,//  WORD          nFileOffset;
        0,//  WORD          nFileExtension;
        NULL,//  LPCTSTR       lpstrDefExt;
        NULL,//  LPARAM        lCustData;
        NULL,//  LPOFNHOOKPROC lpfnHook;
        NULL,//  LPCTSTR       lpTemplateName;
    #if (_WIN32_WINNT >= 0x0500)
        NULL,//  void          *pvReserved;
        0,//  DWORD         dwReserved;
        0,//  DWORD         FlagsEx;
    #endif 
    };
    if(GetOpenFileName(&ofn) )
        return fname;
    else
        return NULL;
}

const char* saveFile(char* name, char *filters)
{
    static char fname[300];
    strncpy(fname, name, 300-1);
    fname[0] = '\0';
    OPENFILENAME ofn = {
        sizeof(OPENFILENAMEA),//  DWORD         lStructSize;
        g_hWnd,//  HWND          hwndOwner;
        g_hInstance,//  HINSTANCE     hInstance;
        filters,//  LPCTSTR       lpstrFilter;
        NULL,//  LPTSTR        lpstrCustomFilter;
        0,//  DWORD         nMaxCustFilter;
        0,//  DWORD         nFilterIndex;
        fname,//  LPTSTR        lpstrFile;
        300-1,//  DWORD         nMaxFile;
        NULL,//  LPTSTR        lpstrFileTitle;
        0,//  DWORD         nMaxFileTitle;
        NULL,//  LPCTSTR       lpstrInitialDir;
        "Save",//  LPCTSTR       lpstrTitle;
        OFN_ENABLESIZING,//  DWORD         Flags;
        0,//  WORD          nFileOffset;
        0,//  WORD          nFileExtension;
        NULL,//  LPCTSTR       lpstrDefExt;
        NULL,//  LPARAM        lCustData;
        NULL,//  LPOFNHOOKPROC lpfnHook;
        NULL,//  LPCTSTR       lpTemplateName;
    #if (_WIN32_WINNT >= 0x0500)
        NULL,//  void          *pvReserved;
        0,//  DWORD         dwReserved;
        0,//  DWORD         FlagsEx;
    #endif 
    };
    if(GetSaveFileName(&ofn) )
        return fname;
    else
        return NULL;
}
//------------------------------------------------------------------------------
void shutdownBase()
{
#ifdef USESVCUI
    g_pConsole = NULL;
    g_pLog = NULL;
    // BUG, HERE:
    //if(g_pWinHandler) g_pWinHandler->DestroyAll();
    //UISERVICE_UNLOAD(g_pFactUI, g_pWinHandler);
#endif
#ifdef SVCSPACEMOUSE
    if(g_pFactMouse)    
        FACTORY_UNLOAD(g_pFactMouse, "nv_SvcSpaceMouse.dll");
#endif
}
//------------------------------------------------------------------------------
// Setup the base layout of the UI
// the rest can be done outside, depending on the sample's needs
#ifdef USESVCUI
void initUIBase()
{
    UISERVICE_LOAD(g_pFactUI, g_pWinHandler);
    if(g_pWinHandler)
    {
        // a Log window is a line-by-line logging, with possible icons for message levels
        g_pLog= g_pWinHandler->CreateWindowLog("LOG", "Log");
        g_pLog->SetVisible()->SetLocation(0,g_winSz[1]+32)->SetSize(g_winSz[0],200);
        // Console is a window in which you can write and capture characters the user typed...
        //g_pConsole = g_pWinHandler->CreateWindowConsole("CONSOLE", "Console");
        //g_pConsole->SetVisible();//->SetLocation(0,g_winSz[1]+32)->SetSize(g_winSz[0],200);
        // Show and update this control when doing long load/computation... for example
        g_pProgress = g_pWinHandler->CreateWindowProgressBar("PROG", "Loading", NULL);
        g_pProgress->SetVisible(0);
    }
}
#elif defined(USEANTTWEAKBAR)
void initUIBase()
{
}
#endif
//------------------------------------------------------------------------------
#ifdef _DEBUG
void APIENTRY myOpenGLCallback(  GLenum source,
                        GLenum type,
                        GLuint id,
                        GLenum severity,
                        GLsizei length,
                        const GLchar* message,
                        GLvoid* userParam)
{
    //static std::map<GLuint, bool> ignoreMap;
    //if(ignoreMap[id] == true)
    //    return;
    char *strSource = "0";
    char *strType = strSource;
    char *strSeverity = strSource;
    switch(source)
    {
    case GL_DEBUG_SOURCE_API_ARB:
        strSource = "API";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
        strSource = "WINDOWS";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
        strSource = "SHADER COMP.";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
        strSource = "3RD PARTY";
        break;
    case GL_DEBUG_SOURCE_APPLICATION_ARB:
        strSource = "APP";
        break;
    case GL_DEBUG_SOURCE_OTHER_ARB:
        strSource = "OTHER";
        break;
    }
    switch(type)
    {
    case GL_DEBUG_TYPE_ERROR_ARB:
        strType = "ERROR";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
        strType = "Deprecated";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
        strType = "Undefined";
        break;
    case GL_DEBUG_TYPE_PORTABILITY_ARB:
        strType = "Portability";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE_ARB:
        strType = "Performance";
        break;
    case GL_DEBUG_TYPE_OTHER_ARB:
        strType = "Other";
        break;
    }
    switch(severity)
    {
    case GL_DEBUG_SEVERITY_HIGH_ARB:
        strSeverity = "High";
        LOGE("ARB_debug : %s - %s - %s : %s\n",strSeverity, strSource, strType, message);
        break;
    case GL_DEBUG_SEVERITY_MEDIUM_ARB:
        strSeverity = "Medium";
        LOGW("ARB_debug : %s - %s - %s : %s\n",strSeverity, strSource, strType, message);
        break;
    case GL_DEBUG_SEVERITY_LOW_ARB:
        strSeverity = "Low";
        LOGI("ARB_debug : %s - %s - %s : %s\n",strSeverity, strSource, strType, message);
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        strSeverity = "DEBUG";
        //LOGI("ARB_debug : %s - %s - %s : %s\n",strSeverity, strSource, strType, message);
        break;
    }
}
#endif
//------------------------------------------------------------------------------
#ifdef NOGLUT
bool initGLBase()
{
    GLuint PixelFormat;
    
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

    pfd.nSize      = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion   = 1;
    pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    
    g_hDC = GetDC( g_hWnd );
    PixelFormat = ChoosePixelFormat( g_hDC, &pfd );
    SetPixelFormat( g_hDC, PixelFormat, &pfd);
    g_hRC = wglCreateContext( g_hDC );
    wglMakeCurrent( g_hDC, g_hRC );
    // calling glewinit NOW because the inside glew, there is mistake to fix...
    // This is the joy of using Core. The query glGetString(GL_EXTENSIONS) is deprecated from the Core profile.
    // You need to use glGetStringi(GL_EXTENSIONS, <index>) instead. Sounds like a "bug" in GLEW.
    glewInit();
    //wglSwapInterval(0);
#if 1
#define GLCOMPAT
    if(!wglCreateContextAttribsARB)
        wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    if(wglCreateContextAttribsARB)
    {
        HGLRC hRC = NULL;
        int attribList[] =
        {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
#ifdef GLCOMPAT
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
#else
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#endif
            WGL_CONTEXT_FLAGS_ARB,
            //WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB|
#ifndef GLCOMPAT
            //WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB| // use it if you want errors when compat options still used
#endif
#ifdef _DEBUG
            WGL_CONTEXT_DEBUG_BIT_ARB
#else
            0
#endif
            ,0, 0
        };
        if (!(hRC = wglCreateContextAttribsARB(g_hDC, 0, attribList)))
        {
            LOGE("wglCreateContextAttribsARB() failed for OpenGL context.\n");
            return false;
            //attribList[3] = 0;
            //if (!(hRC = wglCreateContextAttribsARB(g_hDC, 0, attribList)))
            //    LOGE("wglCreateContextAttribsARB() failed for OpenGL context.\n");
        }
        if (!wglMakeCurrent(g_hDC, hRC))
        { LOGE("wglMakeCurrent() failed for OpenGL context.\n"); }
        else {
            wglDeleteContext( g_hRC );
            g_hRC = hRC;
#ifdef _DEBUG
            if(!glDebugMessageCallbackARB)
            {
                glDebugMessageCallbackARB = (PFNGLDEBUGMESSAGECALLBACKARBPROC)wglGetProcAddress("glDebugMessageCallbackARB");
                glDebugMessageControlARB =  (PFNGLDEBUGMESSAGECONTROLARBPROC)wglGetProcAddress("glDebugMessageControlARB");
            }
            if(glDebugMessageCallbackARB)
            {
                glDebugMessageCallbackARB(myOpenGLCallback, NULL);
                glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH_ARB, 0, NULL, GL_TRUE);
            }
#endif
        }
    }
#endif
    glewInit();
    LOGOK("Loaded Glew\n");

    LOGOK("initialized OpenGL basis\n");
    return true;
}

//------------------------------------------------------------------------------
void keyboard_(unsigned char key, int x, int y)
{
    switch(key)
    {
    case VK_NUMPAD0:
    case VK_NUMPAD1:
    case VK_NUMPAD2:
    case VK_NUMPAD3:
    case VK_NUMPAD4:
    case VK_NUMPAD5:
    case VK_NUMPAD6:
    case VK_NUMPAD7:
    case VK_NUMPAD8:
    case VK_NUMPAD9:
    keyboard(key-0x60+'0', x, y);
    break;
    case VK_F1: specialkeys(/*GLUT_KEY_F1*/0x0001, x, y);
    break;
    case VK_F2: specialkeys(/*GLUT_KEY_F2*/0x0002, x, y);
    break;
    case VK_F3: specialkeys(/*GLUT_KEY_F3*/0x0003, x, y);
    break;
    case VK_F4: specialkeys(/*GLUT_KEY_F4*/0x0004, x, y);
    break;
    case VK_F5: specialkeys(/*GLUT_KEY_F5*/0x0005, x, y);
    break;
    case VK_F6: specialkeys(/*GLUT_KEY_F6*/0x0006, x, y);
    break;
    case VK_F7: specialkeys(/*GLUT_KEY_F7*/0x0007, x, y);
    break;
    case VK_F8: specialkeys(/*GLUT_KEY_F8*/0x0008, x, y);
    break;
    case VK_F9: specialkeys(/*GLUT_KEY_F9*/0x0009, x, y);
    break;
    case VK_F10: specialkeys(/*GLUT_KEY_F10*/0x000A, x, y);
    break;
    case VK_F11: specialkeys(/*GLUT_KEY_F11*/0x000B, x, y);
    break;
    case VK_F12: specialkeys(/*GLUT_KEY_F12*/0x000C, x, y);
    break;
    case VK_LEFT: specialkeys(/*GLUT_KEY_LEFT*/0x0064, x, y);
    break;
    case VK_UP: specialkeys(/*GLUT_KEY_UP*/0x0065, x, y);
    break;
    case VK_RIGHT: specialkeys(/*GLUT_KEY_RIGHT*/0x0066, x, y);
    break;
    case VK_DOWN: specialkeys(/*GLUT_KEY_DOWN*/0x0067, x, y);
    break;
    case VK_PRIOR: specialkeys(/*GLUT_KEY_PAGE_UP*/0x0068, x, y);
    break;
    case VK_NEXT: specialkeys(/*GLUT_KEY_PAGE_DOWN*/0x0069, x, y);
    break;
    case VK_HOME: specialkeys(/*GLUT_KEY_HOME*/0x006A, x, y);
    break;
    case VK_END: specialkeys(/*GLUT_KEY_END*/0x006B, x, y);
    break;
    case VK_INSERT: specialkeys(/*GLUT_KEY_INSERT*/0x006C, x, y);
    break;
    case VK_DELETE: keyboard(0x7f, x, y);
    break;
    case VK_SPACE: keyboard(' ', x,y);
    break;
    case VK_DIVIDE: keyboard('/', x,y);
    break;
    case VK_MULTIPLY: keyboard('*', x,y);
    break;
    case VK_SUBTRACT: keyboard('-', x,y);
    break;
    case VK_ADD: keyboard('+', x,y);
    break;
    case VK_DECIMAL: keyboard('.', x,y);
    break;
    case VK_OEM_1: keyboard(g_bShift?':':';', x,y);// ';:' for US
    break;
    case VK_OEM_PLUS: keyboard('+', x,y);// '+' any country
    break;
    case VK_OEM_COMMA: keyboard(',', x,y);// ',' any country
    break;
    case VK_OEM_MINUS: keyboard('-', x,y);// '-' any country
    break;
    case VK_OEM_PERIOD: keyboard('.', x,y);// '.' any country
    break;
    case VK_OEM_2: keyboard(g_bShift?'?':'/', x,y);// '/?' for US
    break;
    case VK_OEM_3: keyboard('~', x,y);// '`~' for US
    break;
    default: keyboard(g_bShift ? key : tolower(key), x,y);
    break;
    }
}
//------------------------------------------------------------------------------
LRESULT CALLBACK WindowProc( HWND   hWnd, 
                             UINT   msg, 
                             WPARAM wParam, 
                             LPARAM lParam )
{
    bool bRes = false;
    //
    // Pass the messages to our UI, first
    //
    if(!bRes) switch( msg )
    {
        case WM_PAINT:
            main::g_renderCnt++;
            break;
        case WM_CHAR:
        case WM_SYSCHAR:
            break;
        case WM_KEYUP:
            main::g_renderCnt++;
            switch( wParam )
            {
            case VK_CONTROL:
                main::g_bCtrl = false;
                break;
            case VK_SHIFT:
                main::g_bShift = false;
                break;
            }
            break;
        case WM_KEYDOWN:
            main::g_renderCnt++;
            switch( wParam )
            {
            case VK_CONTROL:
                main::g_bCtrl = true;
                break;
            case VK_SHIFT:
                main::g_bShift = true;
                break;
            default:
                keyboard_((unsigned char)wParam, main::g_curX, main::g_curY);
                break;
            }
            break;
        case WM_MOUSEWHEEL:
            mousewheel((short)HIWORD(wParam));
            break;
        case WM_LBUTTONDBLCLK:
            main::g_curX = LOWORD (lParam); main::g_curY = HIWORD (lParam);
            mouse(0, 2, LOWORD (lParam), HIWORD (lParam));
            break;
        case WM_LBUTTONDOWN:
            main::g_curX = LOWORD (lParam); main::g_curY = HIWORD (lParam);
            mouse(0, 0, LOWORD (lParam), HIWORD (lParam));
            break;
        case WM_LBUTTONUP:
            main::g_curX = LOWORD (lParam); main::g_curY = HIWORD (lParam);
            mouse(0, 1, LOWORD (lParam), HIWORD (lParam));
            break;
        case WM_RBUTTONDBLCLK:
            main::g_curX = LOWORD (lParam); main::g_curY = HIWORD (lParam);
            mouse(2, 2, LOWORD (lParam), HIWORD (lParam));
            break;
        case WM_RBUTTONDOWN:
            main::g_curX = LOWORD (lParam); main::g_curY = HIWORD (lParam);
            mouse(2, 0, LOWORD (lParam), HIWORD (lParam));
            break;
        case WM_RBUTTONUP:
            main::g_curX = LOWORD (lParam); main::g_curY = HIWORD (lParam);
            mouse(2, 1, LOWORD (lParam), HIWORD (lParam));
            break;
        case WM_MBUTTONDOWN:
            main::g_curX = LOWORD (lParam); main::g_curY = HIWORD (lParam);
            mouse(1, 0, LOWORD (lParam), HIWORD (lParam));
            break;
        case WM_MBUTTONUP:
            main::g_curX = LOWORD (lParam); main::g_curY = HIWORD (lParam);
            mouse(1, 1, LOWORD (lParam), HIWORD (lParam));
            break;
        case WM_MOUSEMOVE:
            main::g_curX = LOWORD (lParam); main::g_curY = HIWORD (lParam);
            motion(LOWORD (lParam), HIWORD (lParam));
            break;
        case WM_SIZE:
            g_winSz[0] = LOWORD(lParam);
            g_winSz[1] = HIWORD(lParam);
            reshape(LOWORD(lParam), HIWORD(lParam));
            break;
        case WM_CLOSE:
            PostQuitMessage(0);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            break;
    }
    return DefWindowProc( hWnd, msg, wParam, lParam );
}
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
static size_t fmt2_sz    = 0;
static char *fmt2 = NULL;
static FILE *fd = NULL;
static bool bLogReady = false;
void logMessage(int level, const char * fmt, ...)
{
//    int r = 0;
    if(fmt2_sz == 0) {
        fmt2_sz = 1024;
        fmt2 = (char*)malloc(fmt2_sz);
    }
    va_list  vlist;
    va_start(vlist, fmt);
    while((vsnprintf(fmt2, fmt2_sz, fmt, vlist)) < 0) // means there wasn't anough room
    {
        fmt2_sz *= 2;
        if(fmt2) free(fmt2);
        fmt2 = (char*)malloc(fmt2_sz);
    }
#ifdef WIN32
    OutputDebugStringA(fmt2);
#ifdef _DEBUG
    if(bLogReady == false)
    {
        fd = fopen("Log_nvFX.txt", "w");
        bLogReady = true;
    }
    if(fd)
        fprintf(fd, fmt2);

#endif
#endif
#ifdef USESVCUI
    if(g_pLog)
        g_pLog->AddMessage(level, fmt2);
    else
#endif
    extern void logMessageCB(int level, const char * str);
    logMessageCB(level, fmt2);
    ::printf(fmt2);
}

} //namespace main

//----------------------------------------------------------------------------------------------
// http://www.codeguru.com/cpp/w-p/win32/article.php/c1427/A-Simple-Win32-CommandLine-Parser.htm
class CmdLineArgs : public std::vector<char*>
{
public:
  CmdLineArgs ()
  {
    // Save local copy of the command line string, because
    // ParseCmdLine() modifies this string while parsing it.
    PSZ cmdline = GetCommandLineA();
    m_cmdline = new char [strlen (cmdline) + 1];
    if (m_cmdline)
    {
      strcpy (m_cmdline, cmdline);
      ParseCmdLine();
    }
  }
  ~CmdLineArgs()
  {
    delete m_cmdline;
  }
private:
  PSZ m_cmdline; // the command line string
  ////////////////////////////////////////////////////////////////////////////////
  // Parse m_cmdline into individual tokens, which are delimited by spaces. If a
  // token begins with a quote, then that token is terminated by the next quote
  // followed immediately by a space or terminator. This allows tokens to contain
  // spaces.
  // This input string: This "is" a ""test"" "of the parsing" alg"o"rithm.
  // Produces these tokens: This, is, a, "test", of the parsing, alg"o"rithm
  ////////////////////////////////////////////////////////////////////////////////
  void ParseCmdLine ()
  {
    enum { TERM = '\0',
          QUOTE = '\"' };
    bool bInQuotes = false;
    PSZ pargs = m_cmdline;
    while (*pargs)
    {
      while (isspace (*pargs)) // skip leading whitespace
        pargs++;
      bInQuotes = (*pargs == QUOTE); // see if this token is quoted
      if (bInQuotes) // skip leading quote
        pargs++;
      push_back (pargs); // store position of current token
      // Find next token.
      // NOTE: Args are normally terminated by whitespace, unless the
      // arg is quoted. That's why we handle the two cases separately,
      // even though they are very similar.
      if (bInQuotes)
      {
        // find next quote followed by a space or terminator
        while (*pargs &&
          !(*pargs == QUOTE && (isspace (pargs[1]) || pargs[1] == TERM)))
          pargs++;
        if (*pargs)
        {
          *pargs = TERM; // terminate token
          if (pargs[1]) // if quoted token not followed by a terminator
            pargs += 2; // advance to next token
        }
      }
      else
      {
        // skip to next non-whitespace character
        while (*pargs && !isspace (*pargs))
          pargs++;
        if (*pargs && isspace (*pargs)) // end of token
        {
          *pargs = TERM; // terminate token
          pargs++; // advance to next token or terminator
        }
      }
    } // while (*pargs)
  } // ParseCmdLine()
}; // class CmdLineArgs

//------------------------------------------------------------------------------
int WINAPI WinMain(    HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR     lpCmdLine,
                    int       nCmdShow )
{
    //initNSight();
#ifdef MEMORY_LEAKS_CHECK
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); 
    _CrtSetReportMode ( _CRT_ERROR, _CRTDBG_MODE_DEBUG|_CRTDBG_MODE_WNDW);
#endif

    WNDCLASSEX winClass;
    MSG        uMsg;

    main::g_hInstance = hInstance;

    memset(&uMsg,0,sizeof(uMsg));

    winClass.lpszClassName = "MY_WINDOWS_CLASS";
    winClass.cbSize        = sizeof(WNDCLASSEX);
    winClass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
    winClass.lpfnWndProc   = main::WindowProc;
    winClass.hInstance     = hInstance;
    winClass.hIcon           = LoadIcon(hInstance, (LPCTSTR)IDI_OPENGL_ICON);
    winClass.hIconSm       = LoadIcon(hInstance, (LPCTSTR)IDI_OPENGL_ICON);
    winClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    winClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    winClass.lpszMenuName  = NULL;
    winClass.cbClsExtra    = 0;
    winClass.cbWndExtra    = 0;
    
    if(!RegisterClassEx(&winClass) )
        return E_FAIL;

    main::g_hWnd = CreateWindowEx( NULL, "MY_WINDOWS_CLASS",
                             "Viewer",
                             WS_OVERLAPPEDWINDOW, 0, 0, main::g_winSz[0], main::g_winSz[1], NULL, NULL, 
                             hInstance, NULL );

    if( main::g_hWnd == NULL )
        return E_FAIL;

    ShowWindow( main::g_hWnd, nCmdShow );
    UpdateWindow( main::g_hWnd );

#if defined(USESVCUI) || defined(USEANTTWEAKBAR)
    main::initUIBase();
#endif
    // Initialize the very base of OpenGL
    if(main::initGLBase())
    {
        // Initialize more general stuff... typically what can be declared when using GLUT
        // TODO: replace the lpCmdLine by proper C-style arguments
        CmdLineArgs args;

        std::string exe = args[0];
        //std::replace(exe.begin(),exe.end(),'\\','/');
        //size_t last = exe.rfind('/');
        //if (last != std::string::npos){
        //  g_path = exe.substr(0,last) + std::string("/");
        //}

        main::initGL((int)args.size(), (const char**)&args[0]);

        //---------------------------------------------------------------------------
        // Message pump
        while( uMsg.message != WM_QUIT )
        {
            if( PeekMessage( &uMsg, NULL, 0, 0, PM_REMOVE ) )
            { 
                TranslateMessage( &uMsg );
                DispatchMessage( &uMsg );
            }
            else 
            {
                main::idle();
                if(main::g_renderCnt > 0)
                {
                    main::g_renderCnt--;
                    main::display();
                    Sleep(1);
                } else
                    Sleep(10);
            }
        }
    }
    main::shutdown();
    if( main::g_hRC != NULL )
    {
        ReleaseDC( main::g_hWnd, main::g_hDC );
        main::g_hDC = NULL;
    }
    main::shutdownBase();
    UnregisterClass( "MY_WINDOWS_CLASS", hInstance );

#ifdef MEMORY_LEAKS_CHECK
    _CrtDumpMemoryLeaks();
#endif

    return (int)uMsg.wParam;
}
#else
int main(int argc, char ** argv) {

    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_RGBA |GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowPosition(0,0);
    glutInitWindowSize(g_winSz[0], g_winSz[1]);
    glutCreateWindow("OpenGL Test");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialkeys);
    glutMotionFunc(motion);
#if defined(USESVCUI) || defined(USEANTTWEAKBAR)
    initUIBase();
#endif
    glewInit();
    LOGOK("initialized Glew\n");
    initGL(argc, argv);
    glutIdleFunc(idle);
    glutMainLoop();
    shutdown();
    main::shutdownBase();
}
#endif

//------------------------------------------------------------------------------
