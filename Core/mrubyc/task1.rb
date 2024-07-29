uart6 = UART.new(6)

while true
  s = uart6.gets
  next if !s.start_with?("$GNRMC")

  rmc = s.split(",")
  next if rmc[2] != "A"

  # 日付(UTC)
  day = rmc[9][0,2].to_i
  mon = rmc[9][2,2].to_i
  year = rmc[9][4,2].to_i + 2000

  # 時刻(UTC)
  hour = rmc[1][0,2].to_i
  min = rmc[1][2,2].to_i
  sec = rmc[1][4,2].to_i

  # 緯度
  south_north = rmc[4]
  latitude = rmc[3]

  # 経度
  east_west = rmc[6]
  longitude = rmc[5]

  printf( "%d/%2d/%2d %2d:%02d:%02d(UTC)  Lat:%s%s Long:%s%s\n",
          year, mon, day, hour, min, sec,
          latitude, south_north, longitude, east_west )
end
