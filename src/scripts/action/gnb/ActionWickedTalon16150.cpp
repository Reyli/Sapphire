#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <Actor/Player.h>
#include <Action/Action.h>
#include <Math/CalcStats.h>

#include "StatusEffect/StatusEffect.h"

using namespace Sapphire;
using namespace Sapphire::StatusEffect;

const uint16_t STATUS_ID_READY_TO_RIP = 1842;
const uint16_t STATUS_ID_READY_TO_TEAR = 1843;
const uint16_t STATUS_ID_READY_TO_GOUGE = 1844;

class ActionWickedTalon16150 :
  public ScriptAPI::ActionScript
{
public:
  ActionWickedTalon16150() :
    ScriptAPI::ActionScript( 16150 )
  {
  }

  void onBeforeBuildEffect( Sapphire::World::Action::Action& action, uint8_t victimCounter, uint8_t validVictimCounter ) override
  {
    if( validVictimCounter > 0 )
    {
      auto chara = action.getSourceChara();
      auto player = chara->getAsPlayer();
      assert( player );
      player->removeSingleStatusEffectById( STATUS_ID_READY_TO_RIP, false );
      player->removeSingleStatusEffectById( STATUS_ID_READY_TO_TEAR, false );
      auto pNewEffect = Sapphire::StatusEffect::make_StatusEffect( STATUS_ID_READY_TO_GOUGE, action.getSourceChara(), player, 10000, 3000 );
      action.getEffectbuilder()->applyStatusEffect( chara, chara, pNewEffect, 0 );
      player->gaugeGnbSetComboStep( 0 );
    }
  }
};

EXPOSE_SCRIPT( ActionWickedTalon16150 );