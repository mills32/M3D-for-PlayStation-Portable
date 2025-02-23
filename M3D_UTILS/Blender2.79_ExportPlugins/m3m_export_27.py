# This script is licensed as public domain.
# Export models for use in AMGLib for PSP.

bl_info = {
	"name": "PSP M3M Morphing Model",
	"author": "Mills",
	"version": (2024, 2, 0),
	"blender": (2, 7, 9),
	"location": "File > Export > PSP M3M Morphing Model",
	"description": "PlayStation Portable Morphing object for use with M3D",
	"warning": "Only 8 frames",
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

def exportM3M(self,context, filename):
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
	#get animation range
	startFrame = currentScene.frame_start
	endFrame = currentScene.frame_end
	frame = startFrame
	range = endFrame-startFrame
	if range > 8:
		self.report({'ERROR'}, 'Only 8 frames supported. Go to "RENDER" tab and set the frame range to be 8 frames long')
	
	DATA = [0]*triangle_number*3*11*range
	i = 0
	while frame < range:
		currentScene.frame_set(frame)
		mat = obj.matrix_world
		frameobj = obj.to_mesh(bpy.context.scene, True, 'RENDER',False,False)
		facex = 0
		vertex = 0
		for face in frameobj.polygons:
			for vert, loop in zip(face.vertices, face.loop_indices): #Y Z / NY -NZ / v -v 
				DATA[i] = frameobj.uv_layers.active.data[loop].uv[0]
				DATA[i+1] = -frameobj.uv_layers.active.data[loop].uv[1]
				DATA[i+2] = frameobj.vertex_colors.active.data[loop].color[0]
				DATA[i+3] = frameobj.vertex_colors.active.data[loop].color[1]
				DATA[i+4] = frameobj.vertex_colors.active.data[loop].color[2]                
				DATA[i+5] = int(frameobj.vertices[vert].normal[0]*127)
				DATA[i+6] = int(frameobj.vertices[vert].normal[2]*127)
				DATA[i+7] = int(frameobj.vertices[vert].normal[1]*-127)
				DATA[i+8] = 2*frameobj.vertices[vert].co[0]
				DATA[i+9] = 2*frameobj.vertices[vert].co[2]
				DATA[i+10] = 2*-frameobj.vertices[vert].co[1]
				i = i+11
				vertex = vertex + 1
				if vertex == 3:
					vertex = 0
			facex = facex + 1
		frame = frame + 1
	currentScene.frame_set(0)
	#----------
	#Write file
	#----------
	#PSP and x86/64 PC are little endian, so "<"
	file = open(filename, 'wb')
	file.write(struct.pack("<16s", bytes('M3MPSP 2024#####', encoding="utf8")))
	file.write(struct.pack("<8s", bytes('-NTRIS: ', encoding="utf8")))
	file.write(struct.pack("<i",triangle_number))
	file.write(struct.pack("<4s", bytes('####', encoding="utf8")))
	file.write(struct.pack("<10s", bytes('-NFRAMES: ', encoding="utf8")))
	file.write(struct.pack("<b",endFrame))
	file.write(struct.pack("<5s", bytes('#####', encoding="utf8")))
	file.write(struct.pack("<6s", bytes('DATA: ', encoding="utf8")))
	file.write(struct.pack("<i",(endFrame+1)*triangle_number*3*28)) #28 = lenght in bytes
	file.write(struct.pack("<6s", bytes('######', encoding="utf8")))

	vertex_data = 0
	#write vertex data
	while vertex_data < triangle_number*3:
		m_frame = 0
		v_data = vertex_data*11
		while m_frame < range:
			file.write(struct.pack("<f",DATA[v_data]))
			file.write(struct.pack("<f",DATA[v_data+1]))       
			file.write(struct.pack('B',int(DATA[v_data+2] * 255)))
			file.write(struct.pack('B',int(DATA[v_data+3] * 255)))
			file.write(struct.pack('B',int(DATA[v_data+4] * 255))) 
			file.write(bytes([255])) #alpha           
			file.write(struct.pack("<b",DATA[v_data+5]))
			file.write(struct.pack("<b",DATA[v_data+6]))
			file.write(struct.pack("<b",DATA[v_data+7]))
			file.write(bytes([0])) #padding 
			file.write(struct.pack("<f",DATA[v_data+8]/2))
			file.write(struct.pack("<f",DATA[v_data+9]/2))
			file.write(struct.pack("<f",DATA[v_data+10]/2))
			v_data = v_data + (triangle_number*3*11)
			m_frame = m_frame + 1
		vertex_data = vertex_data + 1

	file.write(struct.pack("<16s", bytes('END            O', encoding="utf8")))
	bpy.data.meshes.remove(frameobj)
	file.close()
	file = None;
	currentScene.frame_set(0)

	self.report({'INFO'}, "Exported successfully")

	
class ExportM3M(bpy.types.Operator, bpy_extras.io_utils.ExportHelper):
	'''PSP M3D'''
	bl_idname = "export.m3m"
	bl_label = 'M3M'
	filename_ext = ".m3m"

	def execute(self, context):
		exportM3M(self, context, self.properties.filepath)
		return {'FINISHED'}

	def check(self, context):
		filepath = bpy.path.ensure_ext(self.filepath, '.m3m')
		return False

def menu_func(self, context):
	self.layout.operator(ExportM3M.bl_idname, text="PSP M3M Morphing Model")

def register():
	bpy.utils.register_module(__name__)
	bpy.types.INFO_MT_file_export.append(menu_func)

def unregister():
	bpy.utils.unregister_module(__name__)
	bpy.types.INFO_MT_file_export.remove(menu_func)

if __name__ == "__main__":
	register()

