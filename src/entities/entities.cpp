#include "../terrain.h"
#include "../track.h"
#include "static_mesh.h"
#include "static_decal.h"
#include "start.h"
#include "tree.h"
#include "booster.h"
#include "oil.h"
#include "glue.h"
#include "barrel.h"
#include "billboard.h"
#include "pickup.h"

void registerEntities()
{
    // DO NOT CHANGE THE ORDER OF THE REGISTERED ENTITIES!!!
    // If the order is changed it will break compatibility with previously saved scenes
    // If the order must be changed, then compatibility must be added to Scene::deserialize()
    registerEntity<Terrain>();
    registerEntity<Track>();
    registerEntity<StaticMesh>();
    registerEntity<StaticDecal>();
    registerEntity<Start>();
    registerEntity<Tree>();
    registerEntity<Booster>();
    registerEntity<Oil>();
    registerEntity<WaterBarrel>();
    registerEntity<Billboard>();
    registerEntity<Pickup>();
    registerEntity<Glue>();
}
