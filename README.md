# GridSimUserControl

Qt application for the power-grid simulation game. Communicates with the
NXP MCU over serial to read switch/generator state and drive the LEDs and
display.

## Build

Requires Qt 5.15+ or Qt 6 with the SerialPort module.

```bash
qmake GridSimUserControl.pro
make
```

Or open `GridSimUserControl.pro` in Qt Creator.

On Debian/Ubuntu the dev packages are:
```
sudo apt install qtbase5-dev libqt5serialport5-dev qttools5-dev-tools
```

## Architecture overview

```
                ┌────────────────────────────────────┐
                │  Qt Application (this project)     │
                │                                    │
   ┌─────────┐  │   ┌─────────────────────────┐      │
   │  User   │──┼──▶│  MainWindow (UI + ctrl) │      │
   └─────────┘  │   └────────┬────────────────┘      │
                │            │                       │
                │   ┌────────▼─────────┐             │
                │   │ Grid evaluator   │ 10 Hz tick  │
                │   │ (evaluateGrid)   │             │
                │   └────────┬─────────┘             │
                │            │                       │
                │   ┌────────▼─────────┐             │
                │   │ SerialHandler    │             │
                │   │ (queue + framer) │             │
                │   └────────┬─────────┘             │
                └────────────┼───────────────────────┘
                             │ UART 115200
                             ▼
                ┌────────────────────────────────────┐
                │  NXP MCU                           │
                │  - reads pots (Solar/Wind/Coal/Gas)│
                │  - reads switches sw1..sw7         │
                │  - drives 6 LEDs on lines          │
                │  - drives 7-seg POINTS / SIM YEAR  │
                └────────────────────────────────────┘
```

The 100 ms tick is the heartbeat. Every tick the Qt app:

1. Recomputes every line's demand and supply from the player's slider
   inputs, the current bus topology, and the simulated year.
2. Updates the on-screen transmission table.
3. Pushes any LED state changes to the MCU as `cL<line><state>x`.
4. Every second, pushes year and score (`cYxxxxx`, `cSxxxxx`).

## MCU protocol

### Commands sent to MCU (Qt → MCU)

| Command   | Meaning                           |
|-----------|-----------------------------------|
| `cL61x`   | LED 6 ON                          |
| `cL60x`   | LED 6 OFF                         |
| `cY2028x` | Set displayed year to 2028        |
| `cS4750x` | Set displayed score to 4750       |

`c` = start, `x` = terminate.

### Telemetry from MCU (MCU → Qt)

Comma-separated `name=value` pairs, terminated with `\n`:

```
SUN=60,WIND=45,COAL=80,GAS=70,sw1=1,sw2=1,sw3=0,sw4=1,sw5=1,sw6=1,sw7=0
SOLAR_COST=12,WIND_COST=8,COAL_COST=20,GAS_COST=18
```

| Name(s)                       | Meaning                          |
|-------------------------------|----------------------------------|
| `SUN`, `SOLAR`                | Solar generation capacity (0-100)|
| `WIND`                        | Wind generation capacity         |
| `COAL`                        | Coal generation capacity         |
| `GAS`, `NUCLEAR`              | Gas generation capacity          |
| `*_COST`                      | Cost per unit energy             |
| `sw1`..`sw6`                  | Line switch states (0/1)         |
| `sw7`                         | Tie switch between bus bars      |
| `LINE<n>_DEMAND`              | Optional demand override         |
| `LINE<n>_SUPPLY`              | Optional supply override         |

## Grid simulation maths

### Bus power resolution

```
busOnePower = min(sunUse,  solarPower) + min(windUse, windPower)
busTwoPower = min(coalUse, coalPower)  + min(gasUse,  gasPower)

if (tieSwitchOn):
    combined    = busOnePower + busTwoPower
    busOnePower = combined
    busTwoPower = combined
```

### Per-line evaluation

```
yearFactor      = 1 + 0.05 × (simYear − 2024)             (5% growth/year)
levelReduction  = 1 − 0.15 × lineLevel  (clamped to ≥ 0.4)
demand          = BASE_DEMAND[i] × yearFactor × levelReduction
supplied        = switchOn ? min(demand, busPower / activeLines) : 0
ledOn           = switchOn AND (supplied >= demand) AND (demand > 0)
```

`BASE_DEMAND` is `{20, 25, 30, 35, 25, 20}` per line — tune to taste.

### Score increment (1 Hz, in updateScoreOverTime)

```
demandPercent    = totalSupplied / totalDemand × 100
renewablePercent = (sunUse + windUse) / totalUsed × 100
increment        = demandPercent/10 + renewablePercent/10
```

## Tuning knobs

In `mainwindow.h`:

- `BASE_DEMAND[6]`        — base load per line in % of bus capacity
- `TICK_INTERVAL_MS`      — system tick period (default 100 ms)
- `TICKS_PER_SIM_YEAR`    — how many ticks = one sim year (default 100 = 10 s)
- `MCU_STATUS_INTERVAL`   — how often to push year/score (default 10 ticks = 1 s)

In `evaluateGrid()`:

- The `0.05` factor in `yearFactor` controls demand growth rate.
- The `0.15` factor in `levelReduction` controls how much each upgrade helps.
