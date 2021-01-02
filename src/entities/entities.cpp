#include "../terrain.h"
#include "../track.h"
#include "../spline.h"
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
    registerEntity<Terrain>(false);    // 0
    registerEntity<Track>(false);      // 1
    registerEntity<StaticMesh>(true);  // 2
    registerEntity<StaticDecal>(true); // 3
    registerEntity<Start>(true);       // 4
    registerEntity<Spline>(false);     // 5
    registerEntity<Booster>(true);     // 6
    registerEntity<Oil>(true);         // 7
    registerEntity<Start>(true);       // REMOVE
    registerEntity<Billboard>(true);   // 9
    registerEntity<Pickup>(true);      // 10
    registerEntity<Glue>(true);        // 11
}
