#include <ScriptObject.h>
#include <Territory/InstanceContent.h>

using namespace Sapphire;

class EdensPromiseUmbra : public Sapphire::ScriptAPI::InstanceContentScript
{
public:
  EdensPromiseUmbra() : Sapphire::ScriptAPI::InstanceContentScript( 30097 )
  { }

  void onInit( InstanceContent& instance ) override
  {
    instance.registerEObj( "Entrance", 2000182, 8533227, 5, { 100.000000f, 0.000000f, 115.000000f }, 1.000000f, 0.000000f); 
    // States -> vf_lock_on (id: 11) vf_lock_of (id: 12) 
    instance.registerEObj( "Exit", 2000139, 0, 4, { 100.000000f, 0.000000f, 85.000000f }, 1.000000f, 0.000000f); 

  }

  void onUpdate( InstanceContent& instance, uint64_t tickCount ) override
  {

  }

  void onEnterTerritory( InstanceContent& instance, Entity::Player& player, uint32_t eventId, uint16_t param1,
                         uint16_t param2 ) override
  {

  }

};

EXPOSE_SCRIPT( EdensPromiseUmbra );