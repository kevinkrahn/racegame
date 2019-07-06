import bpy
import bmesh
from math import *
from mathutils import (Euler, Matrix, Vector)

minSegmentLength = 0.5
maxSegmentLength = 80
minAngleDiff = 0.0002

def pathLength(scene, curveObject):
    obj = curveObject.copy()
    obj.data = curveObject.data.copy()
    #obj.data.resolution_u = max(obj.data.resolution_u, 32)
    obj.data.extrude = 0
    obj.data.bevel_depth = 0
    obj.data.offset = 0
    mesh = obj.to_mesh(scene, False, 'PREVIEW')
    totalLength = 0
    for e in mesh.edges:
        vert0 = obj.matrix_world * mesh.vertices[e.vertices[0]].co
        vert1 = obj.matrix_world * mesh.vertices[e.vertices[1]].co
        totalLength += (vert0 - vert1).length
    return totalLength

class TrackPathOffset(bpy.types.Operator):
    bl_idname = "track.offset_path"
    bl_label = "Path Offset"
    bl_options = {'REGISTER', 'UNDO'}

    scaledOffset = bpy.props.FloatProperty(name="Scaled Offset", default=0.0, min=-1000.0, max=1000.0)
    constantOffset = bpy.props.FloatProperty(name="Constant Offset", default=0.0, min=-1000.0, max=1000.0)

    def execute(self, context):
        scene = context.scene
        cursor = scene.cursor_location

        obj = context.active_object
        if obj.type != 'CURVE':
            print("That's not a curve")
            return {'FINISHED'}

        empty = bpy.data.objects.new("empty", None)
        constraint = empty.constraints.new("FOLLOW_PATH")
        constraint.target = obj
        constraint.use_curve_follow = True
        constraint.use_curve_radius = True
        constraint.forward_axis = "FORWARD_X"
        constraint.up_axis = "UP_Y"
        constraint.use_fixed_location = True
        constraint.offset_factor = 0.0
        scene.objects.link(empty)

        verts = []
        steps = floor(pathLength(scene, obj) / minSegmentLength)
        prev_dir = None
        prev_p = None
        prev_v = None
        for i in range(steps):
            constraint.offset_factor = i / steps
            scene.update()
            p = empty.matrix_world * Vector((0, 0, 0))
            rightvecnorm = empty.matrix_world.to_euler().to_matrix() * Vector((0, 1, 0))
            scale = empty.matrix_world.to_scale().y
            rightvec = rightvecnorm * scale
            v = p + rightvec * self.scaledOffset + rightvecnorm * self.constantOffset
            dir = (v - prev_v).normalized() if prev_v else None
            if prev_v == None or prev_dir == None or (((1.0 - dir.dot(prev_dir) > minAngleDiff and (v - prev_v).length > minSegmentLength) or (v - prev_v).length > maxSegmentLength)):
                prev_v = v
                prev_dir = dir;
                verts.append(v)

        scene.objects.unlink(empty)

        curve = bpy.data.curves.new("path_lalalala", type='CURVE')
        curve.dimensions = "3D"
        polyline = curve.splines.new('NURBS')
        polyline.points.add(len(verts))
        polyline.use_cyclic_u = True
        for i, coord in enumerate(verts):
            polyline.points[i].co = (coord.x, coord.y, coord.z, 1)
        object = bpy.data.objects.new("path_offset", curve)
        bpy.context.scene.objects.link(object)

        return {'FINISHED'}

class TrackStripOffset(bpy.types.Operator):
    bl_idname = "track.offset_strip"
    bl_label = "Path Offset Strip"
    bl_options = {'REGISTER', 'UNDO'}

    scaledOffset = bpy.props.FloatProperty(name="Scaled Offset", default=0.0, min=-1000.0, max=1000.0)
    constantOffset = bpy.props.FloatProperty(name="Constant Offset", default=0.0, min=-1000.0, max=1000.0)
    scaledWidth = bpy.props.FloatProperty(name="Scaled Width", default=0.0, min=0.0, max=100.0)
    constantWidth = bpy.props.FloatProperty(name="Constant Width", default=0.5, min=0.0, max=100.0)
    verticalOffset = bpy.props.FloatProperty(name="Vertical Offset", default=0.0, min=-100.0, max=100.0)

    def execute(self, context):
        scene = context.scene
        cursor = scene.cursor_location

        obj = context.active_object
        if obj.type != 'CURVE':
            print("That's not a curve")
            return {'FINISHED'}

        empty = bpy.data.objects.new("empty", None)
        empty.location.z = self.verticalOffset
        constraint = empty.constraints.new("FOLLOW_PATH")
        constraint.target = obj
        constraint.use_curve_follow = True
        constraint.use_curve_radius = True
        constraint.forward_axis = "FORWARD_X"
        constraint.up_axis = "UP_Y"
        constraint.use_fixed_location = True
        constraint.offset_factor = 0.0
        scene.objects.link(empty)

        verts = []
        faces = []
        uv_verts = []
        uv_faces = []
        steps = floor(pathLength(scene, obj) / minSegmentLength)
        prev_v1_index = None
        prev_v2_index = None
        prev_v1_dir = None
        prev_v2_dir = None
        prev_center_p = None
        u = 0.0
        start_center_p = None
        for i in range(steps):
            constraint.offset_factor = i / steps
            scene.update()
            p = empty.matrix_world * Vector((0, 0, 0))
            rightvecnorm = empty.matrix_world.to_euler().to_matrix() * Vector((0, 1, 0))
            scale = empty.matrix_world.to_scale().y
            rightvec = rightvecnorm * scale
            v1 = p + rightvec * self.scaledOffset + rightvecnorm * self.constantOffset - rightvecnorm * self.constantWidth - rightvec * self.scaledWidth
            v2 = p + rightvec * self.scaledOffset + rightvecnorm * self.constantOffset + rightvecnorm * self.constantWidth + rightvec * self.scaledWidth
            center_p = (v1 + v2) * 0.5

            v1_dir = (v1 - verts[prev_v1_index]).normalized() if prev_v1_index else None
            v2_dir = (v2 - verts[prev_v2_index]).normalized() if prev_v2_index else None

            if i == 0:
                start_center_p = center_p
            if prev_center_p != None:
                u += (center_p - prev_center_p).length / (v1 - v2).length
            prev_center_p = center_p

            new_v1_index = prev_v1_index
            new_v2_index = prev_v2_index

            #make_new_v1 = prev_v1_index == None or prev_v1_dir == None or (((1.0 - v1_dir.dot(prev_v1_dir) > minAngleDiff and (v1 - verts[prev_v1_index]).length > minSegmentLength) or (v1 - verts[prev_v1_index]).length > maxSegmentLength) and v1_dir.dot(prev_v1_dir) > -0.2)
            #make_new_v2 = prev_v2_index == None or prev_v2_dir == None or (((1.0 - v2_dir.dot(prev_v2_dir) > minAngleDiff and (v2 - verts[prev_v2_index]).length > minSegmentLength) or (v2 - verts[prev_v2_index]).length > maxSegmentLength) and v2_dir.dot(prev_v2_dir) > -0.2)
            #make_new_v1 = make_new_v1 or (make_new_v2 and (v1 - verts[prev_v1_index]).length > minSegmentLength and v1_dir.dot(prev_v1_dir) > -0.2)
            #make_new_v2 = make_new_v2 or (make_new_v1 and (v2 - verts[prev_v2_index]).length > minSegmentLength and v2_dir.dot(prev_v2_dir) > -0.2)

            make_new_v1 = prev_v1_index == None or prev_v1_dir == None or (((1.0 - v1_dir.dot(prev_v1_dir) > minAngleDiff and (v1 - verts[prev_v1_index]).length > minSegmentLength) or (v1 - verts[prev_v1_index]).length > maxSegmentLength))
            make_new_v2 = prev_v2_index == None or prev_v2_dir == None or (((1.0 - v2_dir.dot(prev_v2_dir) > minAngleDiff and (v2 - verts[prev_v2_index]).length > minSegmentLength) or (v2 - verts[prev_v2_index]).length > maxSegmentLength))
            make_new_v1 = make_new_v1 or (make_new_v2 and (v1 - verts[prev_v1_index]).length > minSegmentLength)
            make_new_v2 = make_new_v2 or (make_new_v1 and (v2 - verts[prev_v2_index]).length > minSegmentLength)

            if make_new_v1:
                new_v1_index = len(verts)
                prev_v1_dir = v1_dir;
                verts.append(v1)
                uv_verts.append((u, 0))

            if make_new_v2:
                new_v2_index = len(verts)
                prev_v2_dir = v2_dir;
                verts.append(v2)
                uv_verts.append((u, 1))

            if (new_v1_index != prev_v1_index or new_v2_index != prev_v2_index) and i > 0:
                if new_v1_index != prev_v1_index and new_v2_index != prev_v2_index:
                    face = (new_v1_index, new_v2_index, prev_v2_index, prev_v1_index)
                    faces.append(face)
                    uv_faces.append([ uv_verts[face[0]],
                                      uv_verts[face[1]],
                                      uv_verts[face[2]],
                                      uv_verts[face[3]] ])
                elif new_v1_index != prev_v1_index:
                    face = (new_v1_index, prev_v2_index, prev_v1_index)
                    faces.append(face)
                    uv_faces.append([ uv_verts[face[0]],
                                      uv_verts[face[1]],
                                      uv_verts[face[2]] ])
                else:
                    face = (new_v2_index, prev_v2_index, prev_v1_index)
                    faces.append(face)
                    uv_faces.append([ uv_verts[face[0]],
                                      uv_verts[face[1]],
                                      uv_verts[face[2]] ])

            prev_v1_index = new_v1_index
            prev_v2_index = new_v2_index

        last_face = (0, 1, len(verts) - 1, len(verts) - 2);
        faces.append(last_face)
        u += (start_center_p - prev_center_p).length / (verts[0] - verts[1]).length
        uv_faces.append([ (u, 0),
                          (u, 1),
                          uv_verts[last_face[2]],
                          uv_verts[last_face[3]] ])

        scene.objects.unlink(empty)

        mesh = bpy.data.meshes.new("path_strip")
        bm = bmesh.new()
        uv_layer = bm.loops.layers.uv.verify()
        bm.faces.layers.tex.verify()
        for v in verts:
            bm.verts.new(v)
        bm.verts.ensure_lookup_table()
        for i, f in enumerate(faces):
            d = bm.faces.new([bm.verts[i] for i in f])
            for j, l in enumerate(d.loops):
                l[uv_layer].uv = uv_faces[i][j]
        bm.to_mesh(mesh)

        object = bpy.data.objects.new("path_strip", mesh)
        bpy.context.scene.objects.link(object)

        return {'FINISHED'}

class TrackToolsPanel(bpy.types.Panel):
    bl_label = "Track Tools"
    bl_idname = "track_panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'TOOLS'
    bl_category = 'Track'

    def draw(self, context):
        layout = self.layout

        row = layout.row()
        row.operator("track.offset_strip")
        row = layout.row()
        row.operator("track.offset_path")

def register():
    bpy.utils.register_class(TrackStripOffset)
    bpy.utils.register_class(TrackPathOffset)
    bpy.utils.register_class(TrackToolsPanel)

def unregister():
    bpy.utils.unregister_class(TrackStripOffset)
    bpy.utils.unregister_class(TrackPathOffset)
    bpy.utils.unregister_class(TrackToolsPanel)

if __name__ == "__main__":
    register()
