import time
from time import sleep
from ili9341 import Display, color565
from machine import Pin, SPI
from rotary_irq_rp2 import RotaryIRQ  


class Encoder:
  def __init__(self):
      self.rotate = RotaryIRQ(pin_num_clk=2, 
              pin_num_dt=3,
              reverse=False, 
              range_mode=RotaryIRQ.RANGE_UNBOUNDED)
      self.val_old = self.rotate.value()

  def rotation(self):
      val_new = self.rotate.value()
      if self.val_old != val_new:
        dif = val_new - self.val_old
        self.val_old = val_new
        print(val_new)
        time.sleep(0.05)
        return dif
      return 0



class Button:
    def __init__(self, pin_number):
      self.pin_number = pin_number
      self.pin = Pin(pin_number, Pin.IN, Pin.PULL_UP)
      self.is_pressed = False
      self.do = False


    def check_click(self):
      if self.pin.value() == 0 and self.is_pressed == False:
        self.is_pressed = True
        self.do = not self.do
        time.sleep(0.1)
        return True

      if self.pin.value() == 1 and self.is_pressed == True:
        self.is_pressed = False
        time.sleep(0.1)

      return False
      
class Player:
    def __init__(self):
        self.center_button = Button(5)
        self.preview_button = Button(8)
        self.next_button = Button(9)
        self.volume_button = Button(6)
        self.position_button = Button(7)
        self.encoder = Encoder()
        self.is_play = False
        self.is_volume_mode  = False
        self.is_position_mode = False
        self.idle = True
        self.volume = 50

    

    def control(self):
        self.dif = self.encoder.rotation()
        if self.is_volume_mode and self.dif != 0:
            self.set_volume()

        if self.center_button.check_click():
            self.is_play = not self.is_play
            if self.is_play == True: 
                print("play")
            if self.is_play == False:
               print("pause")

        elif self.position_button.check_click() and self.is_volume_mode == False:
            self.is_position_mode = not self.is_position_mode
            if self.is_position_mode == True:
                print("modo posicao")
                self.idle = False
            if self.is_position_mode == False:
                 print("mode normal")
                 self.idle = True

        elif self.volume_button.check_click() and self.is_position_mode == False:
            self.is_volume_mode = not self.is_volume_mode
            if self.is_volume_mode == True:
                print("modo volume")
                self.idle = False
            if self.is_volume_mode == False:
               print("modo normal")
               self.idle = True

        elif self.preview_button.check_click() and self.idle == True:
            self.idle = not self.idle
            print("Musica anterior")
            self.idle = not self.idle
        elif self.next_button.check_click() and self.idle == True:
            self.idle = not self.idle
            print("proxima musica")
            self.idle = not self.idle

    def set_volume(self):
       self.volume = self.volume + self.dif
       if self.volume >= 100:
            self.volume = 100
       elif self.volume <= 0:
            self.volume = 0
       print("volume = ", self.volume )  
       
       

my_player = Player()
#config de exibição
spi =SPI(0, baudrate=40000000, sck=Pin(18), mosi=Pin(19))
display = Display(spi, dc=Pin(14), cs=Pin(17), rst=Pin(15),
width=320, height= 240, rotation= 270)

  
while True:
    
 my_player.control()
 time.sleep(0.01)
 





# // clocar memororia para volume e bloquear mudança de volume sem estar no modo de volume(agora ele continuar contando mesmo sem estar e add quando entra )