import time
from time import sleep
from ili9341 import Display, color565
from machine import Pin, SPI
from rotary_irq_rp2 import RotaryIRQ  


class Encoder:
  def __init__(self, min_val, max_val):
      self.rotate = RotaryIRQ(pin_num_clk=2, 
              pin_num_dt=3, 
              min_val=min_val, 
              max_val= max_val, 
              reverse=False, 
              range_mode=RotaryIRQ.RANGE_BOUNDED)
      self.val_old = self.rotate.value()

  def rotation(self) -> None:
    if volume_button.do == True or position_button.do == True:
      val_new = self.rotate.value()
      if self.val_old != val_new:
        self.val_old = val_new
        print(val_new)
        time.sleep(0.05)


volume = Encoder(0, 100)
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
        
        if self.pin_number == 5:
            play(self.do)
        elif self.pin_number == 8:
              preview_music()
        elif self.pin_number == 9:
              next_music()
        elif self.pin_number == 6:
              set_volume(self.do)
        elif self.pin_number == 7:
              set_position(self.do)
        time.sleep(0.1)
      if self.pin.value() == 1 and self.is_pressed == True:
        self.is_pressed = False
        time.sleep(0.1)
      
      
center_button = Button(5)
preview_button = Button(8)
next_button = Button(9)
volume_button = Button(6)
position_button = Button(7)



#config de exibição
spi =SPI(0, baudrate=40000000, sck=Pin(18), mosi=Pin(19))
display = Display(spi, dc=Pin(14), cs=Pin(17), rst=Pin(15),
width=320, height= 240, rotation= 270)

#Play/Pause
def play(is_play):
  if is_play:
    print('Musica Pausada')
  else:
    print('Musica Despausada')

#Avançar   
def next_music():
  print('Proxima Musica')

#Retroceder
def preview_music():
  print('Musica anterior')

#Volume
def set_volume(volume_mode):
  if volume_mode:
    print('Modo Volume')
    
    
  else:
    print('Modo Normal')

#Position
def set_position(position_mode):
   if position_mode:
      print("Modo de Posicao")
   else:
      print("Modo normal")
  
while True:
  center_button.check_click()
  preview_button.check_click()
  next_button.check_click()
  volume_button.check_click()
  position_button.check_click()

  
  volume.rotation()









#arrumar trava de botao para apenas um botao por vez// clocar memororia para volume e bloquear mudança de volume sem estar no modo de volume(agora ele continuar contando mesmo sem estar e add quando entra )