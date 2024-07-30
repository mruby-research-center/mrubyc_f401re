uart2 = UART.new(2)
uart6 = UART.new(6)

while true
  n = uart2.bytes_available
  if n > 0
    uart6.write( uart2.read(n))
  end
  n = uart6.bytes_available
  if n > 0
    uart2.write( uart6.read(n))
  end
end
