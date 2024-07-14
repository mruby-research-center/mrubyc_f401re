#
# MAX31855
#  Cold-Junction Compensated Thermocouple-to-Digital Converter
#
#  https://www.maximintegrated.com/en/products/interface/sensor-interface/MAX31855.html
#
# Breakout board.
#  https://www.switch-science.com/catalog/864/
#
# Pin assign.
#  CN   pin     GPIO    Usage
#  CN7  1       PC10    SCK
#  CN7  2       PC11    MISO
#  CN7  3       PC12    MOSI
#  CN5  3       PB6     CS (SS)
#

puts "MAX31855 Thermo meter."

spi = SPI.new()
cs = GPIO.new("PB6", GPIO::OUT)
cs.write( 1 )
sleep_ms 100


while true
  cs.write( 0 )
  s = spi.read(4)
  cs.write( 1 )

  # DATA[31:18] * 0.25
  temp_tc =  (((s.getbyte(0) << 6) | (s.getbyte(1) >> 2)) -
              ((s.getbyte(0) & 0x80) << 7)) * 0.25

  # DATA[15:4] * 0.0625
  temp_std = (((s.getbyte(2) << 4) | (s.getbyte(3) >> 4)) -
              ((s.getbyte(2) & 0x80) << 5)) * 0.0625

  printf "TC=%.1f ℃  std=%.1f ℃\n", temp_tc, temp_std
  sleep 1
end
