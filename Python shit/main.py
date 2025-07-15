from ledcube import Ledcube

print("Hello from the cube")
ledcube = Ledcube(None)
#ledcube.start()

inputVal = ''
while inputVal != 'stop':
    inputVal = input()
    if inputVal == '1':
        ledcube.writeSPI([0b01010101] * 24)
        
    elif inputVal == '0':
        ledcube.writeSPI([0] * 24)
    
    elif inputVal == 'p':
        ledcube.pushLayer()

print('Shutting down')
ledcube.shutdown()