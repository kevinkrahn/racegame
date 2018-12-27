import bpy, bmesh, struct, time, os, errno
from math import *
from mathutils import (Euler, Matrix, Vector)
from enum import Enum

DataType = Enum('DataType', 'string i64 f64 bytearray array dict')

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
        return struct.pack('<Id', DataType.f64.value, val)
    elif type_name == 'bytearray' or type_name == 'bytes':
        return struct.pack('<II', DataType.bytearray.value, len(val)) + val
    elif type_name == 'str':
        return struct.pack('<I', DataType.string.value) + pack_string(val)
    elif type_name == 'list':
        return struct.pack('<I', DataType.array.value) + pack_list(val)
    elif type_name == 'dict':
        return struct.pack('<I', DataType.dict.value) + pack_dict(val)
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

def namePrefix(path=bpy.data.filepath):
    return os.path.splitext(os.path.basename(path))[0] + '.'

def save_mesh(obj, mesh_map):
    mesh_name = namePrefix() + obj.data.name
    if len(obj.modifiers) > 0:
        mesh_name = namePrefix() + obj.name
    elif obj.library != None:
        return (False, namePrefix(obj.library) + obj.data.name)

    if(mesh_map.get(mesh_name, None) != None):
        return (False, mesh_name)

    mesh = obj.data
    mesh_copy = obj.to_mesh(bpy.context.scene, True, 'PREVIEW')
    bm = bmesh.new()
    bm.from_mesh(mesh_copy)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bm.to_mesh(mesh_copy)
    bm.free()
    del bm

    def get_autosmooth_normal(mesh, fop, mesh_vi):
        result = fop.normal
        if fop.normal.length > 0.0:
            for p in mesh.polygons:
                if ( p != fop ) and ( mesh_vi in p.vertices ) and ( p.normal.length > 0.0):
                    angle = int(round(math.degrees(fop.normal.angle(p.normal))))
                    if angle <= mesh.auto_smooth_angle:
                        result = mesh.vertices[mesh_vi].normal
        return result

    vertices = []
    vertex_count = 0
    vertex_buffer = bytearray()
    vertex_dict = {}
    indices = []
    index_buffer = bytearray()
    element_size = 0

    if len(mesh.polygons) > 0:
        element_size = 3
        for tri in mesh_copy.polygons:
            tri_positions = []
            tri_normals = []
            for i in tri.vertices:
                tri_positions.append(mesh_copy.vertices[i].co)

                normal = tri.normal
                if tri.use_smooth:
                    v = mesh_copy.vertices[i]
                    if mesh_copy.use_auto_smooth:
                        normal = get_autosmooth_normal(mesh_copy, tri, v)
                    else:
                        normal = v.normal

                tri_normals.append(normal)

            tri_tex_coords = []
            tri_colors     = []
            for i in tri.loop_indices:
                tex_coords = []
                colors = []
                for uv_layer in mesh_copy.uv_layers:
                    uv = uv_layer.data[i].uv
                    uv.y = 1.0 - uv.y
                    tex_coords.append(uv)

                for color_layer in mesh_copy.vertex_colors:
                    colors.append(color_layer.data[i].color)

                tri_tex_coords.append(tex_coords)
                tri_colors.append(colors)

            for i in range(3):
                position = tri_positions[i]
                normal = tri_normals[i]
                colors = tri_colors[i]
                tex_coords = tri_tex_coords[i]

                if (len(colors) == 0):
                    colors = [[1.0, 1.0, 1.0]]

                if (len(tex_coords) == 0):
                    tex_coords = [[0.0, 0.0]]

                vertex_tuple = (position[0], position[1], position[2], normal[0], normal[1], normal[2]);
                for c in colors:
                    vertex_tuple = vertex_tuple + (c[0], c[1], c[2])
                for u in tex_coords:
                    vertex_tuple = vertex_tuple + (u[0], u[1])

                index = vertex_dict.get(vertex_tuple, None)
                if index == None:
                    index = vertex_count
                    vertex_count += 1
                    vertex_dict[vertex_tuple] = index
                    vertex_buffer += struct.pack("<6f", position[0], position[1], position[2],
                                                        normal[0], normal[1], normal[2])
                    for c in colors:
                        vertex_buffer += struct.pack("<3f", c[0], c[1], c[2])
                    for u in tex_coords:
                        vertex_buffer += struct.pack("<2f", u[0], u[1])

                indices.append(index)
                index_buffer += struct.pack('<I', index)

    elif len(mesh.edges) > 0:
        element_size = 2
        for tri in mesh_copy.polygons:
            for i in tri.vertices:
                vertex_count += 1
                position = mesh_copy.vertices[i].co
                vertex_buffer += struct.pack("<3f", position[0], position[1], position[2])

    else:
        element_size = 1
        vertex_count = len(mesh_copy.vertices)
        for v in mesh_copy.vertices:
            vertex_buffer += struct.pack("<3f", v.co[0], v.co[1], v.co[2])

    mesh_map[mesh_name] = {
        'name': mesh_name,
        'num_colors': max(1, len(mesh_copy.vertex_colors)),
        'num_texcoords': max(1, len(mesh_copy.uv_layers)),
        'element_size': element_size,
        'vertex_buffer': vertex_buffer,
        'vertex_count': vertex_count,
        'index_buffer': index_buffer,
        'index_count': len(indices),
        'properties': get_props(obj.data)
    }
    return (True, mesh_name)

def save_blender_data():
    start = time.time()
    mesh_map = {}
    scenes = []

    for scene in bpy.data.scenes:
        entities = []

        def save_object(obj, matrix, data_name='NONE'):
            entities.append({
                'type': obj.type,
                'name': obj.name,
                'data_name': data_name,
                'matrix': struct.pack("<16f",
                    matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0],
                    matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1],
                    matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2],
                    matrix[0][3], matrix[1][3], matrix[2][3], matrix[3][3]),
                'properties': get_props(obj)
            })

        for obj in scene.objects:
            if obj.hide_render:
                continue

            if obj.type == 'MESH':
                result = save_mesh(obj, mesh_map)
                save_object(obj, obj.matrix_world, result[1])

            elif obj.type == 'EMPTY':
                if obj.dupli_group:
                    for child in obj.dupli_group.objects:
                        offset = Matrix.Translation(obj.dupli_group.dupli_offset).inverted()
                        matrix = obj.matrix_world * offset * child.matrix_world
                        # TODO: fix
                        #result = save_mesh(child, mesh_map)
                        #save_object(child, 0, matrix, result[1])
                        data_name = namePrefix(
                                bpy.data.filepath if child.library == None else child.library.filepath) + child.data.name
                        save_object(child, matrix, data_name)
                else:
                    save_object(obj, obj.matrix_world)

        scenes.append({
            'type': 'scene',
            'name': namePrefix() + scene.name,
            'entities': entities,
            'properties': get_props(scene)
        })

    out_file = os.path.join('bin', os.path.relpath(bpy.data.filepath, 'assets').replace('.blend', '.dat'))
    try:
        os.makedirs(os.path.dirname(out_file))
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise

    meshes = []
    for mesh in mesh_map.values():
        meshes.append(mesh)
    with open(out_file, 'bw') as file:
        file.write(struct.pack('<I', 0x00001111))
        file.write(pack_value({
            'meshes': meshes,
            'scenes': scenes
        }))
        print('Saved to file:', out_file)

    print('Export took', time.time() - start, 'seconds')
    return {'FINISHED'}

if __name__ == "__main__":
    save_blender_data()

