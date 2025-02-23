import sys
import os
import random
import PIL.Image

filename = "tex_grass_base.png"
image = PIL.Image.open(filename)
pixels_in = image.load()
pixval = list(image.getdata())
max_x, max_y = image.size

#Fur donut:
#idensity = 12000
#fluffyness = 0.8
#halpha = 0
#translucency = 0.5
#nlayers = 8;

#Grass:
idensity = 2000
fluffyness = 0.6
halpha = 0
translucency = 0.2
nlayers = 8;

hlight = translucency
fluffyness = fluffyness*256
#create array of pixels
npixels = max_x*max_y
Data = [0]*nlayers
for layer in range(nlayers):
	Data[layer] = [0]*npixels

pixval = list(image.getdata())
for layer in range(nlayers):
	#set all pixels of the layer to 0
	for pixel in range(npixels):
		Data[layer][pixel] = (0,0,0,0)
	#Change density
	density = int(idensity / ((layer+1)*2))
	#Change darkness according to translucency
	hlight = hlight + ((1-translucency)/nlayers)
	#Change alpha according to fluffyness
	halpha = halpha + (fluffyness/nlayers)
	random.seed(28382)
	if layer == 0:
		for pixel in range(npixels):
			R,G,B,A = pixval[pixel]
			R = R *hlight
			G = G *hlight
			B = B *hlight
			Data[0][pixel] = (int(R),int(G),int(B),256)	
			pixel = pixel + 1
	else:
		for pixel in range(density):
			xrand = random.randint(0,max_x-1)
			yrand = random.randint(0,max_y-1)
			R,G,B,A = pixels_in[xrand,yrand]
			R = R *hlight
			G = G *hlight
			B = B *hlight
			A = 256-halpha;
			Data[layer][xrand+(yrand*max_x)] = (int(R),int(G),int(B),int(A))

if nlayers == 4:
	imglayer1 = Image.new('RGBA', (max_x*2, max_y*2), color = 'red')
	pixels_out = imglayer1.load()
	layer = 0
	#create a image with 8 layers (4x2) 
	for iy in range(2):
		for ix in range(2):
			pixelf = 0
			for y in range(max_y):
				for x in range(max_x):
					RGBA = Data[layer][pixelf]
					pixels_out[x+(ix*max_x),y+(iy*max_y)] = RGBA
					pixelf = pixelf + 1
			layer = layer + 1

if nlayers == 8:
	imglayer1 = PIL.Image.new('RGBA', (max_x*4, max_y*2), color = 'red')
	pixels_out = imglayer1.load()
	layer = 0
	#create a image with 8 layers (4x2) 
	for iy in range(2):
		for ix in range(4):
			pixelf = 0
			for y in range(max_y):
				for x in range(max_x):
					RGBA = Data[layer][pixelf]
					pixels_out[x+(ix*max_x),y+(iy*max_y)] = RGBA
					pixelf = pixelf + 1
			layer = layer + 1

if nlayers == 16:
	imglayer1 = PIL.Image.new('RGBA', (max_x*4, max_y*4), color = 'red')
	pixels_out = imglayer1.load()
	layer = 0
	#create a image with 8 layers (4x2) 
	for iy in range(4):
		for ix in range(4):
			pixelf = 0
			for y in range(max_y):
				for x in range(max_x):
					RGBA = Data[layer][pixelf]
					pixels_out[x+(ix*max_x),y+(iy*max_y)] = RGBA
					pixelf = pixelf + 1
			layer = layer + 1		
name, ext = os.path.splitext(filename)
outfilename = '{}_layers.png'.format(name, ext)
imglayer1.save(outfilename)

