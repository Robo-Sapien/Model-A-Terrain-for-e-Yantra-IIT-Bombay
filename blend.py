import bge
import bpy
import serial

port='COM7'
ser = serial.Serial(port,9600,timeout=10)
#print(ser)

control=bge.logic.getCurrentController().owner
scene= bge.logic.getCurrentScene()
#keyboard=bge.logic.keyboard

data=ser.read(10)
print(data)
#LongUpKey=(bge.logic.KX_INPUT_ACTIVE==keyboard.events[bge.events.UPARROWKEY])
print('Hello')
if  len(data)>1:
    #inp=input("Enter name... :  ")
    inp=bytearray(data).decode('latin-1')
    print('inp    '+inp)
    #inp=str(data)
    command=inp.split('_')
    #print (command)
    model = command[0]
    x,y,z=float(command[1]),float(command[2]),float(command[3])
    if(model=='cube'):
        #bpy.ops.mesh.primitive_cube_add(location=(x,y,z))
        newredcube=scene.addObject('redcube',control)
        newredcube.worldPosition = [x,y,z]
        newredcube.scaling=[0.5,0.5,0.5]
        print('Cube Created!!')
    elif(model=='cyli'):
        yellowcyl=scene.addObject('yellowcyl',control)
        yellowcyl.worldPosition = [x,y,z]
        yellowcyl.scaling=[0.5,0.5,0.5]
        print('Cylinder Created!!')
                   
else:
    pass
