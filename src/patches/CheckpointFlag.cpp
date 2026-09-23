// TODO: tweak cull threshold to account for rotated models
// TODO: change position of the point counters (see CheckPointFlag::doCollect() at 0x02728c30)
// FUTURE: use trapezoidal colliders

#include <actor/Actor.h>
#include <actor/ActorCollision.h>
#include <actor/ActorState.h>
#include <effect/Effect.h>
#include <effect/EffectCreateUtil.h>
#include <player/PlayerObject.h>
#include <red/util/SpriteUtil.h>

#include <container/seadSafeArray.h>
#include <math/seadMathCalcCommon.h>

#define TELKIN_REGISTERS
#include <telkin/Assembly.h>
#include <telkin/Hooks.h>

namespace {
    extern "C" void PlayerObject_setItemGetEffect(PlayerObject*);
}

namespace checkpoint {
    const f32 ANGLE_MULTIPLIER_DEG = 11.25f;

    // Checkpoint powerup
    void changePlayerPowerup(Actor* checkpoint, PlayerObject* player) {
        enum CheckpointBehavior : u32 {
            Default,
            NoPowerup,
            PowerupConditionally,
            PowerupConditionallyWithOverride,
            PowerupAlways
        };

        CheckpointBehavior behavior = static_cast<CheckpointBehavior>(red::SpriteUtil::getNybble21(checkpoint));
        PlayerMode powerup = static_cast<PlayerMode>(red::SpriteUtil::getNybble22(checkpoint));

        switch (behavior) {
            case CheckpointBehavior::Default: {
                if (player->getPlayerMode() == PlayerMode::cPlayerMode_Small) {
                    player->setItem(PlayerMode::cPlayerMode_Normal);
                    PlayerObject_setItemGetEffect(player);
                    // player->setItemGetEffect();
                }
                break;
            }

            case CheckpointBehavior::NoPowerup: break;

            case CheckpointBehavior::PowerupConditionally: {
                PlayerMode current = player->getPlayerMode();

                bool hasPowerupAlready = current >= PlayerMode::cPlayerMode_Fire;
                if (hasPowerupAlready) {
                    break;
                }

                [[fallthrough]];
            }

            case CheckpointBehavior::PowerupConditionallyWithOverride: {
                if (!player->canChangeTo(powerup)) {
                    break;
                }

                [[fallthrough]];
            }
            
            case CheckpointBehavior::PowerupAlways: {
                player->setItem(powerup);
                PlayerObject_setItemGetEffect(player);
                // player->setItemGetEffect();
                break;
            }
        }
    }

    void changePlayerPowerupHook() tAssembly(
        tSaveLR;
        
        mr r3, r30; // checkpoint
        mr r4, r31; // player

        bl _ZN10checkpoint19changePlayerPowerupEP5ActorP12PlayerObject;
    
        // resume
        tRestoreLR;
        addi r0, r2, 0x1C;
        mtctr r0;
        bctr;
    )

    tBranch(0x02728F14, checkpoint::changePlayerPowerupHook, tk::BranchType::bl);
    tBranch(0x02728F98, checkpoint::changePlayerPowerupHook, tk::BranchType::bl);

    // Helper methods
    f32 getCheckpointAngleDeg(Actor* checkpoint) {
        return ANGLE_MULTIPLIER_DEG * red::SpriteUtil::getNybbleRange(checkpoint, 23, 24);
    }

    u32 getCheckpointAngleBAM(Actor* checkpoint) {
        f32 angleDeg = getCheckpointAngleDeg(checkpoint);

        u32 angleBAM;
        if (angleDeg < 180.0f) {
            angleBAM = sead::Mathf::deg2idx(angleDeg);
        } else {
            angleBAM = (u32)sead::Mathf::deg2idx(angleDeg - 180.0f) + 0x80000000;
        }
        return angleBAM;
    }

    // Checkpoint model angle
    void setModelAngle(Actor* checkpoint) {
        const u32 ORIGINAL_Y_ANGLE = -0x40000000;
        bool flip = static_cast<bool>(red::SpriteUtil::getNybble5(checkpoint));

        checkpoint->getAngle().y() = ORIGINAL_Y_ANGLE * (flip ? -1 : 1);
        
        bool affectedByGravity = (red::SpriteUtil::getNybble11(checkpoint) & 2) != 0;
        
        if (affectedByGravity) {
            return;
        }

        u32 angleBAM = getCheckpointAngleBAM(checkpoint);

        checkpoint->getAngle().z() = angleBAM;
    }
    
    void changeModelAngleHook() tAssembly(
        tSaveLR;

        mr r3, r29;
        bl _ZN10checkpoint13setModelAngleEP5Actor;

        tRestoreLR;
        blr;
    );

    tBranch(0x02727F90, checkpoint::changeModelAngleHook, tk::BranchType::bl);

    // Checkpoint collider
    using CC = ActorCollisionCheck;
    void setCheckpointCollision(CC* collider, Actor* checkpoint, const CC::CollisionData& colData) {
        const f32 angleDeg = getCheckpointAngleDeg(checkpoint);
        const f32 angleRad = sead::Mathf::deg2rad(angleDeg);

        const f32 s = sead::Mathf::sin(angleRad);
        const f32 c = sead::Mathf::cos(angleRad);

        const f32 halfWidth = 4.0f;
        const f32 halfHeight = 24.0f;

        CC::CollisionData cd = colData;

        // tweak the size
        cd.half_size = CC::Vec2(
            sead::Mathf::abs(halfWidth * c) +
            sead::Mathf::abs(halfHeight * s),

            sead::Mathf::abs(halfWidth * s) +
            sead::Mathf::abs(halfHeight * c)
        );

        // rotate the offset
        cd.center_offset = CC::Vec2(
            -24.0f * s,
            24.0f * c
        );

        collider->set(checkpoint, cd);
    }

    tBranch(0x02728068, checkpoint::setCheckpointCollision, tk::BranchType::bl);

    // Passive checkpoint event while uncollected
    void setPassiveCheckpointEffectRotation(ActorState* checkpoint_) {
        struct Checkpoint : ActorState {
            u8 stuffWeDontCareAbout[0x130];
            EffectObj mEffectObj;
        };

        Checkpoint* checkpoint = static_cast<Checkpoint*>(checkpoint_);
        f32 angleDeg = getCheckpointAngleDeg(checkpoint);

        Angle3 angle;
        angle.z() = getCheckpointAngleBAM(checkpoint);
        checkpoint->mEffectObj.follow(&checkpoint->getPos(), &angle, nullptr);
    }

    void setPassiveCheckpointEffectRotationHook() tAssembly(
        tSaveLR;

        mr r3, r31;
        bl _ZN10checkpoint34setPassiveCheckpointEffectRotationEP10ActorState;

        tRestoreLR;
        blr;
    );

    tBranch(0x0272858C, checkpoint::setPassiveCheckpointEffectRotationHook, tk::BranchType::bl);

    // Star effect on player collision
    void setPlayerCollectCheckpointEffectRotation(Actor* checkpoint) {
        Angle3 angle;
        angle.z() = getCheckpointAngleBAM(checkpoint);
        EffectCreateUtil::createEffect(RP_MiddleFlag_Change, &checkpoint->getPos(), &angle);
    }

    tBranch(0x02728C78, checkpoint::setPlayerCollectCheckpointEffectRotation, tk::BranchType::bl);
}

