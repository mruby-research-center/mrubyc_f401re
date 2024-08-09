#
# プッシュスイッチクラス
#
class PushSwitch
  attr_accessor :polarity       # Set the value when the switch is pressed.
  attr_accessor :long_press_th

  # コンストラクタ
  def initialize(port)
    @polarity = 0
    @long_press_th = 10
    @long_press_cnt = 0

    @sw = GPIO.new(port, GPIO::IN)
    @sw1 = 1 - @polarity
  end

  # 読み込み
  #  (note) 一定期間（例：100ms）ごとにコールする
  def read
    @sw0 = @sw1
    @sw1 = @sw.read

    ret = (@sw1 == @polarity)
    if ret
      @long_press_cnt += 1
    else
      @long_press_cnt = 0
    end

    return ret
  end

  # スイッチを押されたか？
  def pressed?
    return @polarity == 0 ? (@sw0 > @sw1) : (@sw0 < @sw1)
  end

  # スイッチを離されたか？
  def released?
    return @polarity == 0 ? (@sw0 < @sw1) : (@sw0 > @sw1)
  end

  # スイッチを長押しされたか？
  def long_pressed?
    return @long_press_cnt == @long_press_th
  end
end


#
# メインループ
#
sw = PushSwitch.new("PC13")
n = 1

while true
  sw.read

  if sw.pressed?
    n += 1
    $sleep_time = n * 100
  end

  if sw.long_pressed?
    n = 1
    $sleep_time = n * 100
  end

  sleep_ms 100
end
