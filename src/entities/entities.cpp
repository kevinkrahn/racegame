#include "../terrain.h"
#include "../track.h"
#include "static_mesh.h"
#include "static_decal.h"
#include "start.h"
#include "tree.h"
#include "booster.h"
#include "oil.h"

void registerEntities()
{
    // DO NOT CHANGE THE ORDER OF THE REGISTERED ENTITIES!!!
    // If the order is changed it will break compatibility with previously saved scenes
    registerEntity<Terrain>("Terrain");
    registerEntity<Track>("Track");
    registerEntity<StaticMesh>("StaticMesh");
    registerEntity<StaticDecal>("StaticDecal");
    registerEntity<Start>("Start");
    registerEntity<Tree>("Tree");
    registerEntity<Booster>("Booster");
    registerEntity<Oil>("Oil");
}
