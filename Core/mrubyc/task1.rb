a0 = ADC.new(0)
a1 = ADC.new(1)

while true
  v0 = a0.read_voltage()
  v1 = a1.read_raw()

  printf("A0 = %.3fV, A1 = %d (raw)\n", v0, v1)
  sleep 1
end
