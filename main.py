import machine
import time
import _thread
from machine import Timer

#pins
txser = machine.Pin(17, machine.Pin.OUT)
rxser = machine.Pin(16, machine.Pin.IN)
intled = machine.Pin(25, machine.Pin.OUT)

#backside/transcieving
class Individual:
    header = "101010101011"
    header_tuple = ["1", "0", "1", "0", "1", "0", "1", "0", "1", "0", "1", "1"]
    
    def __init__(self, footprint, bit_duration):
        self.bit_duration = bit_duration
        self.samplerate = (1000000 // self.bit_duration) * 8
        self.tx = Transmitter(footprint, self)
        self.rx = Receiver(footprint, self)
        self.message = ""
        self.txMsg = ""
        
    def stopRx(self):
        self.rx.stopclock()
        self.bit_buffer = []
        self.ticks = 0
        self.synced = False
        self.sampled = False
        self.prev_val = rxser.value()
        
    def decodeLoop_core1(self):
        #to run on core 1
        while True:
            if self.rx.dataReady:
                self.rx.decode(self.rx.bit_buffer)
                
                if self.message.endswith("led on "):
                    intled.high()
                elif self.message.endswith("led off "):
                    intled.low()
                    
                time.sleep_us(int(self.bit_duration // 12.5))
                print("recieved: ", self.message)
                print("----------------------------------------")
        
    def startTx(self, msg):
        txser.low()
        self.txMsg = msg
        
        txser.high()
        time.sleep_us(self.bit_duration * 2)
        txser.low()
        time.sleep_us(self.bit_duration * 2)
                
        self.tx.transmit(self.tx.encode(" "))
        time.sleep_us(int(self.bit_duration * 2.6))
        self.tx.transmit(self.tx.encode(" "))
        time.sleep_us(int(self.bit_duration * 2.6))
        
        for char in tuple(self.txMsg):
            self.tx.transmit(self.tx.encode(char))
            print(char, " has been sent with footprint: ", self.tx.footprint)
            time.sleep_us(int(self.bit_duration * 2.6))
        self.tx.transmit(self.tx.encode(" "))
        time.sleep_us(int(self.bit_duration * 2.6))
        

class Transmitter:
    def __init__(self, footprint, parent):
        self.footprint = footprint #identification
        self.parent = parent
        
    def encode(self, char):
        #content
        charbin = f"{ord(char):08b}"
        content = "".join([self.footprint, charbin])
        
        #encoding
        manchester = "".join({"0": "01", "1": "10"}[b] for b in content)
        finaltransmit = "".join([Individual.header, manchester])
        
        return finaltransmit
    
    def transmit(self, code):
        for pulse in code:
            if pulse == "1":
                txser.high()
            else:
                txser.low()
            time.sleep_us(self.parent.bit_duration//2)
        txser.low()

class Receiver:
    def __init__(self, footprint, parent):
        #id and clock
        self.footprint = footprint #identification
        self.tim = Timer(-1)
        self.parent = parent
        
        #isr
        self.ticks = 0
        self.prev_val = rxser.value()
        self.bit_buffer = []
        
        #flags
        self.sampled = False #check if sampled each bit
        self.synced = False #check if locked to a transmission through header
        self.dataReady = False #check if transmission finished
        
        #decoding
        self.string_buffer = ""
        
    def _callback(self, t):
        #cant allocate memory for arrays or do cpu intense in here, because it happens 800hz and GC will mess stuff up
        current_val = rxser.value()
        
        #clock recovery, reset ticks at middle of bit
        if current_val != self.prev_val:
            if self.ticks > 6:
                #print("transition detected at tick: ", self.ticks, ", current_val at 75% of previous bit: ", current_val)
                self.ticks = 0
                self.sampled = False
            self.prev_val = current_val
        
        self.ticks += 1
        
        #unsynced header check
        if self.ticks == 2 and not self.synced or self.ticks == 6 and not self.synced:
            bit = "0" if current_val == 0 else "1"
            self.bit_buffer.append(bit)
            
            if len(self.bit_buffer) >= 12:
                match = True
                for i in range(12):
                    if self.bit_buffer[-12 + i] != Individual.header_tuple[i]:
                        match = False
                        break
                
                if match:
                    print("matched onto header")
                    print(self.bit_buffer)
                    self.synced = True
                    self.bit_buffer = []
        
        #synced data storage
        if self.synced:
            if self.ticks == 2 and not self.sampled:
                bit = "1" if current_val == 0 else "0"
                self.bit_buffer.append(bit)
                self.sampled = True
            
        if self.ticks > 16 and self.prev_val == current_val and self.synced and len(self.bit_buffer) >= len(self.footprint) + 8 and not self.dataReady:
            self.dataReady = True
    
    def start(self):
        #starts listening for header(self.synced)
        self.sampled = False
        self.bit_buffer = []
        self.ticks = 0
        self.synced = False
        self.tim.init(freq=self.parent.samplerate, mode=Timer.PERIODIC, callback=self._callback)
        self.prev_val = rxser.value()
        
    def stopclock(self):
        self.tim.deinit()
        time.sleep_ms(100)

    
    def decode(self, buffer):
        if self.dataReady:
            self.ticks = 0
            self.synced = False
            
            #convert to string and clear bit_buffer
            self.string_buffer = "".join(self.bit_buffer)
            print(self.string_buffer)
            self.bit_buffer = []
            
            #flags
            self.dataReady = False
            self.synced = False
            self.sampled = False
            self.ticks = 0
            
            if self.string_buffer[0:len(self.footprint)] == self.footprint:
                self.parent.message += chr(int(self.string_buffer[len(self.footprint):len(self.footprint) + 8], 2))
            else:
                print("unmatching footprint")
            
            self.dataReady = False

#loopback
intled.low()
heydude = Individual("110110", 5000)

heydude.rx.start()
_thread.start_new_thread(heydude.decodeLoop_core1, ())

while True:
    heydude.startTx(input("sending: "))

#9459940614
