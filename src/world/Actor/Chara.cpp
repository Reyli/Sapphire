#include <Util/Util.h>
#include <Util/UtilMath.h>
#include <Exd/ExdData.h>
#include <utility>
#include <Network/CommonActorControl.h>
#include <Service.h>

#include "Forwards.h"

#include "Territory/Territory.h"

#include "Network/GameConnection.h"
#include "Network/PacketWrappers/ActorControlPacket.h"
#include "Network/PacketWrappers/ActorControlTargetPacket.h"
#include "Network/PacketWrappers/RestingPacket.h"
#include "Network/PacketWrappers/EffectPacket1.h"
#include "Network/PacketWrappers/HudParamPacket.h"

#include "StatusEffect/StatusEffect.h"
#include "Action/Action.h"
#include "WorldServer.h"
#include "Session.h"
#include "Math/CalcStats.h"
#include "Chara.h"
#include "Player.h"
#include "Manager/TerritoryMgr.h"
#include "Manager/MgrUtil.h"
#include "Manager/PlayerMgr.h"
#include "Common.h"

using namespace Sapphire::Common;
using namespace Sapphire::World::Manager;
using namespace Sapphire::Network::Packets;
using namespace Sapphire::Network::Packets::WorldPackets::Server;
using namespace Sapphire::Network::ActorControl;

Sapphire::Entity::Chara::Chara( ObjKind type ) :
  GameObject( type ),
  m_pose( 0 ),
  m_targetId( INVALID_GAME_OBJECT_ID64 ),
  m_directorId( 0 ),
  m_radius( 1.f )
{

  m_lastTickTime = 0;
  m_lastUpdate = 0;
  m_lastAttack = Util::getTimeMs();

  m_bonusStats.fill( 0 );

  // initialize the free slot queue
  for( uint8_t i = 0; i < MAX_STATUS_EFFECTS; i++ )
  {
    m_statusEffectFreeSlotQueue.push( i );
  }
}

Sapphire::Entity::Chara::~Chara()
= default;

/*! \return the actors name */
std::string Sapphire::Entity::Chara::getName() const
{
  return { m_name };
}


/*! \return current stance of the actors */
Sapphire::Common::Stance Sapphire::Entity::Chara::getStance() const
{
  return m_currentStance;
}

/*! \return actor stats */
const Sapphire::Entity::Chara::ActorStatsArray& Sapphire::Entity::Chara::getStats() const
{
  return m_baseStats;
}

/*! \return current HP */
uint32_t Sapphire::Entity::Chara::getHp() const
{
  return m_hp;
}

uint32_t Sapphire::Entity::Chara::getHpPercent() const
{
  return ( m_hp * 100 ) / m_maxHp;
}

/*! \return current MP */
uint32_t Sapphire::Entity::Chara::getMp() const
{
  return m_mp;
}

/*! \return current TP */
uint16_t Sapphire::Entity::Chara::getTp() const
{
  return m_tp;
}

/*! \return current GP */
uint16_t Sapphire::Entity::Chara::getGp() const
{
  return m_gp;
}

/*! \return current invincibility type */
InvincibilityType Sapphire::Entity::Chara::getInvincibilityType() const
{
  return m_invincibilityType;
}

/*! \return current class or job */
Sapphire::Common::ClassJob Sapphire::Entity::Chara::getClass() const
{
  return m_class;
}

/*! \param ClassJob to set */
void Sapphire::Entity::Chara::setClass( Common::ClassJob classJob )
{
  m_class = classJob;
}

Sapphire::Common::Role Sapphire::Entity::Chara::getRole() const
{
  switch( getClass() )
  {
    case ClassJob::Gladiator:
    case ClassJob::Marauder:
    case ClassJob::Paladin:
    case ClassJob::Warrior:
    case ClassJob::Darkknight:
      return Role::Tank;

    case ClassJob::Pugilist:
    case ClassJob::Lancer:
    case ClassJob::Monk:
    case ClassJob::Dragoon:
    case ClassJob::Rogue:
    case ClassJob::Ninja:
      return Role::Melee;

    case ClassJob::Archer:
    case ClassJob::Bard:
    case ClassJob::Machinist:
      return Role::RangedPhysical;

    case ClassJob::Conjurer:
    case ClassJob::Whitemage:
    case ClassJob::Scholar:
    case ClassJob::Astrologian:
      return Role::Healer;

    case ClassJob::Thaumaturge:
    case ClassJob::Blackmage:
    case ClassJob::Arcanist:
    case ClassJob::Summoner:
      return Role::RangedMagical;

    case ClassJob::Carpenter:
    case ClassJob::Blacksmith:
    case ClassJob::Armorer:
    case ClassJob::Goldsmith:
    case ClassJob::Leatherworker:
    case ClassJob::Weaver:
    case ClassJob::Alchemist:
    case ClassJob::Culinarian:
      return Role::Crafter;

    case ClassJob::Miner:
    case ClassJob::Botanist:
    case ClassJob::Fisher:
      return Role::Gatherer;

    default:
      return Role::None;
  }
}

/*! \param Id of the target to set */
void Sapphire::Entity::Chara::setTargetId( uint64_t targetId )
{
  m_targetId = targetId;
}

/*! \return Id of the current target */
uint64_t Sapphire::Entity::Chara::getTargetId() const
{
  return m_targetId;
}

/*! \return True if the actor is alive */
bool Sapphire::Entity::Chara::isAlive() const
{
  return ( m_hp > 0 );
}

/*! \return max hp for the actor */
uint32_t Sapphire::Entity::Chara::getMaxHp() const
{
  return m_maxHp;
}

/*! \return max mp for the actor */
uint32_t Sapphire::Entity::Chara::getMaxMp() const
{
  return m_maxMp;
}

/*! \return reset hp to current max hp */
void Sapphire::Entity::Chara::resetHp()
{
  m_hp = getMaxHp();
}

/*! \return reset mp to current max mp */
void Sapphire::Entity::Chara::resetMp()
{
  m_mp = getMaxMp();
}

/*! \param hp amount to set ( caps to maxHp ) */
void Sapphire::Entity::Chara::setHp( uint32_t hp )
{
  m_hp = hp < getMaxHp() ? hp : getMaxHp();
}

/*! \param mp amount to set ( caps to maxMp ) */
void Sapphire::Entity::Chara::setMp( uint32_t mp )
{
  m_mp = mp < getMaxMp() ? mp : getMaxMp();
}

/*! \param gp amount to set*/
void Sapphire::Entity::Chara::setGp( uint32_t gp )
{
  m_gp = static_cast< uint16_t >( gp );
}

/*! \param tp amount to set*/
void Sapphire::Entity::Chara::setTp( uint32_t tp )
{
  m_tp = static_cast< uint16_t >( tp );
}

/*! \param type invincibility type to set */
void Sapphire::Entity::Chara::setInvincibilityType( Common::InvincibilityType type )
{
  m_invincibilityType = type;
}

/*! \return current status of the actor */
Sapphire::Common::ActorStatus Sapphire::Entity::Chara::getStatus() const
{
  return m_status;
}

/*! \param status to set */
void Sapphire::Entity::Chara::setStatus( ActorStatus status )
{
  m_status = status;
}

/*!
Performs necessary steps to mark an actor dead.
Sets hp/mp/tp, sets status, plays animation and fires onDeath event
*/
void Sapphire::Entity::Chara::die()
{
  m_status = ActorStatus::Dead;
  m_hp = 0;
  m_mp = 0;
  m_tp = 0;

  // fire onDeath event
  onDeath();

  // if the actor is a player, the update needs to be send to himself too
  bool selfNeedsUpdate = isPlayer();

  FFXIVPacketBasePtr packet = makeActorControl( m_id, SetStatus, static_cast< uint8_t >( ActorStatus::Dead ) );
  server().queueForPlayers( getInRangePlayerIds( selfNeedsUpdate ), packet );

  FFXIVPacketBasePtr packet1 = makeActorControl( m_id, DeathAnimation, 0, 0, 0, 0 );
  server().queueForPlayers( getInRangePlayerIds( selfNeedsUpdate ), packet1 );

}

/*!
Calculates and sets the rotation to look towards a specified
position

\param Position to look towards
*/
bool Sapphire::Entity::Chara::face( const Common::FFXIVARR_POSITION3& p )
{
  float oldRot = getRot();
  float rot = Util::calcAngFrom( getPos().x, getPos().z, p.x, p.z );
  float newRot = PI - rot + ( PI / 2 );

  setRot( newRot );

  return ( fabs( oldRot - newRot ) <= std::numeric_limits< float >::epsilon() * fmax( fabs( oldRot ), fabs( newRot ) ) );
}

/*!
Set and propagate the actor stance to in range players
( not the actor himself )

\param stance to set
*/
void Sapphire::Entity::Chara::setStance( Stance stance )
{
  m_currentStance = stance;

  FFXIVPacketBasePtr packet = makeActorControl( m_id, ToggleWeapon, stance, 1 );
  server().queueForPlayers( getInRangePlayerIds(), packet );
}

/*!
Check if an action is queued for execution, if so update it
and if fully performed, clean up again.

\return true if a queued action has been updated
*/
bool Sapphire::Entity::Chara::checkAction()
{

  if( m_pCurrentAction == nullptr )
    return false;

  if( m_pCurrentAction->update() )
    m_pCurrentAction.reset();

  return true;

}

void Sapphire::Entity::Chara::update( uint64_t tickCount )
{
  updateStatusEffects();

  if( std::difftime( static_cast< time_t >( tickCount ), m_lastTickTime ) > 3000 )
  {
    onTick();

    m_lastTickTime = static_cast< time_t >( tickCount );
  }

  m_lastUpdate = static_cast< time_t >( tickCount );
}

/*!
Change the current target and propagate to in range players

\param target actor id
*/
void Sapphire::Entity::Chara::changeTarget( uint64_t targetId )
{
  setTargetId( targetId );
  FFXIVPacketBasePtr packet = makeActorControlTarget( m_id, SetTarget, 0, 0, 0, 0, targetId );
  server().queueForPlayers( getInRangePlayerIds(), packet );
}

/*!
Dummy function \return 0
*/
uint8_t Sapphire::Entity::Chara::getLevel() const
{
  return 0;
}

/*!
Let an actor take damage and perform necessary steps
according to resulting hp, propagates new hp value to players
in range
TODO: eventually this needs to distinguish between physical and
magical dmg and take status effects into account

\param amount of damage to be taken
*/
void Sapphire::Entity::Chara::takeDamage( uint32_t damage )
{
  if( damage >= m_hp )
  {
    switch( m_invincibilityType )
    {
      case InvincibilityNone:
        setHp( 0 );
        die();
        break;
      case InvincibilityRefill:
        resetHp();
        break;
      case InvincibilityStayAlive:
        setHp( 0 );
        break;
      case InvincibilityIgnoreDamage:
        break;
    }
  }
  else
    m_hp -= damage;
}

/*!
Let an actor get healed and perform necessary steps
according to resulting hp, propagates new hp value to players
in range

\param amount of hp to be healed
*/
void Sapphire::Entity::Chara::heal( uint32_t amount )
{
  if( ( m_hp + amount ) > getMaxHp() )
  {
    m_hp = getMaxHp();
  }
  else
    m_hp += amount;
}

void Sapphire::Entity::Chara::restoreMP( uint32_t amount )
{
  if( ( m_mp + amount ) > getMaxMp() )
  {
    m_mp = getMaxMp();
  }
  else
    m_mp += amount;
}

/*!
Send an HpMpTp update to players in range ( and potentially to self )
TODO: poor naming, should be changed. Status is not HP. Also should be virtual
so players can have their own version and we can abolish the param.

\param true if the update should also be sent to the actor ( player ) himself
*/
void Sapphire::Entity::Chara::sendHudParam()
{
  FFXIVPacketBasePtr packet = makeHudParam( *this );
  server().queueForPlayers( getInRangePlayerIds( isPlayer() ), packet );
}

/*! \return ActionPtr of the currently registered action, or nullptr */
Sapphire::World::Action::ActionPtr Sapphire::Entity::Chara::getCurrentAction() const
{
  return m_pCurrentAction;
}

/*! \param ActionPtr of the action to be registered */
void Sapphire::Entity::Chara::setCurrentAction( Sapphire::World::Action::ActionPtr pAction )
{
  m_pCurrentAction = std::move( pAction );
}

/*!
Autoattack prototype implementation
TODO: move the check if the autoAttack can be performed to the callee
also rename autoAttack to autoAttack as that is more elaborate
On top of that, this only solves attacks from melee classes.
Will have to be extended for ranged attacks.

\param GameObjectPtr the autoAttack is performed on
*/
void Sapphire::Entity::Chara::autoAttack( CharaPtr pTarget )
{
  uint64_t tick = Util::getTimeMs();

  // todo: this needs to use the auto attack delay for the equipped weapon
  if( ( tick - m_lastAttack ) > 2500 )
  {
    pTarget->onActionHostile( getAsChara() );
    m_lastAttack = tick;
    srand( static_cast< uint32_t >( tick ) );

    auto damage = static_cast< uint16_t >( 10 + rand() % 12 );

    auto effectPacket = std::make_shared< EffectPacket1 >( getId(), pTarget->getId(), 7 );
    effectPacket->setRotation( Util::floatToUInt16Rot( getRot() ) );

    Common::CalcResultParam effectEntry{};
    effectEntry.Value = static_cast< int16_t >( damage );
    effectEntry.Type = ActionEffectType::CALC_RESULT_TYPE_DAMAGE_HP;
    effectEntry.Arg0 = static_cast< uint8_t >( ActionHitSeverityType::NormalDamage );
    effectEntry.Arg2 = 0x71;
    effectPacket->addTargetEffect( effectEntry );

    server().queueForPlayers( getInRangePlayerIds(), effectPacket );

    pTarget->takeDamage( damage );
  }
}

/*! \param StatusEffectPtr to be applied to the actor */
void Sapphire::Entity::Chara::addStatusEffect( StatusEffect::StatusEffectPtr pEffect )
{
  auto& teriMgr = Common::Service< World::Manager::TerritoryMgr >::ref();
  auto pZone = teriMgr.getTerritoryByGuId( getTerritoryId() );

  int8_t nextSlot = getStatusEffectFreeSlot();
  // if there is no slot left, do not add the effect
  if( nextSlot == -1 )
    return;

  pEffect->applyStatus();
  m_statusEffectMap[ nextSlot ] = pEffect;

  auto statusEffectAdd = makeZonePacket< FFXIVIpcActionIntegrity >( getId() );

  statusEffectAdd->data().ResultId = pZone->getNextEffectResultId();
  statusEffectAdd->data().Target = pEffect->getTargetActorId();
  statusEffectAdd->data().Hp = getHp();
  statusEffectAdd->data().Mp = static_cast< uint16_t >( getMp() );
  statusEffectAdd->data().Tp = static_cast< uint16_t >( getTp() );
  statusEffectAdd->data().HpMax = getMaxHp();
  statusEffectAdd->data().MpMax = static_cast< uint16_t >( getMaxMp() );
  statusEffectAdd->data().ClassJob = static_cast< uint8_t >( getClass() );
  statusEffectAdd->data().StatusCount = 1;
  statusEffectAdd->data().unknown_E0 = 0xE0;

  // set all status sources to u32 invalid game obj
  // todo: chara status effect map should be filled instead, since hudparam also uses invalid gameobj
  for( int i = 0; i < 4; ++i )
  {
    statusEffectAdd->data().Status[ i ].Source = INVALID_GAME_OBJECT_ID;
  }

  auto& status = statusEffectAdd->data().Status[ 0 ];

  status.Source = pEffect->getSrcActorId();
  status.Time = static_cast< float >( pEffect->getDuration() ) / 1000;
  status.Id = static_cast< uint16_t >( pEffect->getId() );
  status.Slot = static_cast< uint8_t >( nextSlot );
  status.SystemParam = static_cast< int16_t >( pEffect->getParam() );

  server().queueForPlayers( getInRangePlayerIds( isPlayer() ), statusEffectAdd );
}

/*! \param StatusEffectPtr to be applied to the actor */
void Sapphire::Entity::Chara::addStatusEffectById( uint32_t id, int32_t duration, Entity::Chara& source, uint16_t param )
{
  auto effect = StatusEffect::make_StatusEffect( id, source.getAsChara(), getAsChara(), duration, 3000 );
  effect->setParam( param );
  addStatusEffect( effect );
}

/*! \param StatusEffectPtr to be applied to the actor */
void Sapphire::Entity::Chara::addStatusEffectByIdIfNotExist( uint32_t id, int32_t duration, Entity::Chara& source,
                                                             uint16_t param )
{
  if( hasStatusEffect( id ) )
    return;

  auto effect = StatusEffect::make_StatusEffect( id, source.getAsChara(), getAsChara(), duration, 3000 );
  effect->setParam( param );
  addStatusEffect( effect );
}

void Sapphire::Entity::Chara::addStatusEffectByIdIfNotExist( uint32_t id, int32_t duration, Entity::Chara& source,
                                                             std::vector< World::Action::StatusModifier >& modifiers, uint16_t param )
{
  if( hasStatusEffect( id ) )
    return;

  auto effect = StatusEffect::make_StatusEffect( id, source.getAsChara(), getAsChara(), duration, modifiers, 3000 );
  effect->setParam( param );
  addStatusEffect( effect );
}

int8_t Sapphire::Entity::Chara::getStatusEffectFreeSlot()
{
  int8_t freeEffectSlot = -1;

  if( m_statusEffectFreeSlotQueue.empty() )
    return freeEffectSlot;

  freeEffectSlot = static_cast< int8_t >( m_statusEffectFreeSlotQueue.front() );
  m_statusEffectFreeSlotQueue.pop();

  return freeEffectSlot;
}

void Sapphire::Entity::Chara::statusEffectFreeSlot( uint8_t slotId )
{
  m_statusEffectFreeSlotQueue.push( slotId );
}

void Sapphire::Entity::Chara::removeSingleStatusEffectById( uint32_t id )
{
  for( const auto& effectIt : m_statusEffectMap )
  {
    if( effectIt.second->getId() == id )
    {
      removeStatusEffect( effectIt.first );
      break;
    }
  }
}

std::map< uint8_t, Sapphire::StatusEffect::StatusEffectPtr >::iterator Sapphire::Entity::Chara::removeStatusEffect( uint8_t effectSlotId )
{
  auto pEffectIt = m_statusEffectMap.find( effectSlotId );
  if( pEffectIt == m_statusEffectMap.end() )
    return pEffectIt;

  statusEffectFreeSlot( effectSlotId );

  auto pEffect = pEffectIt->second;
  pEffect->removeStatus();

  server().queueForPlayers( getInRangePlayerIds( isPlayer() ), makeActorControl( getId(), StatusEffectLose, pEffect->getId() ) );

  auto it = m_statusEffectMap.erase( pEffectIt );

  if( isPlayer() )
    server().queueForPlayers( getInRangePlayerIds( isPlayer() ), makeHudParam( *getAsPlayer() ) );
  else if( isBattleNpc() )
    server().queueForPlayers( getInRangePlayerIds( isPlayer() ), makeHudParam( *getAsBNpc() ) );

  return it;
}

std::map< uint8_t, Sapphire::StatusEffect::StatusEffectPtr > Sapphire::Entity::Chara::getStatusEffectMap() const
{
  return m_statusEffectMap;
}

const uint8_t* Sapphire::Entity::Chara::getLookArray() const
{
  return m_customize;
}

const uint32_t* Sapphire::Entity::Chara::getModelArray() const
{
  return m_modelEquip;
}

uint8_t Sapphire::Entity::Chara::getPose() const
{
  return m_pose;
}

void Sapphire::Entity::Chara::setPose( uint8_t pose )
{
  m_pose = pose;
}

void Sapphire::Entity::Chara::sendStatusEffectUpdate()
{
  uint64_t currentTimeMs = Util::getTimeMs();

  auto statusEffectList = makeZonePacket< FFXIVIpcStatus >( getId() );
  uint8_t slot = 0;
  for( const auto& effectIt : m_statusEffectMap )
  {
    float timeLeft = static_cast< float >( effectIt.second->getDuration() -
                                           ( currentTimeMs - effectIt.second->getStartTimeMs() ) ) / 1000;
    statusEffectList->data().effect[ slot ].Time = timeLeft;
    statusEffectList->data().effect[ slot ].Id = effectIt.second->getId();
    statusEffectList->data().effect[ slot ].Source = effectIt.second->getSrcActorId();
    slot++;
  }

  server().queueForPlayers( getInRangePlayerIds( isPlayer() ), statusEffectList );
}

void Sapphire::Entity::Chara::updateStatusEffects()
{
  uint64_t currentTimeMs = Util::getTimeMs();

  for( auto effectIt = m_statusEffectMap.begin(); effectIt != m_statusEffectMap.end(); )
  {
    uint8_t effectIndex = effectIt->first;
    auto effect = effectIt->second;

    uint64_t lastTick = effect->getLastTickMs();
    uint64_t startTime = effect->getStartTimeMs();
    uint32_t duration = effect->getDuration();
    uint32_t tickRate = effect->getTickRate();

    if( duration > 0 && ( currentTimeMs - startTime ) > duration )
      effectIt = removeStatusEffect( effectIndex );
    else
      ++effectIt;

    if( ( currentTimeMs - lastTick ) > tickRate )
    {
      effect->setLastTick( currentTimeMs );
      effect->onTick();
    }
  }
}

bool Sapphire::Entity::Chara::hasStatusEffect( uint32_t id )
{
  return m_statusEffectMap.find( id ) != m_statusEffectMap.end();
}

int64_t Sapphire::Entity::Chara::getLastUpdateTime() const
{
  return m_lastUpdate;
}

void Sapphire::Entity::Chara::setLastComboActionId( uint32_t actionId )
{
  m_lastComboActionId = actionId;
  m_lastComboActionTime = Util::getTimeMs();
}

uint32_t Sapphire::Entity::Chara::getLastComboActionId() const
{
  // initially check for the time passed first, if it's more than the threshold just return 0 for the combo
  // we can hide the implementation detail this way and it just works:tm: for anything that uses it

  if( std::difftime( static_cast< time_t >( Util::getTimeMs() ),
                     static_cast< time_t >( m_lastComboActionTime ) ) > Common::MAX_COMBO_LENGTH )
  {
    return 0;
  }

  return m_lastComboActionId;
}

uint32_t Sapphire::Entity::Chara::getDirectorId() const
{
  return m_directorId;
}

void Sapphire::Entity::Chara::setDirectorId( uint32_t directorId )
{
  m_directorId = directorId;
}

uint32_t Sapphire::Entity::Chara::getAgentId() const
{
  return m_agentId;
}

void Sapphire::Entity::Chara::setAgentId( uint32_t agentId )
{
  m_agentId = agentId;
}


float Sapphire::Entity::Chara::getRadius() const
{
  return m_radius;
}

Sapphire::Common::BaseParam Sapphire::Entity::Chara::getPrimaryStat() const
{
  auto& exdData = Common::Service< Data::ExdData >::ref();

  auto classJob = exdData.getRow< Excel::ClassJob >( static_cast< uint16_t >( getClass() ) );
  assert( classJob );

  return static_cast< Sapphire::Common::BaseParam >( classJob->data().Role );
}

uint32_t Sapphire::Entity::Chara::getStatValue( Sapphire::Common::BaseParam baseParam ) const
{
  auto index = static_cast< uint32_t >( baseParam );
  assert( index < m_baseStats.size() );

  return m_baseStats[ index ] + m_bonusStats[ index ];
}

float Sapphire::Entity::Chara::getStatValueFloat( Common::BaseParam baseParam ) const
{
  auto index = static_cast< uint32_t >( baseParam );
  assert( index < m_baseStats.size() );

  return static_cast< float >( m_baseStats[ index ] + m_bonusStats[ index ] );
}

uint32_t Sapphire::Entity::Chara::getBonusStat( Sapphire::Common::BaseParam baseParam ) const
{
  auto index = static_cast< uint32_t >( baseParam );
  assert( index < m_bonusStats.size() );

  return m_bonusStats[ index ];
}

void Sapphire::Entity::Chara::setStatValue( Common::BaseParam baseParam, uint32_t value )
{
  auto index = static_cast< uint32_t >( baseParam );
  assert( index < m_baseStats.size() );

  m_baseStats[ index ] = value;
}

float Sapphire::Entity::Chara::getModifier( Common::ParamModifier paramModifier ) const
{
  if( m_modifiers.find( paramModifier ) == m_modifiers.end() )
    return paramModifier >= Common::ParamModifier::StrengthPercent ? 1.0f : 0;

  auto& mod = m_modifiers.at( paramModifier );
  if( paramModifier >= Common::ParamModifier::StrengthPercent )
  {
    auto valPercent = 1.0f;
    for( const auto val : mod )
      valPercent *= 1.0f + ( val / 100.0f );
    return valPercent;
  }
  else
  {
    return std::accumulate( mod.begin(), mod.end(), 0 );
  }
}

void Sapphire::Entity::Chara::addModifier( Common::ParamModifier paramModifier, int32_t value )
{
  m_modifiers[ paramModifier ].push_back( value );

  if( auto pPlayer = this->getAsPlayer(); pPlayer )
    Common::Service< World::Manager::PlayerMgr >::ref().sendDebug( *pPlayer, "Modifier: {}, value: {}", static_cast< int32_t >( paramModifier ), getModifier( paramModifier ) );
}

void Sapphire::Entity::Chara::delModifier( Common::ParamModifier paramModifier, int32_t value )
{
  assert( m_modifiers.count( paramModifier ) != 0 );
  auto& mod = m_modifiers.at( paramModifier );

  mod.erase( std::remove( mod.begin(), mod.end(), value ), mod.end() );

  if( mod.size() == 0 )
    m_modifiers.erase( paramModifier );

  if( auto pPlayer = this->getAsPlayer(); pPlayer )
    Common::Service< World::Manager::PlayerMgr >::ref().sendDebug( *pPlayer, "Modifier: {}, value: {}", static_cast< int32_t >( paramModifier ), getModifier( paramModifier ) );
}

void Sapphire::Entity::Chara::onTick()
{
  uint32_t thisTickDmg = 0;
  uint32_t thisTickHeal = 0;

  for( const auto& effectIt : m_statusEffectMap )
  {
    auto thisEffect = effectIt.second->getTickEffect();
    switch( thisEffect.first )
    {
      case 1:
      {
        thisTickDmg += thisEffect.second;
        break;
      }

      case 2:
      {
        thisTickHeal += thisEffect.second;
        break;
      }
    }
  }

  if( thisTickDmg != 0 )
  {
    takeDamage( thisTickDmg );
    server().queueForPlayers( getInRangePlayerIds( isPlayer() ), makeActorControl( getId(), HPFloatingText, 0,
                                                                                   static_cast< uint8_t >( ActionEffectType::CALC_RESULT_TYPE_DAMAGE_HP ),
                                                                                   thisTickDmg ) );
  }

  if( thisTickHeal != 0 )
  {
    heal( thisTickHeal );
    server().queueForPlayers( getInRangePlayerIds( isPlayer() ), makeActorControl( getId(), HPFloatingText, 0,
                                                                                   static_cast< uint8_t >( ActionEffectType::CALC_RESULT_TYPE_RECOVER_HP ),
                                                                                   thisTickHeal ) );
  }
}