import spidev
import RPi.GPIO as GPIO
import time
from concurrent.futures import ThreadPoolExecutor

DM_LATCH = 5
DM_EN = 22

MOS_OE=24
MOS_LATCH=27

ONE_SECOND=10**8


class Ledcube:

    currentLayer=0
    dmSpi=None
    mosSpi=None
    updateFunction=None
    running=False

    threadPool=ThreadPoolExecutor(max_workers=3)
    nextFrameTime=time.clock_gettime_ns(time.CLOCK_PROCESS_CPUTIME_ID)+1000000000/(30*8)

    layerData=[0]*24
    currentCube=[0]*24*3
    nextCube=[0]*24*3

    debug=0

    def __init__(self, updateFunction):
        self.updateFunction = updateFunction
        GPIO.setmode(GPIO.BCM)
        GPIO.setwarnings(False)
        GPIO.setup(DM_EN,GPIO.OUT)
        GPIO.setup(DM_LATCH,GPIO.OUT)
        GPIO.setup(MOS_OE,GPIO.OUT)
        GPIO.setup(MOS_LATCH,GPIO.OUT)
        self.dmSpi = spidev.SpiDev()
        self.dmSpi.open(0, 1)
        self.dmSpi.max_speed_hz = 20000000
        self.dmSpi.mode = 0
        self.mosSpi = spidev.SpiDev()
        self.mosSpi.open(0, 1)
        self.mosSpi.max_speed_hz = 1000000
        self.mosSpi.mode = 0
        for i in range(9):
            self.pushLayer()
        self.writeSPI([0] * 24)
        
    
    def pushLayer(self):
        self.mosSpi.writebytes([255 ^ (1 << self.currentLayer)])
        GPIO.output(MOS_LATCH, GPIO.HIGH)
        GPIO.output(MOS_LATCH, GPIO.LOW)

        self.currentLayer += 1
        self.currentLayer %= 8
    
    def writeSPI(self, value):
        self.dmSpi.writebytes(value)
        GPIO.output(DM_LATCH,GPIO.HIGH)
        GPIO.output(DM_LATCH,GPIO.LOW)

    def start(self):
        self.running=True
        self.threadPool.submit(self.startMainThread)
        pass

    def startMainThread(self):
        while self.running:
            #nextLayerData=self.threadPool.submit(self.calculateNextLayer)
            self.writeSPI([255]*24)
            #self.layerData = nextLayerData.result(1/10)
            self.debug += 1
            if self.nextFrameTime<time.clock_gettime_ns(time.CLOCK_PROCESS_CPUTIME_ID):
                self.nextFrameTime += ONE_SECOND/(30*8)
                self.pushLayer()
                print(self.debug)
                self.debug=0
        print("End of main loop")
        pass

    def calculateNextLayer(self):
        return [255]*24

    def calculateNextFrame(self):
        pass

    def shutdown(self):
        self.running = False
        self.threadPool.shutdown(wait=True)
        self.mosSpi.writebytes([0xFF])
        GPIO.output(MOS_LATCH, GPIO.HIGH)
        GPIO.output(MOS_LATCH, GPIO.LOW)
        self.writeSPI([0] * 24)
