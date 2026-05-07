#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSlider>
#include <QLCDNumber>
#include <QLabel>
#include <QPushButton>
#include <QStringList>
#include <QTimer>

#include "serialhandler.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    enum GeneratorType {
        GEN_SOLAR = 0,
        GEN_WIND  = 1,
        GEN_COAL  = 2,
        GEN_GAS   = 3
    };

private slots:
    void on_btn_ScanPorts_clicked();
    void on_combo_SerPorts_currentTextChanged(const QString &arg1);
    void on_btn_line4disconn_clicked();
    void on_pushButton_clicked();
    void onSysTick();

    void onSerialDataSignalRecieved(const QString &text);
    void onConnectionError(const QString &error);
    void onDisconnected();
    void on_btnConnectSerial_clicked();

    void on_btn_line1Upgrade_clicked();
    void on_btn_line2Upgrade_clicked();
    void on_btn_line3Upgrade_clicked();
    void on_btn_line4Upgrade_clicked();
    void on_btn_line5Upgrade_clicked();
    void on_btn_line6Upgrade_clicked();

    void on_horizontalSlider_valueChanged(int value);
    void on_horizontalSlider_2_valueChanged(int value);
    void on_horizontalSlider_3_valueChanged(int value);
    void on_horizontalSlider_4_valueChanged(int value);

    void on_btnStartGame_clicked();

    void on_SomeButtonDebug_clicked();

    // Generator upgrade slot — single handler dispatches to genIndex
    void onUpgradeGeneratorClicked();

private:
    void addEvent(const QString &message);
    void upgradeLine(int lineIndex);
    void initialiseUiToZero();
    void removeWrongDesignerConnections();

    void setUseLabels(const QStringList &labelNames, int value);
    void setResourceMaximum(QSlider *slider, QLCDNumber *maximumBox, int maximumValue);

    void initialiseTransmissionTable();
    void updateTransmissionLine(int line, int demand, int supplied,
                                int flow, int capacity, bool overloaded);

    void updateSummaryDisplays();

    int  calculateScoreIncrement();
    void updateScoreOverTime();
    void displayCurrentScore();

    void setGameControlsEnabled(bool enabled);

    QLCDNumber *findLcd(const QStringList &names) const;
    void displayLcd(const QStringList &names, int value);
    QLCDNumber *scoreDisplay() const;

    // Grid simulation helpers
    void evaluateGrid();
    void pushMcuState();

    // Display refresh / GUI year sync
    void displayYearOnGui(int year);
    void sendDisplayRefresh();
    void sendDisplayStartupBurst();

    // Generator upgrade tab
    void buildGeneratorTab();
    void refreshGeneratorTab();
    void upgradeGenerator(int genIndex);
    double genMultiplier(int genIndex) const;
    static QString genName(int genIndex);

    Ui::MainWindow *ui;
    SerialHandler *m_serial;
    QTimer *m_scoreTimer;
    QTimer *m_tickTimer;

    bool m_isConnected = false;
    bool gameRunning = false;
    bool twoPlayerMode = false;

    int lineLevel[6];
    int lineCost[6];
    bool lineSwitchOn[6];

    int currentScore = 0;
    int totalDemand = 0;
    int totalSupplied = 0;

    int sunUse = 0;
    int windUse = 0;
    int coalUse = 0;
    int gasUse = 0;

    int solarPower = 0;
    int windPower = 0;
    int coalPower = 0;
    int gasPower = 0;

    int solarCost = 0;
    int windCost = 0;
    int coalCost = 0;
    int gasCost = 0;

    // ── Grid simulation state ──
    int  m_simYear     = 2024;
    int  m_tickCount   = 0;
    bool m_lastLedState[6]   = {false, false, false, false, false, false};
    int  m_lastYearSent      = -1;
    int  m_lastScoreSent     = -1;

    // Switch state from MCU. Topology mapping (per physical board):
    //   m_switchOn[0] = SW1 ↔ line 2 / bus1   (wind feeder)
    //   m_switchOn[1] = SW2 ↔ line 3 / bus1   (tie line, bus1 side)
    //   m_switchOn[2] = SW3 ↔ line 4 / bus1   (DEF distribution)
    //   m_switchOn[3] = SW4 ↔ line 1 / bus1   (coal feeder)
    //   m_switchOn[4] = SW5 ↔ line 5 / bus2   (solar feeder)
    //   m_switchOn[5] = SW6 ↔ line 3 / bus2   (tie line, bus2 side)
    //   m_switchOn[6] = SW7 ↔ line 6 / bus2   (gas feeder)
    bool m_switchOn[7] = {true, true, true, true, true, true, true};

    // ── Generator upgrade state ──
    // Costs are 3× base, growing 1.7× per level → ~3× at L1, ~8× at L3.
    int  m_genLevel[4] = {0, 0, 0, 0};
    int  m_genCost[4]  = {600, 600, 750, 750};   // initial score cost per gen
    QLabel       *m_genLevelLbl[4]   = {nullptr, nullptr, nullptr, nullptr};
    QLabel       *m_genDescLbl[4]    = {nullptr, nullptr, nullptr, nullptr};
    QPushButton  *m_genUpgradeBtn[4] = {nullptr, nullptr, nullptr, nullptr};

    /*
     * Configure Game opts below!----------------------------------------------------------------------------------------------
     *










    */



    // Per-line baseline demand. Reflects the physical topology:
    //   Line 1 (coal feeder):     no load
    //   Line 2 (wind feeder):     load A
    //   Line 3 (tie line):        loads G + H  (two loads → larger demand)
    //   Line 4 (bus1 distribution): loads D + E + F  (three loads → largest demand)
    //   Line 5 (solar feeder):    load B
    //   Line 6 (gas feeder):      load C
    static constexpr int BASE_DEMAND[6] = {0, 8, 15, 20, 8, 8};

    // Per-line transmission capacity (max flow before overload).
    // Line upgrades multiply this — see LINE_CAP_GROWTH below.
    //   Line 1 (coal feeder):     50  — must carry full coal export
    //   Line 2 (wind feeder):     40  — must carry full wind export
    //   Line 3 (tie line):        50  — carries inter-bus transfers + loadGH
    //   Line 4 (bus1 distribution): 30  — carries loadDEF
    //   Line 5 (solar feeder):    40
    //   Line 6 (gas feeder):      40
    static constexpr int BASE_CAPACITY[6] = {50, 40, 50, 30, 40, 40};

    // Each line upgrade level adds this fraction to capacity.
    // Level 0 = 1.0x, Level 3 = 2.5x.
    static constexpr double LINE_CAP_GROWTH = 0.5;

    // Score penalty per overloaded line per second tick.
    static constexpr int OVERLOAD_PENALTY = 3;

    // Per-tick power-flow state (recomputed in evaluateGrid)
    int  m_lineFlow[6]       = {0, 0, 0, 0, 0, 0};
    int  m_lineCapacity[6]   = {0, 0, 0, 0, 0, 0};
    bool m_lineOverloaded[6] = {false, false, false, false, false, false};

    // Tick / cadence
    static constexpr int TICK_INTERVAL_MS    = 10;     // this also messes with the 7-segs on the NXP... This is a
                                                        // mis-judgement of the PAIN that DMA is on that bard poor or WRONG docs


    static constexpr int TICKS_PER_SIM_YEAR  = 150;   // 15 s real per sim year (slower)
    static constexpr int MCU_STATUS_INTERVAL = 10;    // push year/score every 1 s
    static constexpr int DISPLAY_REFRESH_DIV = 5;     // cNx every 500 ms (~2 Hz)
    static constexpr int STARTUP_BURST_COUNT = 6;     // blank cmds to wake display

    // Generator upgrades
    static constexpr int    GEN_MAX_LEVEL    = 3;
    static constexpr double GEN_MULTIPLIER   = 0.30;  // +30% per level
    static constexpr double GEN_COST_GROWTH  = 1.7;   // cost ×1.7 per level

    // Game balance
    static constexpr double YEAR_GROWTH_RATE = 0.03;  // 3% demand growth / sim year
};

#endif
