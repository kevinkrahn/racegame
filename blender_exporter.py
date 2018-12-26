import bpy, bmesh, struct, time, os
from math import *
from mathutils import (Euler, Matrix, Vector)

def pack_string(string):
    MAX_LENGTH = 64 # must match ASSET_NAME_MAX_LENGTH in asset_types.h
    length = len(string)
    if length+1 > MAX_LENGTH:
        raise ValueError('Name ({}) is too long! Max length is {} characters.'.format(string, MAX_LENGTH - 1))
    charBytes = bytes(string, 'ASCII')
    pad = [0] * (MAX_LENGTH - length)
    return struct.pack("<" + str(length) + "s" + str(len(pad)) + "B", charBytes, *pad)

def namePrefix(path=bpy.data.filepath):
    return os.path.splitext(os.path.basename(path))[0] + '.'

def save_mesh(obj, mesh_map, mesh_buffer):
    mesh_name = namePrefix() + obj.data.name
    if len(obj.modifiers) > 0:
        mesh_name = namePrefix() + obj.name
    elif obj.library != None:
        return (False, namePrefix(obj.library) + obj.data.name)

    if(mesh_map.get(mesh_name, None) != None):
        return (False, mesh_name)
    else:
        mesh_map[mesh_name] = True

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

    indices = []
    vertex_buffer = bytearray()
    vertex_dict = {}
    vertex_count = 0
    vertex_list = []
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

                vertex_tuple = (position, normal);
                for c in colors:
                    vertex_tuple = vertex_tuple + (c[0], c[1], c[2])
                for u in tex_coords:
                    vertex_tuple = vertex_tuple + (u[0], u[1])

                vertex_hash = str(vertex_tuple)

                index = vertex_dict.get(vertex_hash, None)
                if index == None:
                    index = vertex_count
                    vertex_count += 1
                    vertex_dict[vertex_hash] = index
                    vertex_buffer += struct.pack("<6f", position[0], position[1], position[2],
                                                        normal[0], normal[1], normal[2])
                    for c in colors:
                        vertex_buffer += struct.pack("<3f", c[0], c[1], c[2])
                    for u in tex_coords:
                        #print("UV: " + str(u[0]) + "," + str(u[1]))
                        vertex_buffer += struct.pack("<2f", u[0], u[1])

                indices.append(index)

    elif len(mesh.edges) > 0:
        element_size = 2
        vertex_count = len(mesh_copy.vertices)
        for v in mesh_copy.vertices:
            vertex_buffer += struct.pack("<3f", v.co[0], v.co[1], v.co[2])

        for e in mesh_copy.edges:
            for i in e.vertices:
                indices.append(i)

    else:
        element_size = 1
        return (False, 'NONE')

    # TODO: export mesh bounding box
    matrix = obj.matrix_world
    mesh_buffer += pack_string(mesh_name)
    mesh_buffer += struct.pack("<IIII16f", max(1, len(mesh_copy.vertex_colors)),
                            max(1, len(mesh_copy.uv_layers)), vertex_count, len(indices),
                            matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0],
                            matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1],
                            matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2],
                            matrix[0][3], matrix[1][3], matrix[2][3], matrix[3][3])
    mesh_buffer += struct.pack("<I", element_size)
    mesh_buffer += vertex_buffer
    mesh_buffer += struct.pack("<" + "I"*len(indices), *indices)
    return (True, mesh_name)

def save_path(obj, path_buffer, scene):
    obj.data.resolution_u = 5
    obj.data.extrude = 0
    obj.data.bevel_depth = 0
    obj.data.offset = 0
    path_mesh = obj.to_mesh(scene, False, 'PREVIEW')

    path_buffer += pack_string(namePrefix() + obj.name)
    path_buffer += struct.pack("<I", len(path_mesh.vertices))

    for v in path_mesh.vertices:
        p = obj.matrix_world * v.co
        path_buffer += struct.pack("<fff", p.x, p.y, p.z)

    return (True, namePrefix() + obj.name)


def save_blender_data():
    start = time.clock()

    print("Exporting blender data...")

    mesh_counter = 0
    path_counter = 0

    mesh_map = {}

    mesh_buffer  = bytearray()
    path_buffer  = bytearray()
    scene_buffer = bytearray()

    scene_buffer += struct.pack("<I", len(bpy.data.scenes))


    for scene in bpy.data.scenes:
        current_scene_entity_buffer = bytearray()
        obj_count = 0

        def save_object(obj, data_type, matrix, data_name='NONE'):
            nonlocal obj_count, current_scene_entity_buffer
            obj_count += 1

            current_scene_entity_buffer += pack_string(obj.name)
            current_scene_entity_buffer += pack_string(data_name)
            current_scene_entity_buffer += struct.pack("<16f",
                    matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0],
                    matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1],
                    matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2],
                    matrix[0][3], matrix[1][3], matrix[2][3], matrix[3][3])
            current_scene_entity_buffer += struct.pack("<I", data_type)
            settings = obj.my_settings
            current_scene_entity_buffer += struct.pack("<I", int(settings.entity_type))
            current_scene_entity_buffer += struct.pack("<I", settings.collision_enabled)
            current_scene_entity_buffer += struct.pack("<I", settings.cast_shadows)
            current_scene_entity_buffer += pack_string(settings.texture)
            current_scene_entity_buffer += pack_string(settings.shader)

        for obj in scene.objects:
            if obj.hide_render:
                continue

            if obj.type == 'MESH':
                result = save_mesh(obj, mesh_map, mesh_buffer)
                if result[0]:
                    mesh_counter += 1
                save_object(obj, 0, obj.matrix_world, result[1])

            elif obj.type == 'CURVE':
                result = save_path(obj, path_buffer, scene)
                if result[0]:
                    path_counter += 1
                save_object(obj, 1, obj.matrix_world, result[1])

            elif obj.type == 'EMPTY':
                if obj.dupli_group:
                    for child in obj.dupli_group.objects:
                        offset = Matrix.Translation(obj.dupli_group.dupli_offset).inverted()
                        matrix = obj.matrix_world * offset * child.matrix_world
                        # TODO: fix
                        #result = save_mesh(child, mesh_map, mesh_buffer)
                        #save_object(child, 0, matrix, result[1])
                        data_name = namePrefix(
                                bpy.data.filepath if child.library == None else child.library.filepath) + child.data.name
                        save_object(child, 0, matrix, data_name)
                else:
                    save_object(obj, 2, obj.matrix_world)

        current_scene_buffer = bytearray()
        current_scene_buffer += pack_string(namePrefix() + scene.name)
        current_scene_buffer += struct.pack("<I", obj_count)
        current_scene_buffer += current_scene_entity_buffer

        scene_buffer += struct.pack("<I", len(current_scene_buffer))
        scene_buffer += current_scene_buffer


    out_file = os.path.join('bin', os.path.basename(bpy.data.filepath) + '.dat')
    f = open(out_file, 'bw')
    f.write(scene_buffer)
    f.write(struct.pack("<I", mesh_counter))
    f.write(mesh_buffer)
    f.write(struct.pack("<I", path_counter))
    f.write(path_buffer)
    f.close()

    print("Saved to file: ", out_file)

    #print("Export took", time.clock()-start, "seconds")
    return {'FINISHED'}

#def save_handler(dummy):
    #pass

if __name__ == "__main__":
    save_blender_data()

