#include "ShopMgr.h"

#include <Framework.h>
#include <Exd/ExdDataGenerated.h>
#include <Actor/Player.h>
#include <Common.h>

extern Core::Framework g_fw;
using namespace Core;

bool Sapphire::World::Manager::ShopMgr::purchaseGilShopItem( Entity::Player& player, uint32_t shopId, uint16_t itemId, uint32_t quantity )
{
  auto exdData = g_fw.get< Data::ExdDataGenerated >();
  if( !exdData )
    return false;

  auto gilShopItem = exdData->get< Data::GilShopItem >( shopId, itemId );
  if( !gilShopItem )
    return false;

  auto item = exdData->get< Data::Item >( gilShopItem->item );
  if( !item )
    return false;

  if( player.getCurrency( Common::CurrencyType::Gil ) < item->priceMid )
    return false;

  if( !player.addItem( gilShopItem->item, quantity ) )
    return false;

  player.removeCurrency( Common::CurrencyType::Gil, item->priceMid );

  return true;
}