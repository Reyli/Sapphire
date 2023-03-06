#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <Actor/Player.h>
#include <Action/Action.h>
#include <Math/CalcStats.h>

#include "StatusEffect/StatusEffect.h"

using namespace Sapphire;
using namespace Sapphire::StatusEffect;

const uint16_t STATUS_ID_SHARPER_FANG_AND_CLAW = 802;

class ActionFullThrust84 :
  public ScriptAPI::ActionScript
{
public:
  ActionFullThrust84() :
    ScriptAPI::ActionScript( 84 )
  {
  }

  void onExecute( Sapphire::World::Action::Action& action ) override
  {

    auto pPlayer = action.getSourceChara()->getAsPlayer();
    if( pPlayer->gaugeDrgGetDragonState( Common::DrgState::BloodOfTheDragon ) || pPlayer->gaugeDrgGetDragonState( Common::DrgState::LifeOfTheDragon ) )
    {
      auto pEffect = Sapphire::StatusEffect::make_StatusEffect( STATUS_ID_SHARPER_FANG_AND_CLAW, action.getSourceChara(), action.getSourceChara(), 10000, 3000 );
      action.getEffectbuilder()->applyStatusEffect( action.getSourceChara(), action.getSourceChara(), pEffect );
    }
  }
};

EXPOSE_SCRIPT( ActionFullThrust84 );