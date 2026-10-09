// Reconstructed older UI family adaptation.
// Original Tribes-AA 4aab7137 support/ui/ui_manager.hpp; complete Area51 variant available in external entropy-src donor.
//==============================================================================
//  
//  ui_manager.hpp
//  
//==============================================================================

#ifndef UI_MANAGER_HPP
#define UI_MANAGER_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#ifndef X_TYPES_HPP
#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_math.hpp>
#endif

//==============================================================================
//  Externals
//==============================================================================

class ui_win;
class ui_font;
class ui_dialog;
class ui_control;

//==============================================================================
//  Types
//==============================================================================

//==============================================================================
//  Logging
//==============================================================================

extern xstring ui_log;

//==============================================================================
//  ui_manager
//==============================================================================

class ui_manager
{
public:

    //==========================================================================
    //  Templates for dialogs and controls
    //==========================================================================

    struct control_tem
    {
        s32             ControlID;
        s32             StringID;
        const char*     pClass;
        s32             x, y, w ,h;
        s32             nx, ny, nw, nh;
        s32             Flags;
    };

    struct dialog_tem
    {
        s32             StringID;
        s32             NavW, NavH;
        s32             nControls;
        control_tem*    pControls;
        s32             FocusControl;
    };

    //==========================================================================
    //  Typedefs for window and dialog factories
    //==========================================================================

    typedef ui_win* (*ui_pfn_winfact)( s32 UserID, ui_manager* pManager, const irect& Position, ui_win* pParent, s32 Flags );
    typedef ui_win* (*ui_pfn_dlgfact)( s32 UserID, ui_manager* pManager, ui_manager::dialog_tem* pDialogTem, const irect& Position, ui_win* pParent, s32 Flags, void* pUserData );

    //==========================================================================
    //  Input Button Data
    //==========================================================================

    class button
    {
    public:
        xbool       State;
        f32         AnalogScaler;
        f32         AnalogEngage;
        f32         AnalogDisengage;
        f32         RepeatDelay;
        f32         RepeatInterval;
        f32         RepeatTimer;
        s32         nPresses;
        s32         nRepeats;
        s32         nReleases;

    public:
                    button              ( void ) { State = 0; nPresses = 0; nRepeats = 0; nReleases = 0; RepeatDelay = 0.200f; RepeatInterval = 0.060f; AnalogScaler = 1.0f; AnalogEngage = 0.5f; AnalogDisengage = 0.3f; };
                   ~button              ( void ) {};

        void        Clear               ( void )                    { State = 0; nPresses = 0; nRepeats = 0; nReleases = 0; };

        void        SetupRepeat         ( f32 Delay, f32 Interval ) { RepeatDelay = Delay; RepeatInterval = Interval; };
        void        SetupAnalog         ( f32 s, f32 e, f32 d )     { AnalogScaler = s ; AnalogEngage = e; AnalogDisengage = d; };
    };

    //==========================================================================
    //  Control States
    //==========================================================================

    enum control_state
    {
        CS_NORMAL               = 0,
        CS_HIGHLIGHT,
        CS_SELECTED,
        CS_HIGHLIGHT_SELECTED,
        CS_DISABLED
    };

    //==========================================================================
    //  User Data
    //==========================================================================

    enum user_navigate
    {
        NAV_UP,
        NAV_DOWN,
        NAV_LEFT,
        NAV_RIGHT,
    };

    struct user
    {
        xbool                   Enabled;
        s32                     ControllerID;
        irect                   Bounds;
        s32                     Data;
        s32                     Height;
        ui_win*                 pCaptureWindow;
        ui_win*                 pLastWindowUnderCursor;
        s32                     iBackground;
        s32                     iHighlightElement;

        xbool                   CursorVisible;                // TRUE when Mouse Cursor Visible
        xbool                   MouseActive;                  // TRUE when Mouse Active
        s32                     CursorX;                      // Mouse Cursor X
        s32                     CursorY;                      // Mouse Cursor Y
        s32                     LastCursorX;                  // Last Mouse Cursor X
        s32                     LastCursorY;                  // Last Mouse Cursor Y
        button                  ButtonLB;
        button                  ButtonMB;
        button                  ButtonRB;

        button                  DPadUp[2];
        button                  DPadDown[2];
        button                  DPadLeft[2];
        button                  DPadRight[2];
        button                  PadSelect[2];
        button                  PadBack[2];
        button                  PadDelete[2];
        button                  PadHelp[2];
        button                  PadShoulderL[2];
        button                  PadShoulderR[2];
        button                  PadShoulderL2[2];
        button                  PadShoulderR2[2];
        button                  LStickUp[2];
        button                  LStickDown[2];
        button                  LStickLeft[2];
        button                  LStickRight[2];

        xarray<ui_dialog*>      DialogStack;
    };

    //==========================================================================
    //  Window Class
    //==========================================================================

    struct winclass
    {
        xstring         ClassName;
        ui_pfn_winfact  pFactory;
    };

    //==========================================================================
    //  Graphic Element for UI
    //==========================================================================

    struct element
    {
        xstring         Name;
        xbitmap         Bitmap;
        s32             nStates;
        s32             cx;
        s32             cy;
        xarray<irect>   r;
    };

    //==========================================================================
    //  Background
    //==========================================================================

    struct background
    {
        xstring         Name;
        xbitmap         Bitmap;
    };

    //==========================================================================
    //  Font
    //==========================================================================

    struct font
    {
        xstring         Name;
        ui_font*        pFont;
    };

    //==========================================================================
    //  Dialog Class
    //==========================================================================

    struct dialogclass
    {
        xstring         ClassName;
        ui_pfn_dlgfact  pFactory;
        dialog_tem*     pDialogTem;
    };

    //==========================================================================
    //  Clip Record
    //==========================================================================

    struct cliprecord
    {
        irect           r;
    };

    //==========================================================================
    //  Highlight
    //==========================================================================

    struct highlight
    {
        irect           r;
        s32             iElement;
        xbool           Flash;
    };

//==============================================================================
//  Functions
//==============================================================================

protected:
    void            UpdateButton        ( ui_manager::button& Button, xbool State, f32 DeltaTime );
    void            UpdateAnalog        ( ui_manager::button& Button, f32 Value, f32 DeltaTime );

public:
                    ui_manager          ( void );
                   ~ui_manager          ( void );

    void            Init                ( void );
    void            Kill                ( void );

    s32             LoadBackground      ( const char* pName, const char* pPathName );
    s32             FindBackground      ( const char* pName );
    void            RenderBackground    ( s32 iBackground ) const;

    s32             LoadElement         ( const char* pName, const char* pPathName, s32 nStates, s32 cx, s32 cy );
    s32             FindElement         ( const char* pName ) const;
    void            RenderElement       ( s32 iElement, const irect& Position,       s32 State, const xcolor& Color = XCOLOR_WHITE, xbool IsAdditive = FALSE ) const;
    void            RenderElementUV     ( s32 iElement, const irect& Position, const irect& UV, const xcolor& Color = XCOLOR_WHITE, xbool IsAdditive = FALSE ) const;
    const element*  GetElement          ( s32 iElement ) const;

    s32             LoadFont            ( const char* pName, const char* pPathName );
    s32             FindFont            ( const char* pName ) const;
    void            RenderText          ( s32 iFont, const irect& Position, s32 Flags, const xcolor& Color, const   char* pString ) const;
    void            RenderText          ( s32 iFont, const irect& Position, s32 Flags, const xcolor& Color, const xwchar* pString ) const;
    void            RenderText_Wrap     ( s32 iFont, const irect& Position, s32 Flags, const xcolor& Color, const xwstring& String );
    void            TextSize            ( s32 iFont, irect& Rect, const xwchar* pString, s32 Count ) const;
    s32             GetLineHeight       ( s32 iFont ) const;

    void            RenderRect          ( const irect& r, const xcolor& Color, xbool IsWire=TRUE ) const;
    void            RenderGouraudRect   ( const irect& r, const xcolor& c1, const xcolor& c2, const xcolor& c3, const xcolor& c4, xbool IsWire=TRUE ) const;

    xbool           RegisterWinClass    ( const char* ClassName, ui_pfn_winfact pFactory );
    ui_win*         CreateWin           ( s32 UserID, const char* ClassName, const irect& Position, ui_win* pParent, s32 Flags );

    xbool           RegisterDialogClass ( const char* ClassName, dialog_tem* pDialogTem, ui_pfn_dlgfact pFactory );
    ui_dialog*      OpenDialog          ( s32 UserID, const char* ClassName, irect Position, ui_win* pParent, s32 Flags, void* pUserData = NULL );
    void            EndDialog           ( s32 UserID, xbool ResetCursor = FALSE );
    void            EndUsersDialogs     ( s32 UserID );
    s32             GetNumUserDialogs   ( s32 UserID );
    ui_dialog*      GetTopmostDialog    ( s32 UserID );

    s32             CreateUser          ( s32 ControllerID, const irect& Bounds, s32 Data = 0 );
    void            DeleteUser          ( s32 UserID );
    void            DeleteAllUsers      ( void );
    user*           GetUser             ( s32 UserID ) const;
    s32             GetUserData         ( s32 UserID ) const;
    ui_win*         GetWindowUnderCursor( s32 UserID ) const;
    void            SetCursorVisible    ( s32 UserID, xbool State );
    xbool           GetCursorVisible    ( s32 UserID ) const;
    void            SetCursorPos        ( s32 UserID, s32  x, s32  y );
    void            GetCursorPos        ( s32 UserID, s32& x, s32& y ) const;
    ui_win*         SetCapture          ( s32 UserID, ui_win* pWin );
    void            ReleaseCapture      ( s32 UserID );
    void            SetUserBackground   ( s32 UserID, s32 iBackground );
    const irect&    GetUserBounds       ( s32 UserID ) const;
    void            EnableUser          ( s32 UserID, xbool State );
    xbool           IsUserEnabled       ( s32 UserID ) const;
    void            AddHighlight        ( s32 UserID, irect& r, xbool Flash = TRUE );

    void            PushClipWindow      ( const irect &r );
    void            PopClipWindow       ( void );

    ui_win*         GetWindowAtXY       ( user* pUser, s32 x, s32 y );
    xbool           ProcessInput        ( f32 DeltaTime );
    xbool           ProcessInput        ( f32 DeltaTime, s32 UserID );

    void            EnableUserInput     ( void );
    void            DisableUserInput    ( void );

    void            Update              ( f32 DeltaTime );
    void            Render              ( void );

    void            WordWrapString      ( s32 iFont, const irect& r, const xwstring& String, xwstring& Wrapped );

    void            SetRes              ( void );

//==============================================================================
//  Data
//==============================================================================

protected:

    // Descriptive reconstructed spelling; original spelling/access unknown.
    friend class ui_bitmap_font;
    friend class ui_truetype_font;
    f32                     m_AlphaTime;
    f32                     m_HighlightAlpha;

    xarray<user*>           m_Users;

    xarray<winclass>        m_WindowClasses;
    xarray<dialogclass>     m_DialogClasses;

    xarray<background*>     m_Backgrounds;
    xarray<element*>        m_Elements;
    xarray<font*>           m_Fonts;

    xarray<cliprecord>      m_ClipStack;

    xarray<highlight>       m_Highlights;

    xbool                   m_EnableUserInput;

    xbitmap                 m_Mouse;
    xcolor                  m_MouseColor;

    xbitmap                 m_ButtonBitmaps[19];
    s32                     m_ScreenWidth;
    s32                     m_ScreenHeight;

#ifdef TARGET_PC
    xarray<ui_dialog*>      m_KillDialogStack;
    xbool                   m_EnableCursor;
    xbool                   m_UseTrueTypeFont;
    xstring                 m_FontFaceName;
    s32                     m_FontHeight;
    f32                     m_FontWidthScale;
    s32                     m_FontCharSet;
#endif

public:
    xstring*            m_log;
};

//==============================================================================
// Genuine original global API; complete provider lives in ui_manager.cpp.
extern ui_manager* g_UiMgr;

#endif // UI_MANAGER_HPP
//==============================================================================
