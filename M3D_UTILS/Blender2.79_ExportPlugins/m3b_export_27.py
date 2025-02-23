# This script is licensed as public domain.
# Export models for use in M3D PSP.
# Github:

bl_info = {
	"name": "PSP M3B binary Model",
	"author": "Mills",
	"version": (2024, 2, 0),
	"blender": (2, 7, 9),
	"location": "File > Export > PSP M3B binary Model",
	"description": "PlayStation Portable binary Model for use with M3D",
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

def exportM3B(self,context, filename):
	currentScene = bpy.context.scene
	bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)

	#get selected objects
	obj = bpy.context.selected_objects
	#set variables
	triangle_number = 0

	obj = context.active_object
	obj.data.calc_normals_split()
	obj.data.update(calc_edges=True, calc_tessface=True)
	triangle_number = 0	
	number = 0
	for face in obj.data.polygons:
		triangle_number = triangle_number + 1
	
	#apply transformations to meshes
	bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
	DATA = [0]*triangle_number*3*8
	i = 0

	obj.data.calc_normals_split()
	obj.data.update(calc_edges=True, calc_tessface=True)
	mat = obj.matrix_world
	frameobj = obj.to_mesh(scene=bpy.context.scene, apply_modifiers=True, settings='PREVIEW')
	#dat = frameobj.loop[0].triangles
	facex = 0
	vertex = 0
	for face in frameobj.polygons:
		#test = face.loop.#Triangles[0].split_normals[0]
		for vert, loop in zip(face.vertices, face.loop_indices): #Y Z / NY -NZ / v -v 
			DATA[i] = frameobj.uv_layers.active.data[loop].uv[0]
			DATA[i+1] = -frameobj.uv_layers.active.data[loop].uv[1]
			DATA[i+2] = frameobj.vertex_colors.active.data[loop].color[0]
			DATA[i+3] = frameobj.vertex_colors.active.data[loop].color[1]
			DATA[i+4] = frameobj.vertex_colors.active.data[loop].color[2]
			DATA[i+5] = 2*frameobj.vertices[vert].co[0]
			DATA[i+6] = 2*frameobj.vertices[vert].co[2]
			DATA[i+7] = 2*-frameobj.vertices[vert].co[1]
			i = i+8
			vertex = vertex + 1
			if vertex == 3:
				vertex = 0
		facex = facex + 1
	currentScene.frame_set(0)
	#----------
	#Write file
	#----------
	#PSP and x86/64 PC are little endian, so "<"
	file = open(filename, 'wb')
	file.write(struct.pack("<16s", bytes('M3BPSP 2024#####', encoding="utf8")))
	file.write(struct.pack("<8s", bytes('-NTRIS: ', encoding="utf8")))
	file.write(struct.pack("<i",triangle_number))
	file.write(struct.pack("<4s", bytes('####', encoding="utf8")))
	file.write(struct.pack("<6s", bytes('DATA: ', encoding="utf8")))
	file.write(struct.pack("<i",triangle_number*3*24)) #24 = length in bytes
	file.write(struct.pack("<6s", bytes('######', encoding="utf8")))

	v_data = 0
	#write vertex data
	vertex_data = 0
	while vertex_data < triangle_number*3:
		file.write(struct.pack("<f",DATA[v_data]))
		file.write(struct.pack("<f",DATA[v_data+1]))
		file.write(struct.pack('B',int(DATA[v_data+2] * 255)))
		file.write(struct.pack('B',int(DATA[v_data+3] * 255)))
		file.write(struct.pack('B',int(DATA[v_data+4] * 255)))
		file.write(bytes([255])) #alpha  
		file.write(struct.pack("<f",DATA[v_data+5]/2))
		file.write(struct.pack("<f",DATA[v_data+6]/2))
		file.write(struct.pack("<f",DATA[v_data+7]/2))
		v_data = v_data + 8
		vertex_data = vertex_data + 1

	file.write(struct.pack("<16s", bytes('END            O', encoding="utf8")))
	bpy.data.meshes.remove(frameobj)
	file.close()
	file = None;
	currentScene.frame_set(0)

	self.report({'INFO'}, "Exported successfully")

	
class ExportM3B(bpy.types.Operator, bpy_extras.io_utils.ExportHelper):
	'''PSP M3D'''
	bl_idname = "export.m3b"
	bl_label = 'M3B'
	filename_ext = ".m3b"

	def execute(self, context):
		exportM3B(self, context, self.properties.filepath)
		return {'FINISHED'}

	def check(self, context):
		filepath = bpy.path.ensure_ext(self.filepath, '.m3b')
		return False

def menu_func(self, context):
	self.layout.operator(ExportM3B.bl_idname, text="PSP M3B binary Model")

def register():
	bpy.utils.register_module(__name__)
	bpy.types.INFO_MT_file_export.append(menu_func)

def unregister():
	bpy.utils.unregister_module(__name__)
	bpy.types.INFO_MT_file_export.remove(menu_func)

if __name__ == "__main__":
	register()

