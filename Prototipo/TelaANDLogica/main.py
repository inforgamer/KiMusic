import time
from time import sleep
from ili9341 import Display, color565
from machine import Pin, SPI
from rotary_irq_rp2 import RotaryIRQ  

#Play/Pause
botao_central = Pin(5,Pin.IN,Pin.PULL_UP)
is_play = False
is_pressed_play = False

#Next
botao_next = Pin(9,Pin.IN, Pin.PULL_UP)
is_pressed_next = False

#Preview
botao_preview = Pin(8,Pin.IN, Pin.PULL_UP)
is_pressed_preview = False

#Volume
botao_volume = Pin(6,Pin.IN,Pin.PULL_UP)
is_volume_mode = False
is_pressed_volume = False




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
def set_volume():
  print('Volume mode')

while True:
  #Play/Pause
  if botao_central.value() == 0 and is_pressed_play == False:
     is_pressed_play = True
     is_play = not is_play
     play(is_play)
     time.sleep(0.1)
  if botao_central.value() == 1 and is_pressed_play == True:
    is_pressed_play = False



  #Next
  if botao_next.value() == 0 and is_pressed_next == False:
      is_pressed_next = True
      next_music()
      time.sleep(0.1)
  if botao_next.value() == 1 and is_pressed_next == True:
      is_pressed_next = False
      time.sleep(0.1)



  #Preview
  if botao_preview.value() == 0 and is_pressed_preview == False:
    is_pressed_preview = True
    preview_music()
    time.sleep(0.1)
  if botao_preview.value() == 1 and is_pressed_preview == True:
    is_pressed_preview = False
    time.sleep(0.1)



  #Volume
  if botao_volume.value() == 0 and not is_pressed_volume:
    is_pressed_volume = True
    is_volume_mode = not is_volume_mode

    if is_volume_mode:
      set_volume()
    else:
      print("Saiu")

      time.sleep(0.1)

  if botao_volume.value() == 1 and is_pressed_volume:
    is_pressed_volume = False





  












