#include "../terrain.h"
#include "../track.h"
#include "static_mesh.h"
#include "static_decal.h"
#include "start.h"
#include "booster.h"
#include "oil.h"
#include "glue.h"
#include "billboard.h"
#include "pickup.h"

void registerEntities()
{
    // DO NOT CHANGE THE ORDER OF THE REGISTERED ENTITIES!!!
    // If the order is changed it will break compatibility with previously saved scenes
    // If the order must be changed, then compatibility must be added to Scene::deserialize()
    registerEntity<Terrain>();     // 0
    registerEntity<Track>();       // 1
    registerEntity<StaticMesh>();  // 2
    registerEntity<StaticDecal>(); // 3
    registerEntity<Start>();       // 4
    registerEntity<Start>();       // REMOVE
    registerEntity<Booster>();     // 6
    registerEntity<Oil>();         // 7
    registerEntity<Start>();       // REMOVE
    registerEntity<Billboard>();   // 9
    registerEntity<Pickup>();      // 10
    registerEntity<Glue>();        // 11
}
