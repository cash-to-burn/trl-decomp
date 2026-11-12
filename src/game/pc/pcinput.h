#pragma once

#ifndef GAME_PC_PCINPUT_H
#define GAME_PC_PCINPUT_H

#include "common.h"
#include "cdc/cdcSys/UTF8String.h"

// Forward declaration of DirectInput interfaces
struct IDirectInput8A;
struct IDirectInputDevice8A;

class InputDevice {
public:
    struct UpdateInfo {
        int type;           // 1 = button, 2 = axis
        int index;          // button or axis index
        uint32_t timeStamp;
        float value;        // 0.0f - 1.0f (button) or -1.0f - 1.0f (axis)
    };

    virtual ~InputDevice() {}

    virtual const char *GetDeviceUniqueID() = 0;
    virtual bool IsPresent() = 0;
    virtual uint32_t GetTimeStamp() = 0;
    virtual int GetNumMotors() = 0;
    virtual int GetMotorCaps(int motor) = 0;
    virtual void SetMotorIntensity(int motor, float intensity) = 0;
    virtual void ApplyIntensities() = 0;
    virtual int GetNumButtons() = 0;
    virtual int GetNumAxis() = 0;
    virtual cdc::UTF8String *GetButtonName(int button) = 0;
    virtual cdc::UTF8String *GetAxisName(int axis) = 0;
    virtual int Poll(UpdateInfo *info, int maxEvents, bool raw) = 0;
    virtual void SetOverrideButtonName(int button, const cdc::UTF8String *name) {}

    // Static device factory and singleton management
    static void Create();
    static void Destroy();
    static void Update();
    static InputDevice *GetDevicePtr(int index = 0);
    static int GetNumDevices();
    static void EnableMouse(bool enable);
    static bool IsMouseEnabled();
    static bool EnableMouseButtons(bool enable);

protected:
    static InputDevice *s_devices[4];
    static int s_numDevices;
    static IDirectInput8A *s_pDI;
};

class PCSubInputDevice : public InputDevice {
public:
    PCSubInputDevice();
    virtual ~PCSubInputDevice();

    virtual const char *GetDeviceUniqueID() override;
    virtual bool IsPresent() override;
    virtual uint32_t GetTimeStamp() override;
    virtual int GetNumMotors() override;
    virtual int GetMotorCaps(int motor) override;
    virtual void SetMotorIntensity(int motor, float intensity) override;
    virtual void ApplyIntensities() override;
    virtual int GetNumButtons() override;
    virtual int GetNumAxis() override;
    virtual cdc::UTF8String *GetButtonName(int button) override;
    virtual cdc::UTF8String *GetAxisName(int axis) override;

protected:
    IDirectInputDevice8A *m_pDIDevice;
    cdc::UTF8String m_buttonNames[256];
    int m_buttonOffsets[256];
    cdc::UTF8String m_axisNames[16];
    int m_axisOffsets[16];
    int m_numButtons;
    int m_numAxis;
    uint32_t m_lastTimeStamp;
};

class PCKeyboardDevice : public PCSubInputDevice {
public:
    PCKeyboardDevice();
    virtual ~PCKeyboardDevice() {}
    virtual int Poll(UpdateInfo *info, int maxEvents, bool raw) override;
};

class PCMouseDevice : public PCSubInputDevice {
public:
    PCMouseDevice();
    virtual ~PCMouseDevice() {}
    virtual int Poll(UpdateInfo *info, int maxEvents, bool raw) override;
};

class PCGamepadDevice : public PCSubInputDevice {
public:
    PCGamepadDevice();
    virtual ~PCGamepadDevice() {}
    virtual int Poll(UpdateInfo *info, int maxEvents, bool raw) override;
};

class PCCombinationInputDevice : public InputDevice {
public:
    PCCombinationInputDevice(PCKeyboardDevice *keyb, PCMouseDevice *mouse, PCGamepadDevice *pad);
    virtual ~PCCombinationInputDevice();

    virtual const char *GetDeviceUniqueID() override;
    virtual bool IsPresent() override;
    virtual uint32_t GetTimeStamp() override;
    virtual int GetNumMotors() override;
    virtual int GetMotorCaps(int motor) override;
    virtual void SetMotorIntensity(int motor, float intensity) override;
    virtual void ApplyIntensities() override;
    virtual int GetNumButtons() override;
    virtual int GetNumAxis() override;
    virtual cdc::UTF8String *GetButtonName(int button) override;
    virtual cdc::UTF8String *GetAxisName(int axis) override;
    virtual int Poll(UpdateInfo *info, int maxEvents, bool raw) override;
    virtual void SetOverrideButtonName(int button, const cdc::UTF8String *name) override;

    bool m_bIsMouseEnabled;
    bool m_bDisableMouseButtons;
    cdc::UTF8String *m_pOverrideButtonNames;
    PCKeyboardDevice *m_pKeybDev;
    PCMouseDevice *m_pMouseDev;
    PCGamepadDevice *m_pGamepadDev;
};

#endif // GAME_PC_PCINPUT_H
