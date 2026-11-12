#include "game/pc/pcinput.h"
#include <cstring>
#include <cstdlib>

// Static device instances
InputDevice *InputDevice::s_devices[4] = {nullptr, nullptr, nullptr, nullptr};
int InputDevice::s_numDevices = 0;
IDirectInput8A *InputDevice::s_pDI = nullptr;

// 0x0040f800 (PDB: 0x0040ff30)
InputDevice *InputDevice::GetDevicePtr(int index)
{
    if (index >= 0 && index < 4) {
        return s_devices[index];
    }
    return nullptr;
}

// 0x005e6180 in PDB
int InputDevice::GetNumDevices()
{
    return s_numDevices;
}

// 0x0040f750 (PDB: 0x0040ff00)
bool InputDevice::IsMouseEnabled()
{
    if (s_devices[0] != nullptr) {
        PCCombinationInputDevice *comb = (PCCombinationInputDevice *)s_devices[0];
        return comb->m_bIsMouseEnabled;
    }
    return false;
}

// 0x0040f760 (PDB: 0x0040ff10)
bool InputDevice::EnableMouseButtons(bool enable)
{
    if (s_devices[0] != nullptr) {
        PCCombinationInputDevice *comb = (PCCombinationInputDevice *)s_devices[0];
        bool prev = !comb->m_bDisableMouseButtons;
        comb->m_bDisableMouseButtons = !enable;
        return prev;
    }
    return false;
}

// 0x00411930 in PDB
void InputDevice::EnableMouse(bool enable)
{
    if (s_devices[0] != nullptr) {
        PCCombinationInputDevice *comb = (PCCombinationInputDevice *)s_devices[0];
        comb->m_bIsMouseEnabled = enable;
    }
}

// 0x00411850 (PDB: 0x00411850)
void InputDevice::Create()
{
    if (s_devices[0] != nullptr) {
        return;
    }

    PCKeyboardDevice *keyb = new PCKeyboardDevice();
    PCMouseDevice *mouse = new PCMouseDevice();
    PCGamepadDevice *pad = new PCGamepadDevice();

    s_devices[0] = new PCCombinationInputDevice(keyb, mouse, pad);
    s_numDevices = 1;
}

// 0x0040fec0 in PDB
void InputDevice::Destroy()
{
    for (int i = 0; i < 4; i++) {
        if (s_devices[i] != nullptr) {
            delete s_devices[i];
            s_devices[i] = nullptr;
        }
    }
    s_numDevices = 0;
}

// 0x004702f0 in PDB
void InputDevice::Update()
{
    // Frame tick update
}

// PCSubInputDevice Implementation
PCSubInputDevice::PCSubInputDevice()
    : m_pDIDevice(nullptr)
    , m_numButtons(0)
    , m_numAxis(0)
    , m_lastTimeStamp(0)
{
    memset(m_buttonOffsets, 0, sizeof(m_buttonOffsets));
    memset(m_axisOffsets, 0, sizeof(m_axisOffsets));
}

// 0x004101c0 in PDB
PCSubInputDevice::~PCSubInputDevice()
{
    m_pDIDevice = nullptr;
}

// 0x00412a80 in PDB
const char *PCSubInputDevice::GetDeviceUniqueID()
{
    return "";
}

// 0x0061c550 in PDB
bool PCSubInputDevice::IsPresent()
{
    return true;
}

// 0x0040ff40 in PDB
uint32_t PCSubInputDevice::GetTimeStamp()
{
    return m_lastTimeStamp;
}

// 0x0040ff50 in PDB
int PCSubInputDevice::GetNumButtons()
{
    return m_numButtons;
}

// 0x0040ff60 in PDB
int PCSubInputDevice::GetNumAxis()
{
    return m_numAxis;
}

// 0x0040ff70 in PDB
cdc::UTF8String *PCSubInputDevice::GetButtonName(int button)
{
    if (button >= 0 && button < 256) {
        return &m_buttonNames[button];
    }
    return nullptr;
}

// 0x0040ff80 in PDB
cdc::UTF8String *PCSubInputDevice::GetAxisName(int axis)
{
    if (axis >= 0 && axis < 16) {
        return &m_axisNames[axis];
    }
    return nullptr;
}

// 0x00412a80 in PDB
int PCSubInputDevice::GetNumMotors()
{
    return 0;
}

// 0x0040ff90 in PDB
int PCSubInputDevice::GetMotorCaps(int motor)
{
    (void)motor;
    return 0;
}

// 0x0055d060 in PDB
void PCSubInputDevice::SetMotorIntensity(int motor, float intensity)
{
    (void)motor;
    (void)intensity;
}

// 0x004702f0 in PDB
void PCSubInputDevice::ApplyIntensities()
{
}

// PCKeyboardDevice Implementation
PCKeyboardDevice::PCKeyboardDevice()
{
    m_numButtons = 104;
    m_numAxis = 0;
}

// 0x00411490 in PDB
int PCKeyboardDevice::Poll(UpdateInfo *info, int maxEvents, bool raw)
{
    (void)info;
    (void)maxEvents;
    (void)raw;
    return 0;
}

// PCMouseDevice Implementation
PCMouseDevice::PCMouseDevice()
{
    m_numButtons = 3;
    m_numAxis = 3;
}

// 0x004111d0 in PDB
int PCMouseDevice::Poll(UpdateInfo *info, int maxEvents, bool raw)
{
    (void)info;
    (void)maxEvents;
    (void)raw;
    return 0;
}

// PCGamepadDevice Implementation
PCGamepadDevice::PCGamepadDevice()
{
    m_numButtons = 16;
    m_numAxis = 6;
}

// 0x00410a30 in PDB
int PCGamepadDevice::Poll(UpdateInfo *info, int maxEvents, bool raw)
{
    (void)info;
    (void)maxEvents;
    (void)raw;
    return 0;
}

// PCCombinationInputDevice Implementation
PCCombinationInputDevice::PCCombinationInputDevice(PCKeyboardDevice *keyb, PCMouseDevice *mouse, PCGamepadDevice *pad)
    : m_bIsMouseEnabled(true)
    , m_bDisableMouseButtons(false)
    , m_pOverrideButtonNames(nullptr)
    , m_pKeybDev(keyb)
    , m_pMouseDev(mouse)
    , m_pGamepadDev(pad)
{
}

// 0x00410100 in PDB
PCCombinationInputDevice::~PCCombinationInputDevice()
{
    if (m_pKeybDev != nullptr) {
        delete m_pKeybDev;
        m_pKeybDev = nullptr;
    }
    if (m_pMouseDev != nullptr) {
        delete m_pMouseDev;
        m_pMouseDev = nullptr;
    }
    if (m_pGamepadDev != nullptr) {
        delete m_pGamepadDev;
        m_pGamepadDev = nullptr;
    }
    if (m_pOverrideButtonNames != nullptr) {
        delete[] m_pOverrideButtonNames;
        m_pOverrideButtonNames = nullptr;
    }
}

// 0x0040fb20 in PDB
const char *PCCombinationInputDevice::GetDeviceUniqueID()
{
    return "PC_COMBINED_INPUT";
}

// 0x0061c550 in PDB
bool PCCombinationInputDevice::IsPresent()
{
    return true;
}

// 0x0040fb40 in PDB
uint32_t PCCombinationInputDevice::GetTimeStamp()
{
    if (m_pKeybDev != nullptr) {
        return m_pKeybDev->GetTimeStamp();
    }
    return 0;
}

// 0x0040fb50 in PDB
int PCCombinationInputDevice::GetNumMotors()
{
    if (m_pGamepadDev != nullptr) {
        return m_pGamepadDev->GetNumMotors();
    }
    return 0;
}

// 0x0040fb70 in PDB
int PCCombinationInputDevice::GetMotorCaps(int motor)
{
    if (m_pGamepadDev != nullptr) {
        return m_pGamepadDev->GetMotorCaps(motor);
    }
    return 0;
}

// 0x0040fb90 in PDB
void PCCombinationInputDevice::SetMotorIntensity(int motor, float intensity)
{
    if (m_pGamepadDev != nullptr) {
        m_pGamepadDev->SetMotorIntensity(motor, intensity);
    }
}

// 0x0040fbb0 in PDB
void PCCombinationInputDevice::ApplyIntensities()
{
    if (m_pGamepadDev != nullptr) {
        m_pGamepadDev->ApplyIntensities();
    }
}

// 0x0040fbc0 in PDB
int PCCombinationInputDevice::GetNumButtons()
{
    int total = 0;
    if (m_pKeybDev != nullptr) total += m_pKeybDev->GetNumButtons();
    if (m_pMouseDev != nullptr) total += m_pMouseDev->GetNumButtons();
    if (m_pGamepadDev != nullptr) total += m_pGamepadDev->GetNumButtons();
    return total;
}

// 0x0040fbd0 in PDB
int PCCombinationInputDevice::GetNumAxis()
{
    int total = 0;
    if (m_pMouseDev != nullptr) total += m_pMouseDev->GetNumAxis();
    if (m_pGamepadDev != nullptr) total += m_pGamepadDev->GetNumAxis();
    return total;
}

// 0x0040fbe0 in PDB
cdc::UTF8String *PCCombinationInputDevice::GetButtonName(int button)
{
    if (m_pOverrideButtonNames != nullptr) {
        return &m_pOverrideButtonNames[button];
    }
    if (m_pKeybDev != nullptr && button < m_pKeybDev->GetNumButtons()) {
        return m_pKeybDev->GetButtonName(button);
    }
    return nullptr;
}

// 0x0040fc90 in PDB
cdc::UTF8String *PCCombinationInputDevice::GetAxisName(int axis)
{
    if (m_pGamepadDev != nullptr && axis < m_pGamepadDev->GetNumAxis()) {
        return m_pGamepadDev->GetAxisName(axis);
    }
    return nullptr;
}

// 0x0040fd20 in PDB
int PCCombinationInputDevice::Poll(UpdateInfo *info, int maxEvents, bool raw)
{
    int eventCount = 0;
    if (m_pKeybDev != nullptr && eventCount < maxEvents) {
        eventCount += m_pKeybDev->Poll(info + eventCount, maxEvents - eventCount, raw);
    }
    if (m_pMouseDev != nullptr && eventCount < maxEvents && m_bIsMouseEnabled) {
        eventCount += m_pMouseDev->Poll(info + eventCount, maxEvents - eventCount, raw);
    }
    if (m_pGamepadDev != nullptr && eventCount < maxEvents) {
        eventCount += m_pGamepadDev->Poll(info + eventCount, maxEvents - eventCount, raw);
    }
    return eventCount;
}

// 0x0040ffa0 in PDB
void PCCombinationInputDevice::SetOverrideButtonName(int button, const cdc::UTF8String *name)
{
    if (button < 0 || button >= 256 || name == nullptr) {
        return;
    }
    if (m_pOverrideButtonNames == nullptr) {
        m_pOverrideButtonNames = new cdc::UTF8String[256];
    }
    m_pOverrideButtonNames[button] = *name;
}
