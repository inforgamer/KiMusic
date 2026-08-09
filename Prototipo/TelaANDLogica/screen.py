
from ili9341 import Display, color565
from machine import Pin, SPI

class Screen:
    def __init__(self):
        self.spi =SPI(0, baudrate=40000000, sck=Pin(18), mosi=Pin(19))
        self.display = Display(self.spi, dc=Pin(14), cs=Pin(17), rst=Pin(15), width=320, height= 240, rotation= 270)
    
    def text(self):
        self.display.draw_text8x8(16, 16, 'MicroPython', color565(255, 255, 0))
