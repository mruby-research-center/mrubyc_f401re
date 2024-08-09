$sleep_time = 100

led = GPIO.new("PA5", GPIO::OUT)

while true
  led.write( 1 )
  sleep_ms $sleep_time
  led.write( 0 )
  sleep_ms $sleep_time
end
