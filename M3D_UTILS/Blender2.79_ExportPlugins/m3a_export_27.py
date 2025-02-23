# This script is licensed as public domain.
# Export models for use in AMGLib for PSP

bl_info = {
    "name": "PSP M3A Aimated Model",
    "author": "Mills",
    "version": (2024, 2, 0),
    "blender": (2, 7, 9),
    "location": "File > Export > PSP M3A Animated Model",
    "description": "PlayStation Portable Animated (Skinned) Model for use with M3D",
    "warning": "",
    "wiki_url": "",
    "tracker_url": "",
    "category": "Import-Export"}

import struct
import os, struct, math
import mathutils
import bpy
import bpy_extras.io_utils
from array import array
from math import cos
from math import sin
error0 = 0
error1 = 0
error2 = 0
error3 = 0
error4 = 0

def exportM3A(self,context, filename):
    currentScene = bpy.context.scene

    #set variables
    total_triangles = 0;
    armature = None
    bone_number = 0
    frame = 0
    frames = 0

    obj = bpy.context.object
    
    #Count triangles
    for face in obj.data.polygons:
        total_triangles = total_triangles + 1
    
    #Find armature
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    armature = obj.find_armature()
    if armature is None:
        self.report({'ERROR'}, 'Object has no armature')
        file.close()
        file = None;
        return
    
    #----------
    #Write file
    #----------
    #PSP and x86/64 PC are little endian, so "<"
    file = open(filename, 'w')
    file.write("PSP M3A: M3D SKINNED MODEL\n")
    file.write("--------------------------\n")

    #Get armature
    currentScene.frame_set(0)
    i = 0
    boneindex = []
    bonenames = []
    for bone in armature.pose.bones:
        j = 0
        bonenames.append(bone.name)
        if bone.parent: #get parent index
            for b in armature.pose.bones:
                if bone.parent.name == b.name:
                    boneindex.append(j)                
                else:
                    j = j + 1
        else:
            boneindex.append(-1)
        bone_number = bone_number + 1
        i = i + 1;
    file.write("NBONES %s\n" %(bone_number))
    
    #write armature
    i = 0
    for item in bonenames:
        file.write("b %s index %s parent %s\n" %(bonenames[i],i,boneindex[i]))
        i = i + 1

    currentScene.frame_set(0)
    #get animation range
    try:
        startFrame = int(armature.animation_data.action.frame_range.x)
        endFrame = int(armature.animation_data.action.frame_range.y)
    except:
        self.report({'ERROR'}, 'Armature has no animation frames. Insert frames in pose mode')
        return
    startFrame = int(armature.animation_data.action.frame_range.x)
    endFrame = int(armature.animation_data.action.frame_range.y)
    frames = int(armature.animation_data.action.frame_range.y+1)
    
    #Get rotation from edit bones
    bonerot_edit = [0]*bone_number
    bpy.context.scene.objects.active = armature
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    bpy.ops.object.mode_set(mode='POSE')
    
    file.write('NFRAMES %s\n'%(frames))
    for frame in range(startFrame, endFrame+1, 1):
        currentScene.frame_set(frame)
        eb = 0
        bonen = 0
        file.write("f\n")
        for bone in armature.pose.bones:
            bonepos = obj.matrix_world.inverted() * bone.head
            bonerot = bone.rotation_quaternion
            file.write(" %s" %(round(bonepos.x,6)))
            file.write(" %s" %(round(bonepos.z,6)))
            file.write(" %s" %(round(-bonepos.y,6)))
            file.write(" %s" %(round(-bonerot.x,6)))
            file.write(" %s" %(round(-bonerot.y,6)))
            file.write(" %s" %(round(-bonerot.z,6)))
            file.write(" %s\n" %(round(bonerot.w,6)))
    
    file.write("NFACES %s\n" %(total_triangles))
    #read triangles and write them
    for face in obj.data.polygons:
        #read faces
        i = 0
        DATA = [0]*9*3
        face_vertices = 0
        for vert, loop in zip(face.vertices, face.loop_indices): #Y Z / NY -NZ / v -v 
            DATA[i+1] = obj.data.uv_layers.active.data[loop].uv[0]
            DATA[i+2] = -obj.data.uv_layers.active.data[loop].uv[1]
            DATA[i+3] = obj.data.vertices[vert].normal[0]
            DATA[i+4] = obj.data.vertices[vert].normal[2]
            DATA[i+5] = -obj.data.vertices[vert].normal[1]
            DATA[i+6] = obj.data.vertices[vert].co[0]
            DATA[i+7] = obj.data.vertices[vert].co[2]
            DATA[i+8] = -obj.data.vertices[vert].co[1]
            #get vertex group index
            w = 0
            group_index = 0;
            for item in obj.data.vertices[vert].groups: 
                group_index = item.group;            
                w = w + 1
                # check the vertex uses only 1 group/bone
                if w > 1:
                    self.report({'ERROR'}, 'THERE IS A VERTEX USING MORE THAN 1 GROUP / BONE')
                    file.close()
                    file = None;
                    return
            #check the real bone index, vertex group index is not always the same      
            bone_index = 0;
            for name in bonenames:
                if obj.vertex_groups[group_index].name == name:
                    DATA[i] = bone_index
                bone_index = bone_index + 1

            i = i+9
            face_vertices = face_vertices + 1
            if face_vertices == 4:
                self.report({'ERROR'}, 'THERE IS A FACE WHICH IS NOT A TRIANGLE')
                return
        #write a face
        file.write("t\n")
        for i in range(0,9):
            file.write(" %s" %(round(DATA[i],6)))
        file.write("\n")
        for i in range(9,18):
            file.write(" %s" %(round(DATA[i],6)))
        file.write("\n")
        for i in range(18,27):
            file.write(" %s" %(round(DATA[i],6)))
        file.write("\n")
    
    #for i in range(32):
    #file.write("{} \n")
    #    file.write("{} ".format(boneweights[i+1]))
    #    file.write("{} ".format(boneweights[i+2]))

    block = 0
    
    file.close()
    file = None;
    currentScene.frame_set(0)
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.context.scene.objects.active = obj
    self.report({'INFO'}, "Exported successfully")

    
class ExportM3A(bpy.types.Operator, bpy_extras.io_utils.ExportHelper):
    '''PSP M3D'''
    bl_idname = "export.m3a"
    bl_label = 'M3A'
    filename_ext = ".m3a"

    def execute(self, context):
        exportM3A(self, context, self.properties.filepath)
        return {'FINISHED'}

    def check(self, context):
        filepath = bpy.path.ensure_ext(self.filepath, '.m3a')
        return False

def menu_func(self, context):
    self.layout.operator(ExportM3A.bl_idname, text="PSP M3A Animated Model")

def register():
    bpy.utils.register_module(__name__)
    bpy.types.INFO_MT_file_export.append(menu_func)

def unregister():
    bpy.utils.unregister_module(__name__)
    bpy.types.INFO_MT_file_export.remove(menu_func)

if __name__ == "__main__":
    register()

