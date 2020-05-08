import bpy, bmesh, struct, time, os, errno
from math import *
from mathutils import (Euler, Matrix, Vector)
from enum import Enum

DataType = Enum('DataType', 'string i64 f32 bytearray array dict bool')

def pack_string(string):
    charBytes = bytes(string, 'ASCII')
    return struct.pack("<I", len(charBytes)) + charBytes

def pack_list(list):
    buffer = bytearray()
    buffer += struct.pack('<I', len(list))
    for value in list:
        buffer += pack_value(value)
    return buffer

def pack_value(val):
    type_name = type(val).__name__
    if type_name == 'int':
        return struct.pack('<Iq', DataType.i64.value, val)
    elif type_name == 'float':
        return struct.pack('<If', DataType.f32.value, val)
    elif type_name == 'bytearray' or type_name == 'bytes':
        return struct.pack('<II', DataType.bytearray.value, len(val)) + val
    elif type_name == 'str':
        return struct.pack('<I', DataType.string.value) + pack_string(val)
    elif type_name == 'list':
        return struct.pack('<I', DataType.array.value) + pack_list(val)
    elif type_name == 'dict':
        return struct.pack('<I', DataType.dict.value) + pack_dict(val)
    elif type_name == 'bool':
        return struct.pack('<II', DataType.bool.value, 1 if val else 0)
    else:
        #raise ValueError('Cannot serialize data type: ' + type_name)
        print('Ignoring unsupported data type: ', type_name)
        return None

def pack_dict(dict):
    pair_buffer = bytearray()
    pair_count = 0
    for key, value in dict.items():
        packedValue = pack_value(value)
        if packedValue != None:
            pair_buffer += pack_string(key)
            pair_buffer += packedValue
            pair_count += 1
    return struct.pack('<I', pair_count) + pair_buffer

def get_props(obj):
    props = {}
    for (key, value) in obj.items():
        props[key] = value
    return props

def build_mesh_buffer(triangles, source_mesh, mesh_name):
    vertices = []
    vertex_count = 0
    vertex_buffer = bytearray()
    vertex_dict = {}
    indices = []

    for tri in triangles:
        tri_tex_coords = []
        tri_colors = []
        tri_tangents = []
        tri_positions = []
        tri_normals = []
        for loop_index in tri.loops:
            if len(source_mesh.uv_layers) > 0:
                uvs = []
                for uv_layer in source_mesh.uv_layers:
                    uvs.append(uv_layer.data[loop_index].uv)
                tri_tex_coords.append(uvs)
            else:
                tri_tex_coords.append([[ 0, 0 ]])

            if len(source_mesh.vertex_colors) > 0:
                colors = []
                for color in source_mesh.vertex_colors:
                    colors.append(color.data[loop_index].color)
                tri_colors.append(colors)
            else:
                tri_colors.append([[ 1, 1, 1 ]])

            loop = source_mesh.loops[loop_index]
            tri_tangents.append([ loop.tangent[0], loop.tangent[1], loop.tangent[2], loop.bitangent_sign ])
            tri_normals.append(loop.normal)
            tri_positions.append(source_mesh.vertices[loop.vertex_index].co)

        for i in range(3):
            position = tri_positions[i]
            normal = tri_normals[i]
            tangent = tri_tangents[i]
            colors = tri_colors[i]
            tex_coords = tri_tex_coords[i]

            vertex = (position[0], position[1], position[2], normal[0], normal[1], normal[2])
            for c in colors:
                vertex = vertex + (c[0], c[1], c[2])
            for u in tex_coords:
                vertex = vertex + (u[0], u[1])

            index = vertex_dict.get(vertex, None)
            if index == None:
                vertices.append(vertex)
                index = vertex_count
                vertex_count += 1
                vertex_dict[vertex] = index
                vertex_buffer += struct.pack("<10f", position[0], position[1], position[2],
                                                    normal[0], normal[1], normal[2],
                                                    tangent[0], tangent[1], tangent[2], tangent[3])
                for u in tex_coords:
                    vertex_buffer += struct.pack("<2f", u[0], 1.0 - u[1])
                for c in colors:
                    vertex_buffer += struct.pack("<3f", c[0], c[1], c[2])

            indices.append(index)

    minP = [999999.0, 999999, 999999];
    maxP = [-999999.0, -999999, -999999];
    for v in source_mesh.vertices:
        if v.co[0] < minP[0]: minP[0] = v.co[0];
        if v.co[1] < minP[1]: minP[1] = v.co[1];
        if v.co[2] < minP[2]: minP[2] = v.co[2];
        if v.co[0] > maxP[0]: maxP[0] = v.co[0];
        if v.co[1] > maxP[1]: maxP[1] = v.co[1];
        if v.co[2] > maxP[2]: maxP[2] = v.co[2];


    index_buffer = bytearray()
    for index in indices:
        index_buffer += struct.pack('<I', index)

    return {
        'name': mesh_name,
        'numColors': max(1, len(source_mesh.vertex_colors)),
        'numTexCoords': max(1, len(source_mesh.uv_layers)),
        'vertices': vertex_buffer,
        'numVertices': vertex_count,
        'indices': index_buffer,
        'numIndices': len(indices),
        'aabb': { 'min': [minP[0], minP[1], minP[2]], 'max': [maxP[0], maxP[1], maxP[2]] },
        'hasTangents': True,
    }

def save_mesh(obj, mesh_map, mesh_lookup):
    mesh_name = obj.data.name
    if len(obj.modifiers) > 0:
        mesh_name = obj.name

    if(mesh_lookup.get(mesh_name, None) != None):
        return mesh_lookup[mesh_name]

    depsgraph = bpy.context.evaluated_depsgraph_get()
    mesh_copy = obj.evaluated_get(depsgraph).to_mesh()

    bm = bmesh.new()
    bm.from_mesh(mesh_copy)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bm.to_mesh(mesh_copy)
    bm.free()
    del bm

    mesh_copy.calc_normals_split()
    mesh_copy.calc_loop_triangles()

    if len(mesh_copy.loop_triangles) == 0:
        return []

    if len(mesh_copy.uv_layers) == 0:
        uv_layer = mesh_copy.uv_layers.new(name="hello")

    mesh_copy.calc_tangents()

    # split mesh by material
    materials = {}
    for tri in mesh_copy.loop_triangles:
        if materials.get(tri.material_index, None) == None:
            materials[tri.material_index] = []
        materials[tri.material_index].append(tri)

    meshes = []
    if len(materials) > 0:
        for matIndex in materials:
            name = mesh_name
            if len(materials) > 1:
                name += "_mat" + str(matIndex)
            mesh = build_mesh_buffer(materials[matIndex], mesh_copy, name)
            mesh_map[name] = mesh
            meshes.append(name)
    else:
        mesh = build_mesh_buffer(mesh_copy.loop_triangles, mesh_copy, mesh_name)
        mesh_map[mesh_name] = mesh
        meshes.append(mesh_name)

    mesh_lookup[mesh_name] = meshes
    return meshes

def save_blender_data():
    start = time.time()
    mesh_map = {}
    mesh_lookup = {}
    path_map = {}
    scenes = []

    for scene in bpy.data.scenes:
        bpy.context.window.scene = scene
        objects = []
        collections = []

        def save_object(obj, objectType, name, matrix, data_name='NONE'):
            collection_indexes = []
            for collection in obj.users_collection:
                if collection.name in collections:
                     collection_indexes.append(collections.index(collection.name))
                else:
                     collection_indexes.append(len(collections))
                     collections.append(collection.name)

            objects.append({
                'type': objectType,
                'name': name,
                'data_name': data_name,
                'collection_indexes': collection_indexes,
                'matrix': struct.pack("<16f",
                    matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0],
                    matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1],
                    matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2],
                    matrix[0][3], matrix[1][3], matrix[2][3], matrix[3][3]),
                'properties': get_props(obj),
                'bounds': [obj.dimensions.x, obj.dimensions.y, obj.dimensions.z],
            })

        for obj in scene.objects:
            if obj.hide_render:
                continue

            if obj.type == 'MESH':
                meshes = save_mesh(obj, mesh_map, mesh_lookup)
                if len(meshes) > 0:
                    if len(meshes) > 1:
                        for index, mesh_name in enumerate(meshes):
                            save_object(obj, 'MESH', obj.name + "_mat" + str(index),
                                    obj.matrix_world, mesh_name)
                    else:
                        save_object(obj, 'MESH', obj.name, obj.matrix_world, meshes[0])

        scenes.append({
            'type': 'scene',
            'name': scene.name,
            'objects': objects,
            'collections': collections,
            'properties': get_props(scene)
        })

    out_file = 'blender_output.dat'
    with open(out_file, 'bw') as file:
        file.write(struct.pack('<I', 0x00001111))
        file.write(pack_value({
            'meshes': mesh_map,
            'scenes': scenes
        }))
        print('Saved to file:', out_file)

    print('Export took', time.time() - start, 'seconds')
    return {'FINISHED'}

if __name__ == "__main__":
    save_blender_data()

