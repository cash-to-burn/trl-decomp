#include "game/anim/anim.h"

// 0x004779c0 (PDB: 0x00479340 / ??1AnimTarget@@UAE@XZ)
AnimTarget::~AnimTarget() = default;

// 0x00613790 (PDB: 0x00450370 / ?PickSequence@AnimTarget@@UAEXPAVAnimProcessor@@@Z)
void AnimTarget::PickSequence(AnimProcessor* /*animProcessor*/)
{
}

// 0x00477a10 (PDB: 0x005e7d10 / ?AddSequence@AnimTarget@@QAEXPAVAnimSequence@@@Z)
void AnimTarget::AddSequence(AnimSequence* sequence)
{
    mSequences.push_back(sequence);
}
