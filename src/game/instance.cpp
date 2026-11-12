#include "game/instance.h"
#include "game/obtable.h"
#include "game/gameloop.h"
#include "game/main.h"
#include "game/Stream/stream.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Globals
InstanceList gInstanceList = {};                        // 0x010c59e0 (PDB: ?gInstanceList@@3UInstanceList@@A)
Instance gInstancePool[207] = {};                       // 0x010c5e50 (PDB: ?gInstancePool@@3PAUInstance@@A)
short gInstanceIntroUniqueIDs[207] = {};                // 0x010c5b00 (PDB: ?gInstanceIntroUniqueIDs@@3PAFA)
short gInstanceCurrentStreamUnitIDs[207] = {};          // 0x010c5ca0 (PDB: ?gInstanceCurrentStreamUnitIDs@@3PAFA)

// Helper list insertion function
// 0x0045b990 (PDB: ?NODE_InsertNode@@YAXPAUNodeType@@0@Z)
static void NODE_InsertNode(NodeType* head, NodeType* node)
{
    if (head == nullptr || node == nullptr) return;
    node->next = head->next;
    node->prev = head;
    if (head->next != nullptr) {
        head->next->prev = node;
    }
    head->next = node;
}

// 0x004545b0 (PDB: 0x00455be0 / ?INSTANCE_InitInstanceList@@YAXXZ)
void INSTANCE_InitInstanceList(void)
{
    gInstanceList.instance = gInstancePool;

    // Link the 207 instance pool nodes into the free list
    for (int i = 0; i < 207; i++) {
        gInstancePool[i].next = (i < 206) ? &gInstancePool[i + 1] : nullptr;
        gInstancePool[i].prev = (i > 0) ? &gInstancePool[i - 1] : nullptr;
    }

    gInstancePool[0].prev = nullptr;
    gInstancePool[206].next = nullptr;

    gInstanceList.first_free = &gInstancePool[0];
    gInstanceList.numInstances = 0;
    gInstanceList.first = nullptr;
    gInstanceList.nextInstanceID = 1;

    // Clear all 32 instance group headers
    for (int i = 0; i < 32; i++) {
        gInstanceList.group[i].prev = nullptr;
        gInstanceList.group[i].next = nullptr;
    }

    // Clear unique ID and stream ID tracker arrays
    memset(gInstanceIntroUniqueIDs, 0, sizeof(gInstanceIntroUniqueIDs));
    memset(gInstanceCurrentStreamUnitIDs, 0, sizeof(gInstanceCurrentStreamUnitIDs));
}

// 0x00454670 (PDB: 0x00455ca0 / ?INSTANCE_NewInstance@@YAPAUInstance@@XZ)
Instance* INSTANCE_NewInstance(void)
{
    Instance* inst = gInstanceList.first_free;
    if (inst == nullptr) {
        return nullptr;
    }

    gInstanceList.first_free = inst->next;

    inst->next = gInstanceList.first;
    if (gInstanceList.first != nullptr) {
        gInstanceList.first->prev = inst;
    }
    gInstanceList.first = inst;
    inst->prev = nullptr;

    inst->instanceID = gInstanceList.nextInstanceID++;
    gInstanceList.numInstances++;

    return inst;
}

// 0x004546d0 (PDB: 0x00455d10 / ?INSTANCE_InsertInstanceGroup@@YAXPAUInstance@@@Z)
void INSTANCE_InsertInstanceGroup(Instance* instance)
{
    if (instance == nullptr || instance->object == nullptr) return;

    Object* obj = instance->object;
    int flags = obj->oflags;
    unsigned char groupIndex = 0;

    if (flags & 0x20) groupIndex = 1;
    if (flags & 0x08) groupIndex |= 2;
    if (flags & 0x10) groupIndex |= 4;
    if (flags & 0x04) groupIndex |= 0x10;
    if (flags & 0x01) groupIndex |= 0x08;

    instance->collisionFlags = static_cast<signed char>(groupIndex);

    NodeType* head = &gInstanceList.group[groupIndex];
    NODE_InsertNode(head, &instance->node);
}

// 0x004553f0 (PDB: 0x00456a30 / ?INSTANCE_Find@@YAPAUInstance@@H@Z)
Instance* INSTANCE_Find(int instanceID)
{
    for (int i = 0; i < 207; i++) {
        if (gInstanceIntroUniqueIDs[i] == static_cast<short>(instanceID)) {
            Instance* inst = &gInstancePool[i];
            if ((inst->iflags & 0x20) == 0) {
                return inst;
            }
            return nullptr;
        }
    }
    return nullptr;
}

// 0x00455bb0 (PDB: 0x004571f0 / ?INSTANCE_SetModel@@YAXPAUInstance@@H@Z)
void INSTANCE_SetModel(Instance* instance, int modelNum)
{
    if (instance == nullptr || instance->object == nullptr) return;

    Object* obj = instance->object;
    if (modelNum < 0 || modelNum >= obj->numModels || obj->modelList == nullptr) {
        return;
    }

    instance->currentRenderModel = static_cast<signed char>(modelNum);
    instance->currentBaseModel = static_cast<signed char>(modelNum);
}

// 0x00455db0 (PDB: 0x00457430 / ?INSTANCE_SetCurrentStreamUnitID@@YAXPAUInstance@@H@Z)
void INSTANCE_SetCurrentStreamUnitID(Instance* instance, int streamUnitID)
{
    if (instance == nullptr) return;

    instance->currentStreamUnitID = static_cast<unsigned char>(streamUnitID);

    // Calculate pool index
    ptrdiff_t offset = instance - gInstancePool;
    if (offset >= 0 && offset < 207) {
        gInstanceCurrentStreamUnitIDs[offset] = static_cast<short>(streamUnitID);
    }
}

// 0x00456770 (PDB: 0x00457df0 / ?INSTANCE_PlainDeath@@YAXPAUInstance@@@Z)
void INSTANCE_PlainDeath(Instance* instance)
{
    if (instance == nullptr) return;

    instance->iflags &= ~0x400; // 0xfffffbff

    Instance* attached = INSTANCE_Find(instance->attachedID);
    if (attached != nullptr) {
        attached->iflags2 &= ~0x80;
    }

    typedef void (*CleanupFunc)(Instance*);
    reinterpret_cast<CleanupFunc>(0x005be470)(instance);

    instance->iflags |= 0x20;
}

// 0x004567c0 (PDB: 0x00457e40 / ?INSTANCE_KillInstance@@YAXPAUInstance@@@Z)
void INSTANCE_KillInstance(Instance* instance)
{
    if (instance == nullptr) return;

    if ((instance->iflags & 0x20) == 0) {
        INSTANCE_PlainDeath(instance);
    }
}

// 0x00457a20 (PDB: 0x00457c60 / ?INSTANCE_BirthCommon@@YAXPAUInstance@@PAUObject@@HH@Z)
void INSTANCE_BirthCommon(Instance* instance, Object* obj, int modelNum, int introNum)
{
    if (instance == nullptr || obj == nullptr) return;

    int instID = instance->instanceID;

    // Clear 0x94 dwords starting from offset 0x10, then restore instanceID (0xec)
    char* startPtr = reinterpret_cast<char*>(instance) + 0x10;
    memset(startPtr, 0, 0x94 * sizeof(uint32_t));
    instance->instanceID = instID;

    instance->object = obj;
    instance->data = obj->data;
    instance->vvRemoveDist = obj->vvRemoveDist;
    instance->removeDist = obj->removeDist;
    instance->introUniqueID = introNum;

    INSTANCE_SetModel(instance, modelNum);

    // Record intro unique ID in lookup array
    ptrdiff_t index = instance - gInstancePool;
    if (index >= 0 && index < 207) {
        gInstanceIntroUniqueIDs[index] = static_cast<short>(introNum);
    }
}

// 0x004582a0 (PDB: 0x00459920 / ?INSTANCE_IntroduceInstance@@YAPAUInstance@@PAUIntro@@F_N@Z)
Instance* INSTANCE_IntroduceInstance(Intro* intro, short streamUnitID, bool flag)
{
    if (intro == nullptr) return nullptr;

    // Check if already introduced
    Instance* existing = INSTANCE_Find(intro->UniqueID);
    if (existing != nullptr) {
        if (!flag) return nullptr;
        return existing;
    }

    Object* obj = OBTABLE_FindObject(intro->objectID);
    if (obj == nullptr) {
        return nullptr;
    }

    Instance* inst = INSTANCE_NewInstance();
    if (inst == nullptr) {
        return nullptr;
    }

    intro->introFlags |= 0x80000000;

    INSTANCE_BirthCommon(inst, obj, 0, intro->UniqueID);

    inst->introNum = intro->intronum;
    inst->position = intro->position;
    inst->oldPos = intro->position;
    inst->rotation = intro->rotation;
    inst->oldRotation = intro->rotation;
    inst->scale = intro->scale;
    inst->intro = intro;
    inst->introData = intro->data;
    inst->birthStreamUnitID = streamUnitID;
    inst->currentStreamUnitID = static_cast<unsigned char>(streamUnitID);

    INSTANCE_SetCurrentStreamUnitID(inst, streamUnitID);

    OBTABLE_GetInstanceCollideFunc(inst);
    OBTABLE_GetInstanceAdditionalCollideFunc(inst);
    OBTABLE_GetInstanceProcessFunc(inst);
    OBTABLE_GetInstanceQueryFunc(inst);
    OBTABLE_GetInstanceMessageFunc(inst);
    OBTABLE_InstanceInit(inst);
    INSTANCE_InsertInstanceGroup(inst);

    return inst;
}

// 0x00458600 (PDB: 0x00459c80 / ?INSTANCE_BirthObjectNoParent@@YAPAUInstance@@IPAVVector3@cdc@@PAVEuler@3@PAUIntroData@@PAUObject@@HH@Z)
Instance* INSTANCE_BirthObjectNoParent(unsigned int streamUnitID, cdc::Vector3* pos, cdc::Euler* rot, IntroData* introData, Object* obj, int modelNum, int flag)
{
    (void)introData;
    (void)flag;

    if (obj == nullptr) {
        return nullptr;
    }

    Instance* inst = INSTANCE_NewInstance();
    if (inst == nullptr) {
        return nullptr;
    }

    static short s_introCounter = 0;
    short introNum = s_introCounter++;

    INSTANCE_BirthCommon(inst, obj, modelNum, introNum);

    if (pos != nullptr) {
        inst->position = *pos;
    }
    if (rot != nullptr) {
        inst->rotation = *rot;
    }

    inst->scale.x = 1.0f;
    inst->scale.y = 1.0f;
    inst->scale.z = 1.0f;
    inst->scale.w = 0.0f;

    INSTANCE_SetCurrentStreamUnitID(inst, streamUnitID);
    inst->birthStreamUnitID = streamUnitID;

    return inst;
}

// 0x00458750 (PDB: 0x00459dd0 / ?INSTANCE_BirthObject@@YAPAUInstance@@PAU1@PAUObject@@HH@Z)
Instance* INSTANCE_BirthObject(Instance* parent, Object* obj, int modelNum, int flag)
{
    if (obj == nullptr) {
        return nullptr;
    }

    Instance* inst = INSTANCE_NewInstance();
    if (inst == nullptr) {
        return nullptr;
    }

    static short s_introCounter = 0;
    short introNum = s_introCounter++;

    INSTANCE_BirthCommon(inst, obj, modelNum, introNum);

    if (parent != nullptr) {
        inst->position = parent->position;
        inst->rotation = parent->rotation;
        inst->scale = parent->scale;

        INSTANCE_SetCurrentStreamUnitID(inst, parent->currentStreamUnitID);
        inst->birthStreamUnitID = parent->birthStreamUnitID;
        inst->parent = parent;
        inst->intro = parent->intro;
        inst->introData = parent->introData;
    }

    (void)flag;
    return inst;
}

// 0x00458840 (PDB: 0x00459ec0 / ?INSTANCE_IntroduceSavedInstance@@YAPAUInstance@@PAUSavedIntro@@PAUStreamUnit@@PAH@Z)
Instance* INSTANCE_IntroduceSavedInstance(SavedIntro* savedIntro, StreamUnit* streamUnit, int* pOut)
{
    if (savedIntro == nullptr || streamUnit == nullptr) {
        if (pOut) *pOut = 0;
        return nullptr;
    }

    // Check if instance is already alive
    Instance* existing = INSTANCE_Find(savedIntro->introUniqueID);
    if (existing != nullptr) {
        if (pOut) *pOut = 0;
        return existing;
    }

    // Lookup Object for objectID
    Object* obj = OBTABLE_FindObject(savedIntro->objectID);
    if (obj == nullptr) {
        if (pOut) *pOut = 0;
        return nullptr;
    }

    cdc::Vector3 pos;
    pos.x = savedIntro->position.x;
    pos.y = savedIntro->position.y;
    pos.z = savedIntro->position.z;
    pos.w = 0.0f;

    cdc::Euler rot;
    rot.x = savedIntro->rotation.x;
    rot.y = savedIntro->rotation.y;
    rot.z = savedIntro->rotation.z;
    rot.w = 0.0f;

    Instance* inst = INSTANCE_BirthObjectNoParent(streamUnit->StreamUnitID, &pos, &rot, nullptr, obj, 0, 0);
    if (inst == nullptr) {
        if (pOut) *pOut = 0;
        return nullptr;
    }

    inst->introUniqueID = savedIntro->introUniqueID;
    inst->iflags |= savedIntro->flags;
    inst->iflags2 |= savedIntro->flags2;

    if (pOut) *pOut = 1;
    return inst;
}
