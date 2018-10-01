// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Kernel {
class Mutex;
class SharedMemory;
} // namespace Kernel

namespace Service::APT {

class AppletManager;

/// Each APT service can only have up to 2 sessions connected at the same time.
static const u32 MaxAPTSessions = 2;

/// Used by the application to pass information about the current framebuffer to applets.
struct CaptureBufferInfo {
    u32_le size;
    u8 is_3d;
    INSERT_PADDING_BYTES(0x3); // Padding for alignment
    u32_le top_screen_left_offset;
    u32_le top_screen_right_offset;
    u32_le top_screen_format;
    u32_le bottom_screen_left_offset;
    u32_le bottom_screen_right_offset;
    u32_le bottom_screen_format;
};
static_assert(sizeof(CaptureBufferInfo) == 0x20, "CaptureBufferInfo struct has incorrect size");

enum class StartupArgumentType : u32 {
    OtherApp = 0,
    Restart = 1,
    OtherMedia = 2,
};

enum class ScreencapPostPermission : u32 {
    CleanThePermission = 0, // TODO(JamePeng): verify what "zero" means
    NoExplicitSetting = 1,
    EnableScreenshotPostingToMiiverse = 2,
    DisableScreenshotPostingToMiiverse = 3
};

class Module final {
public:
    explicit Module(Core::System& system);
    ~Module();

    class Interface : public ServiceFramework<Interface> {
    public:
        Interface(std::shared_ptr<Module> apt, const char* name, u32 max_session);
        ~Interface();

    protected:
        /**
         * APT::Initialize service function
         * Service function that initializes the APT process for the running application
         *  Outputs:
         *      1 : Result of the function, 0 on success, otherwise error code
         *      3 : Handle to the notification event
         *      4 : Handle to the pause event
         */
        void Initialize(Kernel::HLERequestContext& ctx);

        /**
         * APT::GetSharedFont service function
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Virtual address of where shared font will be loaded in memory
         *      4 : Handle to shared font memory
         */
        void GetSharedFont(Kernel::HLERequestContext& ctx);

        /**
         * APT::Wrap service function
         *  Inputs:
         *      1 : Output buffer size
         *      2 : Input buffer size
         *      3 : Nonce offset to the input buffer
         *      4 : Nonce size
         *      5 : Buffer mapping descriptor ((input_buffer_size << 4) | 0xA)
         *      6 : Input buffer address
         *      7 : Buffer mapping descriptor ((input_buffer_size << 4) | 0xC)
         *      8 : Output buffer address
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Buffer unmapping descriptor ((input_buffer_size << 4) | 0xA)
         *      3 : Input buffer address
         *      4 : Buffer unmapping descriptor ((input_buffer_size << 4) | 0xC)
         *      5 : Output buffer address
         */
        void Wrap(Kernel::HLERequestContext& ctx);

        /**
         * APT::Unwrap service function
         *  Inputs:
         *      1 : Output buffer size
         *      2 : Input buffer size
         *      3 : Nonce offset to the output buffer
         *      4 : Nonce size
         *      5 : Buffer mapping descriptor ((input_buffer_size << 4) | 0xA)
         *      6 : Input buffer address
         *      7 : Buffer mapping descriptor ((input_buffer_size << 4) | 0xC)
         *      8 : Output buffer address
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Buffer unmapping descriptor ((input_buffer_size << 4) | 0xA)
         *      3 : Input buffer address
         *      4 : Buffer unmapping descriptor ((input_buffer_size << 4) | 0xC)
         *      5 : Output buffer address
         */
        void Unwrap(Kernel::HLERequestContext& ctx);

        /**
         * APT::NotifyToWait service function
         *  Inputs:
         *      1 : AppID
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void NotifyToWait(Kernel::HLERequestContext& ctx);

        /**
         * APT::GetLockHandle service function
         *  Inputs:
         *      1 : Applet attributes
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Applet attributes
         *      3 : Power button state
         *      4 : IPC handle descriptor
         *      5 : APT mutex handle
         */
        void GetLockHandle(Kernel::HLERequestContext& ctx);

        /**
         * APT::Enable service function
         *  Inputs:
         *      1 : Applet attributes
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void Enable(Kernel::HLERequestContext& ctx);

        /**
         * APT::GetAppletManInfo service function.
         *  Inputs:
         *      1 : Unknown
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Unknown u32 value
         *      3 : Unknown u8 value
         *      4 : Home Menu AppId
         *      5 : AppID of currently active app
         */
        void GetAppletManInfo(Kernel::HLERequestContext& ctx);

        /**
         * APT::GetAppletInfo service function.
         *  Inputs:
         *      1 : AppId
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2-3 : Title ID
         *      4 : Media Type
         *      5 : Registered
         *      6 : Loaded
         *      7 : Attributes
         */
        void GetAppletInfo(Kernel::HLERequestContext& ctx);

        /**
         * APT::IsRegistered service function. This returns whether the specified AppID is
         * registered with NS yet. An AppID is "registered" once the process associated with the
         * AppID uses APT:Enable. Home Menu uses this command to determine when the launched process
         * is running and to determine when to stop using GSP, etc., while displaying the "Nintendo
         * 3DS" loading screen.
         *
         *  Inputs:
         *      1 : AppID
         *  Outputs:
         *      0 : Return header
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Output, 0 = not registered, 1 = registered.
         */
        void IsRegistered(Kernel::HLERequestContext& ctx);

        void InquireNotification(Kernel::HLERequestContext& ctx);

        /**
         * APT::SendParameter service function. This sets the parameter data state.
         * Inputs:
         *     1 : Source AppID
         *     2 : Destination AppID
         *     3 : Signal type
         *     4 : Parameter buffer size, max size is 0x1000 (this can be zero)
         *     5 : Value
         *     6 : Handle to the destination process, likely used for shared memory (this can be
         * zero)
         *     7 : (Size<<14) | 2
         *     8 : Input parameter buffer ptr
         * Outputs:
         *     0 : Return Header
         *     1 : Result of function, 0 on success, otherwise error code
         */
        void SendParameter(Kernel::HLERequestContext& ctx);

        /**
         * APT::ReceiveParameter service function. This returns the current parameter data from NS
         * state, from the source process which set the parameters. Once finished, NS will clear a
         * flag in the NS state so that this command will return an error if this command is used
         * again if parameters were not set again. This is called when the second Initialize event
         * is triggered. It returns a signal type indicating why it was triggered.
         *  Inputs:
         *      1 : AppID
         *      2 : Parameter buffer size, max size is 0x1000
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : AppID of the process which sent these parameters
         *      3 : Signal type
         *      4 : Actual parameter buffer size, this is <= to the the input size
         *      5 : Value
         *      6 : Handle from the source process which set the parameters, likely used for shared
         * memory
         *      7 : Size
         *      8 : Output parameter buffer ptr
         */
        void ReceiveParameter(Kernel::HLERequestContext& ctx);

        /**
         * APT::GlanceParameter service function. This is exactly the same as
         * APT_U::ReceiveParameter (except for the word value prior to the output handle), except
         * this will not clear the flag (except when responseword[3]==8 || responseword[3]==9) in NS
         * state.
         *  Inputs:
         *      1 : AppID
         *      2 : Parameter buffer size, max size is 0x1000
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Unknown, for now assume AppID of the process which sent these parameters
         *      3 : Unknown, for now assume Signal type
         *      4 : Actual parameter buffer size, this is <= to the the input size
         *      5 : Value
         *      6 : Handle from the source process which set the parameters, likely used for shared
         * memory
         *      7 : Size
         *      8 : Output parameter buffer ptr
         */
        void GlanceParameter(Kernel::HLERequestContext& ctx);

        /**
         * APT::CancelParameter service function. When the parameter data is available, and when the
         * above specified fields match the ones in NS state(for the ones where the checks are
         * enabled), this clears the flag which indicates that parameter data is available (same
         * flag cleared by APT:ReceiveParameter).
         *  Inputs:
         *      1 : Flag, when non-zero NS will compare the word after this one with a field in the
         * NS
         *          state.
         *      2 : Unknown, this is the same as the first unknown field returned by
         * APT:ReceiveParameter.
         *      3 : Flag, when non-zero NS will compare the word after this one with a field in the
         * NS
         *          state.
         *      4 : AppID
         *  Outputs:
         *      0 : Return header
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : Status flag, 0 = failure due to no parameter data being available, or the above
         * enabled
         *          fields don't match the fields in NS state. 1 = success.
         */
        void CancelParameter(Kernel::HLERequestContext& ctx);

        /**
         * APT::PrepareToStartApplication service function. When the input title-info programID is
         * zero, NS will load the actual program ID via AMNet:GetTitleIDList. After doing some
         * checks with the programID, NS will then set a NS state flag to value 1, then set the
         * programID for AppID 0x300(application) to the input program ID(or the one from
         * GetTitleIDList). A media-type field in the NS state is also set to the input media-type
         * value (other state fields are set at this point as well). With 8.0.0-18, NS will set an
         * u8 NS state field to value 1 when input flags bit8 is set.
         *  Inputs:
         *    1-4 : 0x10-byte title-info struct
         *      4 : Flags
         *  Outputs:
         *      0 : Return header
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void PrepareToStartApplication(Kernel::HLERequestContext& ctx);

        /**
         * APT::StartApplication service function. Buf0 is copied to NS FIRMparams+0x0, then Buf1 is
         * copied to the NS FIRMparams+0x480. Then the application is launched.
         * Inputs:
         *     1 : Buffer 0 size, max size is 0x300
         *     2 : Buffer 1 size, max size is 0x20 (this can be zero)
         *     3 : u8 flag
         *     4 : (Size0<<14) | 2
         *     5 : Buffer 0 pointer
         *     6 : (Size1<<14) | 0x802
         *     7 : Buffer 1 pointer
         * Outputs:
         *     0 : Return Header
         *     1 : Result of function, 0 on success, otherwise error code
         */
        void StartApplication(Kernel::HLERequestContext& ctx);

        /**
         * APT::AppletUtility service function
         *  Inputs:
         *      1 : Unknown, but clearly used for something
         *      2 : Buffer 1 size (purpose is unknown)
         *      3 : Buffer 2 size (purpose is unknown)
         *      5 : Buffer 1 address (purpose is unknown)
         *      65 : Buffer 2 address (purpose is unknown)
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void AppletUtility(Kernel::HLERequestContext& ctx);

        /**
         * APT::SetAppCpuTimeLimit service function
         *  Inputs:
         *      1 : Value, must be one
         *      2 : Percentage of CPU time from 5 to 80
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void SetAppCpuTimeLimit(Kernel::HLERequestContext& ctx);

        /**
         * APT::GetAppCpuTimeLimit service function
         *  Inputs:
         *      1 : Value, must be one
         *  Outputs:
         *      0 : Return header
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : System core CPU time percentage
         */
        void GetAppCpuTimeLimit(Kernel::HLERequestContext& ctx);

        /**
         * APT::PrepareToStartLibraryApplet service function
         *  Inputs:
         *      0 : Command header [0x00180040]
         *      1 : Id of the applet to start
         *  Outputs:
         *      0 : Return header
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void PrepareToStartLibraryApplet(Kernel::HLERequestContext& ctx);

        /**
         * APT::PrepareToStartNewestHomeMenu service function
         *  Inputs:
         *      0 : Command header [0x001A0000]
         *  Outputs:
         *      0 : Return header
         *      1 : Result of function
         */
        void PrepareToStartNewestHomeMenu(Kernel::HLERequestContext& ctx);

        /**
         * APT::PreloadLibraryApplet service function
         *  Inputs:
         *      0 : Command header [0x00160040]
         *      1 : Id of the applet to start
         *  Outputs:
         *      0 : Return header
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void PreloadLibraryApplet(Kernel::HLERequestContext& ctx);

        /**
         * APT::FinishPreloadingLibraryApplet service function
         *  Inputs:
         *      0 : Command header [0x00170040]
         *      1 : Id of the applet
         *  Outputs:
         *      0 : Return header
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void FinishPreloadingLibraryApplet(Kernel::HLERequestContext& ctx);

        /**
         * APT::StartLibraryApplet service function
         *  Inputs:
         *      0 : Command header [0x001E0084]
         *      1 : Id of the applet to start
         *      2 : Buffer size
         *      3 : Always 0?
         *      4 : Handle passed to the applet
         *      5 : (Size << 14) | 2
         *      6 : Input buffer virtual address
         *  Outputs:
         *      0 : Return header
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void StartLibraryApplet(Kernel::HLERequestContext& ctx);

        /**
         * APT::CloseApplication service function
         *  Inputs:
         *      0 : Command header [0x00270044]
         *      1 : Parameters Size
         *      2 : 0x0
         *      3 : Handle Parameter
         *      4 : (Parameters Size << 14) | 2
         *      5 : void*, Parameters
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void CloseApplication(Kernel::HLERequestContext& ctx);

        /**
         * APT::PrepareToDoApplicationJump service function
         *  Inputs:
         *      0 : Command header [0x00310100]
         *      1 : Flags
         *      2 : Program ID low
         *      3 : Program ID high
         *      4 : Media type
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         * @param ctx
         */
        void PrepareToDoApplicationJump(Kernel::HLERequestContext& ctx);

        /**
         * APT::DoApplicationJump service function
         *  Inputs:
         *      0 : Command header [0x00320084]
         *      1 : Parameter Size (capped to 0x300)
         *      2 : HMAC Size (capped to 0x20)
         *      3 : (Parameter Size << 14) | 2
         *      4 : void*, Parameter
         *      5 : (HMAC Size << 14) | 0x802
         *      6 : void*, HMAC
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void DoApplicationJump(Kernel::HLERequestContext& ctx);

        /**
         * APT::CancelLibraryApplet service function
         *  Inputs:
         *      0 : Command header [0x003B0040]
         *      1 : u8, Application exiting (0 = not exiting, 1 = exiting)
         *  Outputs:
         *      0 : Header code
         *      1 : Result code
         */
        void CancelLibraryApplet(Kernel::HLERequestContext& ctx);

        /**
         * APT::PrepareToCloseLibraryApplet service function
         *  Inputs:
         *      0 : Command header [0x002500C0]
         *      1 : bool, Not pause
         *      2 : bool, Caller exiting
         *      3 : bool, Jump to home
         *  Outputs:
         *      0 : Header code
         *      1 : Result code
         */
        void PrepareToCloseLibraryApplet(Kernel::HLERequestContext& ctx);

        /**
         * APT::CloseLibraryApplet service function
         *  Inputs:
         *      0 : Command header [0x00280044]
         *      1 : Buffer size
         *      2 : 0x0
         *      3 : Object handle
         *      4 : (Size << 14) | 2
         *      5 : Input buffer virtual address
         *  Outputs:
         *      0 : Header code
         *      1 : Result code
         */
        void CloseLibraryApplet(Kernel::HLERequestContext& ctx);

        /**
         * APT::SendCaptureBufferInfo service function
         *  Inputs:
         *      0 : Command header [0x00400042]
         *      1 : Size
         *      2 : (Size << 14) | 2
         *      3 : void*, CaptureBufferInfo
         *  Outputs:
         *      0 : Header code
         *      1 : Result code
         */
        void SendCaptureBufferInfo(Kernel::HLERequestContext& ctx);

        /**
         * APT::ReceiveCaptureBufferInfo service function
         *  Inputs:
         *      0 : Command header [0x00410040]
         *      1 : Size
         *      64 : Size << 14 | 2
         *      65 : void*, CaptureBufferInfo
         *  Outputs:
         *      0 : Header code
         *      1 : Result code
         *      2 : Actual Size
         */
        void ReceiveCaptureBufferInfo(Kernel::HLERequestContext& ctx);

        /**
         * APT::GetStartupArgument service function
         *  Inputs:
         *      1 : Parameter Size (capped to 0x300)
         *      2 : StartupArgumentType
         *      65 : Output buffer for startup argument
         *  Outputs:
         *      0 : Return header
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : u8, Exists (0 = does not exist, 1 = exists)
         */
        void GetStartupArgument(Kernel::HLERequestContext& ctx);

        /**
         * APT::SetScreenCapPostPermission service function
         *  Inputs:
         *      0 : Header Code[0x00550040]
         *      1 : u8 The screenshot posting permission
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         */
        void SetScreenCapPostPermission(Kernel::HLERequestContext& ctx);

        /**
         * APT::GetScreenCapPostPermission service function
         *  Inputs:
         *      0 : Header Code[0x00560000]
         *  Outputs:
         *      1 : Result of function, 0 on success, otherwise error code
         *      2 : u8 The screenshot posting permission
         */
        void GetScreenCapPostPermission(Kernel::HLERequestContext& ctx);

        /**
         * APT::CheckNew3DSApp service function
         *  Outputs:
         *      1: Result code, 0 on success, otherwise error code
         *      2: u8 output: 0 = Old3DS, 1 = New3DS.
         *  Note:
         *  This uses PTMSYSM:CheckNew3DS.
         *  When a certain NS state field is non-zero, the output value is zero,
         *  Otherwise the output is from PTMSYSM:CheckNew3DS.
         *  Normally this NS state field is zero, however this state field is set to 1
         *  when APT:PrepareToStartApplication is used with flags bit8 is set.
         */
        void CheckNew3DSApp(Kernel::HLERequestContext& ctx);

        /**
         * Wrapper for PTMSYSM:CheckNew3DS
         * APT::CheckNew3DS service function
         *  Outputs:
         *      1: Result code, 0 on success, otherwise error code
         *      2: u8 output: 0 = Old3DS, 1 = New3DS.
         */
        void CheckNew3DS(Kernel::HLERequestContext& ctx);

    private:
        std::shared_ptr<Module> apt;
        bool application_reset_prepared{};
    };

private:
    bool LoadSharedFont();
    bool LoadLegacySharedFont();

    Core::System& system;

    /// Handle to shared memory region designated to for shared system font
    Kernel::SharedPtr<Kernel::SharedMemory> shared_font_mem;
    bool shared_font_loaded = false;
    bool shared_font_relocated = false;

    Kernel::SharedPtr<Kernel::Mutex> lock;

    u32 cpu_percent = 0; ///< CPU time available to the running application

    // APT::CheckNew3DSApp will check this unknown_ns_state_field to determine processing mode
    u8 unknown_ns_state_field = 0;

    std::vector<u8> screen_capture_buffer;

    ScreencapPostPermission screen_capture_post_permission =
        ScreencapPostPermission::CleanThePermission; // TODO(JamePeng): verify the initial value

    std::shared_ptr<AppletManager> applet_manager;
};

void InstallInterfaces(Core::System& system);

} // namespace Service::APT
