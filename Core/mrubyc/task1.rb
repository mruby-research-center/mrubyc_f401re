#
# ST LPS25H
# MEMS pressure sensor: 260-1260 hPa absolute digital output barometer
# https://www.st.com/ja/mems-and-sensors/lps25h.html
#

class LPS25H
  I2C_ADRS = 0x5c

  #
  # init instance
  #
  def initialize( i2c_bus, address = I2C_ADRS )
    @bus = i2c_bus
    @address = address
  end

  #
  # init sensor
  #
  def init()
    d = @bus.read( @address, 1, 0x0f ).bytes
    raise "Sensor not found" if d[0] != 0xbd

    @bus.write( @address, 0x20, 0x90 )
  end

  #
  # meas
  #
  def meas()
    d = @bus.read( @address, 5, 0xa8 ).bytes

    return {
      :pressure => to_uint24(d[2], d[1], d[0]).to_f / 4096,
      :temperature => 42.5 + to_int16(d[4], d[3]).to_f / 480,
    }
  end


  def to_int16( b1, b2 )
    return (b1 << 8 | b2) - ((b1 & 0x80) << 9)
  end
  def to_uint24( b1, b2, b3 )
    return b1 << 16 | b2 << 8 | b3
  end
end


puts "LPS25H Barometer"
sensor = LPS25H.new( I2C.new )
sensor.init()

while true
  sleep 1

  data = sensor.meas()
  printf( "Temp:%4.1f C  Pres:%7.2f hPa\n",
          data[:temperature], data[:pressure] )
end
