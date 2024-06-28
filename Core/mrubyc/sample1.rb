pb3 = GPIO.new("PB3", GPIO::OUT)  # PB3ピンを出力に設定する

while true
  pb3.write( 1 )
  sleep 1
  pb3.write( 0 )
  sleep 1
end
