from time import sleep
from ili9341 import Display, color565
from machine import Pin,SPI
import time

#config de exibição
spi =SPI(0, baudrate=40000000, sck=Pin(18), mosi=Pin(19))
display = Display(spi, dc=Pin(14), cs=Pin(17), rst=Pin(15), width=320,height=240, rotation=270)

def wait_pin_change(pin):
    cur_value = pin.value()
    active = 0
    while active < 20:
        if pin.value() != cur_value:
            active += 1
        else:
            active = 0
        
        time.sleep_ms(1)




#funções de controle
def setControle():
    tempo = 0
    # o tipo de controle (para volume ou para play/pause sera definido pelo tempo de pressionamento do botao central)
    pin_menu = Pin(5,Pin.IN, Pin.PULL_UP)
    while True:
        wait_pin_change(pin_menu)
        tempo += 1
        display.fill_rectangle(0, 0, 50, 20, 0)
        display.draw_text8x8(0, 0, f"{tempo}", color565(255, 255, 255))







