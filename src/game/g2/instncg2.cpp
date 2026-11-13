#include "game/g2/instncg2.h"
#include "game/g2/quatg2.h"
#include "game/anim/anim.h"
#include "game/anim/anitracker.h"
#include "game/camera/camera.h"
#include "game/collide.h"
#include "game/instance.h"
#include "game/main.h"
#include "game/obtable.h"
#include "game/player/player.h"
#include "game/spline.h"

#include <cmath>
#include <cstring>

// Scratchpad and cross-subsystem helpers
extern void* getScratchAddr(int size);                                                                      // 0x004019c0 (PDB: 0x004019f0)
extern void freeScratchAddr(int size);                                                                      // 0x004019f0 (PDB: 0x00401a20)
extern void TRANS_BuildWeightedMatrix(cdc::Matrix* dst, cdc::Matrix* src1, cdc::Matrix* src2, float weight); // 0x00402e90 (PDB: 0x00403400)
extern void TRANS_ApplyWeightedOffset(cdc::Matrix* dst, cdc::Matrix* src, cdc::Vector3* offset, float weight); // 0x00402f90 (PDB: 0x00403500)
extern void CLOTH_UpdateAllCloth(Instance* inst);                                                           // 0x00435730 (PDB: 0x00436a70)

struct AnimatedGeoms {
    void UpdateGeomTransforms(BaseInstance* inst);                                                          // 0x005561e0 (PDB: 0x005582d0)
};

struct CharacterProxy {
    void SetZRotation(float rotZ);                                                                          // 0x005502a0 (PDB: 0x005522a0)
    void SetPosition(const cdc::Vector3& pos);                                                              // 0x005502b0 (PDB: 0x005522b0)
    void MoveToNewSettings();                                                                               // 0x00551940 (PDB: 0x00553940)
};

// Size: 0x08 (TI: 0x65d1)
class CinematicHandlerHost {
public:
    virtual ~CinematicHandlerHost() = default;
    virtual bool HasCinematic() = 0;
    virtual bool HasSubtitles() = 0;
    virtual bool IsCinematicPlaying() = 0;
    virtual float GetMediaTime() = 0;
    virtual bool IsPlaying() = 0;
    virtual bool IsCueing() = 0;
    virtual void ActivateInstances() = 0;
    virtual bool DirectInstance(Instance* inst) = 0;

    /* 0x04 */ uint8_t mSubtitleMode;
};

namespace {

constexpr int kScratchSegValueBytes = 0x2d00;

void ApplyVirtualSegments(BaseInstance* inst, const Model* model)
{
    const int numSegments = model->numSegments;
    const int numVirtSegments = model->numVirtSegments;
    if (numVirtSegments <= 0) {
        return;
    }

    cdc::Matrix* matrices = inst->matrix;
    cdc::Matrix* dst = &matrices[numSegments];
    const VirtSegment* virtSegs = reinterpret_cast<const VirtSegment*>(&model->segmentList[numSegments]);
    const bool zeroWeights = (g_gameTracker.debugFlags2 & 0x08000000) != 0;

    for (int i = 0; i < numVirtSegments; ++i) {
        cdc::Matrix* mat1 = &matrices[virtSegs[i].index];
        cdc::Matrix* mat2 = &matrices[virtSegs[i].weightIndex];
        const float weight = zeroWeights ? 0.0f : virtSegs[i].weight;
        TRANS_BuildWeightedMatrix(dst, mat1, mat2, weight);
        cdc::Vector3 pivot = virtSegs[i].pivot;
        TRANS_ApplyWeightedOffset(dst, mat2, &pivot, weight);
        ++dst;
    }
}

void ApplyParentLinkTransform(BaseInstance* inst, cdc::Matrix* rootMat, bool animated)
{
    BaseInstance* linkParent = inst->LinkParent;
    if (inst->linkMarkerIndex != -1) {
        if (COLLIDE_GetHMarkerTransformFromID(linkParent, inst->linkMarkerIndex, inst->matrix, false) == 0) {
            inst->linkMarkerIndex = -1;
            *inst->matrix = *linkParent->matrix;
        }
        if (animated) {
            *rootMat = *inst->matrix;
        }
        inst->position = *reinterpret_cast<const cdc::Vector3*>(&inst->matrix->col3);
        return;
    }

    if (animated) {
        cdc::Matrix identityMat;
        identityMat.Identity();
        rootMat->Identity();
        cdc::OrthonormalInverse3x3(rootMat, identityMat);
        *inst->matrix = linkParent->matrix[inst->ParentLinkNode];
        cdc::Mul4x4(rootMat, *inst->matrix, *rootMat);
        inst->position = *reinterpret_cast<const cdc::Vector3*>(&inst->matrix->col3);
    } else {
        *rootMat = linkParent->matrix[inst->ParentLinkNode];
        inst->position = *reinterpret_cast<const cdc::Vector3*>(&rootMat->col3);
    }
}

template <typename VecA, typename VecB>
bool VectorsDiffer(const VecA& a, const VecB& b)
{
    return a.x != b.x || a.y != b.y || a.z != b.z;
}

void CopyTranslation(cdc::Vector3& dst, const cdc::Vector& src)
{
    dst.x = src.x;
    dst.y = src.y;
    dst.z = src.z;
    dst.w = src.w;
}

} // namespace

// 0x004702f0
void G2Instance_Reset()
{
}

// 0x004dee70 (PDB: 0x004e09a0)
AnimFragment* G2Instance_GetKeylist(BaseInstance* inst, int keylistID)
{
    return G2Instance_GetKeylistFromObject(inst->object, keylistID);
}

// 0x004deea0 (PDB: 0x004e09d0)
AnimFragment* G2Instance_GetKeylistFromObject(Object* obj, int keylistID)
{
    const AnimListEntry* entry = &static_cast<const AnimListEntry*>(obj->animList)[keylistID];
    if (entry->animationID == 0) {
        return nullptr;
    }
    return aniTracker[entry->animationNumber].animationData;
}

// 0x004deed0 (PDB: 0x004e0a00)
void G2Instance_BuildClothTransforms(Instance* inst)
{
    if ((inst->iflags3 & 0x1) != 0) {
        CLOTH_UpdateAllCloth(inst);
    }
}

// 0x004deef0 (PDB: 0x004e0a20)
void G2Instance_BuildVirtualTransforms(Instance* inst)
{
    const Model* model = inst->animComponent->mModel;
    ApplyVirtualSegments(inst, model);
    inst->iflags &= ~0x4000;
}

// 0x004defd0 (PDB: 0x004e0b00)
void G2Instance_NextKeyframe(BaseInstance* inst)
{
    _segValuesAnim = nullptr;
    _segValues = static_cast<G2AnimSegValue*>(getScratchAddr(kScratchSegValueBytes));
    inst->animComponent->mAnimProcessor->NextKeyframe();
    _segValuesAnim = nullptr;
    _segValues = _segValuesStatic;
    freeScratchAddr(kScratchSegValueBytes);
}

// 0x004df020 (PDB: 0x004e0b50)
int G2Instance_GetNumSections(Instance* inst)
{
    const Object* obj = inst->object;
    int count = 0;
    if (obj->sectionA != -1) {
        ++count;
    }
    if (obj->sectionB != -1) {
        ++count;
    }
    if (obj->sectionC != -1) {
        ++count;
    }
    if (count == 0) {
        count = 1;
    }
    return count;
}

// 0x004df060 (PDB: 0x004e0b90)
void _G2Instance_RebuildAnimatedTransforms(Instance* inst)
{
    cdc::Matrix* segMatrices = inst->matrix;
    if (segMatrices == nullptr) {
        return;
    }

    const Model* model = reinterpret_cast<Model**>(inst->object->modelList)[inst->currentRenderModel];
    cdc::Matrix* rootMat = segMatrices - 1;

    if ((inst->iflags & 0x1) != 0 && inst->intro != nullptr) {
        *rootMat = inst->intro->multiSpline->curRotMatrix;
    } else if (inst->LinkParent != nullptr) {
        ApplyParentLinkTransform(inst, rootMat, true);
    } else {
        rootMat->Build_XYZOrder(inst->rotation);
    }

    rootMat->Scale(inst->scale);
    rootMat->SetTrans(inst->position);

    if (g_gameTracker.activeCinematic == nullptr || !g_gameTracker.activeCinematic->DirectInstance(inst)) {
        inst->animComponent->mAnimProcessor->mSegMatrices = inst->matrix;
        if (inst->animComponent != nullptr) {
            inst->animComponent->mBlendProcessor.BuildTransforms();
        }
    }

    inst->iflags |= 0x4000;

    if (inst->LinkParent != nullptr && inst->linkMarkerIndex == -1) {
        cdc::Matrix* matrices = inst->matrix;
        const cdc::Vector3 offset = cdc::Vector3::Make(
            matrices[0].col3.x - matrices[3].col3.x,
            matrices[0].col3.y - matrices[3].col3.y,
            matrices[0].col3.z - matrices[3].col3.z);

        AnimProcessor* animProc = inst->animComponent->mAnimProcessor;
        for (int i = 0; i < model->numSegments; ++i) {
            if (animProc->IsControllerActive(i, 0x20)) {
                break;
            }
            matrices[i].col3.x += offset.x;
            matrices[i].col3.y += offset.y;
            matrices[i].col3.z += offset.z;
        }

        rootMat->SetTrans(*reinterpret_cast<const cdc::Vector3*>(&inst->matrix[0].col3));
    }

    CopyTranslation(inst->position, rootMat->col3);

    AnimProcessor* animProc = inst->animComponent->mAnimProcessor;
    if (inst == g_gameTracker.playerInstance && gPlayer->currentRope != nullptr) {
        _G2EulerAngles_Type euler;
        G2EulerAngles_FromMatrixf(&euler, rootMat, 0x15);
        inst->rotation.x = euler.x;
        inst->rotation.y = euler.y;
        inst->rotation.z = euler.z;
        inst->rotation.w = 0.0f;
        animProc->mFlags &= ~0x80;
        return;
    }

    if ((animProc->mFlags & 0x80) != 0 || inst->multibodyRagdoll != nullptr) {
        _G2EulerAngles_Type euler;
        G2EulerAngles_FromMatrixf(&euler, rootMat, 0x15);
        if (std::fabs(euler.x - inst->rotation.x) > 0.01f ||
            std::fabs(euler.y - inst->rotation.y) > 0.01f ||
            std::fabs(euler.z - inst->rotation.z) > 0.01f) {
            inst->rotation.x = euler.x;
            inst->rotation.y = euler.y;
            inst->rotation.z = euler.z;
            inst->rotation.w = 0.0f;
        }
        animProc->mFlags &= ~0x80;
    }
}

// 0x004df4b0 (PDB: 0x004e0fe0)
void G2Instance_SetTransformsToIdentity(BaseInstance* inst)
{
    const Object* obj = inst->object;
    if (obj->oflags < 0 || (inst->iflags & 0x400000) != 0) {
        inst->curMatrixPool ^= 1;
        inst->oldMatrix = inst->matrix;
        cdc::Matrix* mat = inst->matrixPool[static_cast<uint8_t>(inst->curMatrixPool) & 1];
        inst->matrix = mat;
        mat->Build_XYZOrder(inst->rotation);
        mat->Scale(inst->scale);
        mat->SetTrans(inst->position);
        if (inst->oldMatrix != nullptr) {
            *inst->oldMatrix = *mat;
        }
        return;
    }

    inst->curMatrixPool ^= 1;
    inst->oldMatrix = inst->matrix;
    cdc::Matrix* baseMat = inst->matrixPool[static_cast<uint8_t>(inst->curMatrixPool) & 1];
    cdc::Matrix* segMatrices = baseMat + 1;
    inst->matrix = segMatrices;

    baseMat->Build_XYZOrder(inst->rotation);
    baseMat->Scale(inst->scale);
    baseMat->SetTrans(inst->position);

    const Model* model = reinterpret_cast<Model**>(inst->object->modelList)[inst->currentRenderModel];
    const Segment* seg = model->segmentList;
    cdc::Matrix* dst = segMatrices;

    for (int i = 0; i < model->numSegments; ++i) {
        const cdc::Matrix& parent = inst->matrix[seg[i].parent];
        dst->col0 = parent.col0;
        dst->col1 = parent.col1;
        dst->col2 = parent.col2;
        dst->col3 = cdc::Vector3::Make(
            parent.col3.x + seg[i].pivot.x,
            parent.col3.y + seg[i].pivot.y,
            parent.col3.z + seg[i].pivot.z,
            1.0f);
        ++dst;
    }

    ApplyVirtualSegments(inst, model);

    if (inst->oldMatrix != nullptr) {
        const int totalSegments = model->numSegments + model->numVirtSegments;
        for (int i = 0; i < totalSegments; ++i) {
            inst->oldMatrix[i] = inst->matrix[i];
        }
    }
}

// 0x004df760 (PDB: 0x004e1290)
void _G2Instance_BuildFacadeTransforms(Instance* inst, Segment* segment, cdc::Matrix* segMat, cdc::Matrix* parentMat)
{
    const cdc::Vector3 camPos = theCamera.core.position;
    cdc::Vector3 pivotPos;
    if (segment->pivot.x != 0.0f || segment->pivot.y != 0.0f || segment->pivot.z != 0.0f) {
        const cdc::Vector3 rotatedPivot = cdc::Mul3x3(*parentMat, segment->pivot);
        pivotPos = cdc::Vector3::Make(
            rotatedPivot.x + inst->position.x,
            rotatedPivot.y + inst->position.y,
            rotatedPivot.z + inst->position.z);
    } else {
        pivotPos = inst->position;
    }

    segMat->SetTrans(pivotPos);

    if ((segment->flags & 1) != 0) {
        const cdc::Vector& zAxis = parentMat->col2;
        cdc::Euler euler;
        euler.x = -std::atan2(zAxis.y, zAxis.z);
        euler.y = std::atan2(zAxis.x, std::sqrt(1.0f - zAxis.y * zAxis.y));
        euler.z = 0.0f;
        euler.w = 0.0f;

        cdc::Matrix orientMat;
        orientMat.Build_XYZOrder(euler);
        cdc::Matrix invMat;
        cdc::OrthonormalInverse3x3(&invMat, orientMat);

        const cdc::Vector3 toCam = cdc::Vector3::Make(
            camPos.x - pivotPos.x,
            camPos.y - pivotPos.y,
            camPos.z - pivotPos.z);
        const cdc::Vector3 localDir = cdc::Mul3x3(invMat, toCam);

        segMat->Identity();
        segMat->Scale(inst->scale);
        const float yaw = std::atan2(localDir.y, localDir.x) + 0.25f;
        segMat->RotZ(std::sin(yaw), std::cos(yaw));
        segMat->RotY(std::sin(euler.y), std::cos(euler.y));
        segMat->RotX(std::sin(euler.x), std::cos(euler.x));
    } else if ((segment->flags & 2) != 0) {
        const float dx = camPos.x - pivotPos.x;
        const float dy = camPos.y - pivotPos.y;
        const float dz = camPos.z - pivotPos.z;
        const float horiz = std::sqrt(dx * dx + dy * dy);
        const float pitch = std::atan2(dz, horiz) + 0.75f;
        const float yaw = std::atan2(dy, dx) + 0.25f;

        segMat->Identity();
        segMat->Scale(inst->scale);
        segMat->RotX(std::sin(pitch), std::cos(pitch));
        segMat->RotZ(std::sin(yaw), std::cos(yaw));
    }
}

// 0x004dfa40 (PDB: 0x004e1570)
void _G2Instance_RebuildNonAnimatedTransforms(Instance* inst)
{
    cdc::Matrix* segMat = inst->matrix;
    if (segMat == nullptr) {
        return;
    }

    const Model* model = reinterpret_cast<Model**>(inst->object->modelList)[inst->currentRenderModel];
    Segment* seg = model->segmentList;

    for (int i = 0; i < model->numSegments; ++i) {
        if ((seg->flags & 0x3) != 0) {
            cdc::Matrix parentMat;
            parentMat.Build_XYZOrder(inst->rotation);
            parentMat.Scale(inst->scale);
            _G2Instance_BuildFacadeTransforms(inst, seg, segMat, &parentMat);
        } else {
            if ((inst->iflags & 0x1) != 0 && inst->intro != nullptr) {
                *segMat = inst->intro->multiSpline->curRotMatrix;
            } else if (inst->LinkParent != nullptr) {
                ApplyParentLinkTransform(inst, segMat, false);
            } else if (inst->rotation.x != 0.0f || inst->rotation.y != 0.0f || inst->rotation.z != 0.0f) {
                segMat->Build_XYZOrder(inst->rotation);
            } else {
                segMat->Identity();
            }

            segMat->Scale(inst->scale);
            segMat->SetTrans(inst->position);
        }

        ++seg;
        ++segMat;
    }
}

// 0x004dfc60 (PDB: 0x004e1790)
void G2Instance_BuildTransforms(Instance* inst)
{
    if ((inst->iflags2 & 0x10000000) != 0) {
        if ((inst->iflags2 & 0x04000000) != 0) {
            return;
        }
        if ((inst->iflags2 & 0x20000000) != 0 && (inst->iflags & 0x800) != 0) {
            return;
        }
    }

    cdc::Matrix* mat = inst->matrix;
    cdc::Matrix* oldMat = inst->oldMatrix;
    bool skipRebuild = true;

    if (mat == nullptr || oldMat == nullptr || inst->LinkParent != nullptr ||
        oldMat->col3.x != mat->col3.x || oldMat->col3.y != mat->col3.y || oldMat->col3.z != mat->col3.z ||
        VectorsDiffer(inst->position, inst->oldPos) ||
        VectorsDiffer(inst->rotation, inst->oldRotation)) {
        skipRebuild = false;
    } else {
        const Object* obj = inst->object;
        if ((obj->oflags >= 0 && (obj->oflags & 0x800000) == 0 &&
             (inst->animComponent->mAnimProcessor->mFlags & 0x3) != 0) ||
            (inst->iflags & 0x100000) == 0 ||
            (inst->iflags3 & 0x1) != 0 ||
            inst->multibodyRagdoll != nullptr) {
            skipRebuild = false;
        }
    }

    if ((inst->iflags3 & 0x40) != 0) {
        skipRebuild = true;
    }

    const bool canSkip = (inst->iflags & 0x200000) == 0 &&
        inst->iflags2 >= 0 &&
        mat != nullptr &&
        oldMat != nullptr &&
        ((inst->iflags2 & 0x1) != 0 || (g_gameTracker.streamFlags & 0x1000) != 0 || skipRebuild);

    if (!canSkip) {
        inst->oldMatrix = mat;
        inst->curMatrixPool ^= 1;
        const int poolIdx = static_cast<uint8_t>(inst->curMatrixPool) & 1;

        if (inst->object->oflags >= 0) {
            inst->matrix = inst->matrixPool[poolIdx] + 1;
            _G2Instance_RebuildAnimatedTransforms(inst);
        } else {
            inst->matrix = inst->matrixPool[poolIdx];
            _G2Instance_RebuildNonAnimatedTransforms(inst);
        }

        inst->iflags |= 0x4000;
        if (inst->iflags2 < 0) {
            const Model* baseModel = reinterpret_cast<Model**>(inst->object->modelList)[inst->currentBaseModel];
            std::memcpy(inst->oldMatrix, inst->matrix, static_cast<size_t>(baseModel->numSegments) * sizeof(cdc::Matrix));
            inst->iflags2 &= ~0x80000000;
        }

        if (inst->animatedGeoms != nullptr) {
            inst->animatedGeoms->UpdateGeomTransforms(inst);
        }
    }

    if (inst->characterProxy != nullptr) {
        inst->characterProxy->SetZRotation(inst->rotation.z);
        if ((inst->object->oflags2 & 0x100000) != 0) {
            inst->characterProxy->SetPosition(inst->position);
        } else {
            const cdc::Vector3 segPos = cdc::Vector3::Make(
                inst->matrix[1].col3.x,
                inst->matrix[1].col3.y,
                inst->matrix[1].col3.z,
                inst->matrix[1].col3.w);
            inst->characterProxy->SetPosition(segPos);
        }
        inst->characterProxy->MoveToNewSettings();
    }

    for (Instance* child = inst->LinkChild; child != nullptr; child = child->LinkSibling) {
        G2Instance_BuildTransforms(child);
    }
}

// 0x004dff40 (PDB: 0x004e1a70)
void G2Instance_RebuildTransforms(Instance* inst)
{
    if (inst->object->oflags >= 0) {
        _G2Instance_RebuildAnimatedTransforms(inst);
    } else {
        _G2Instance_RebuildNonAnimatedTransforms(inst);
    }
}

// 0x004dff60 (PDB: 0x004e1a90)
void G2Instance_BuildTransformsForList(Instance* inst)
{
    _segValues = static_cast<G2AnimSegValue*>(getScratchAddr(kScratchSegValueBytes));
    _segValuesAnim = nullptr;

    for (Instance* cur = inst; cur != nullptr; cur = static_cast<Instance*>(cur->next)) {
        if (cur->LinkParent == nullptr) {
            G2Instance_BuildTransforms(cur);
        }
    }

    _segValues = _segValuesStatic;
    _segValuesAnim = nullptr;
    freeScratchAddr(kScratchSegValueBytes);
}
