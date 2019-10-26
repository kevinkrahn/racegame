#include "../terrain.h"
#include "../track.h"
#include "static_mesh.h"
#include "static_decal.h"
#include "start.h"
#include "tree.h"
#include "booster.h"

void registerEntities()
{
    // DO NOT CHANGE THE ORDER OF THE REGISTERED ENTITIES!!!
    // If the order is changed it will break compatibility with previously saved scenes
    registerEntity<Terrain>();
    registerEntity<Track>();
    registerEntity<StaticMesh>();
    registerEntity<StaticDecal>();
    registerEntity<Start>();
    registerEntity<Tree>();
    registerEntity<Booster>();
}
