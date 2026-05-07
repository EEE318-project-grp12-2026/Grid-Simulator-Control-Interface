#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSerialPortInfo>
#include <QMessageBox>
#include <QDebug>
#include <QRadioButton>
#include <QPushButton>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

#define STAT_NOTCON 0
#define STAT_CON    1
#define STAT_ERR    2

int __SER_STAT = STAT_NOTCON;

// Out-of-class definitions for static constexpr arrays
constexpr int MainWindow::BASE_DEMAND[6];
constexpr int MainWindow::BASE_CAPACITY[6];

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_serial(new SerialHandler(this))
    , m_scoreTimer(new QTimer(this))
    , m_tickTimer(new QTimer(this))
{
    ui->setupUi(this);

    connect(ui->btnStartgame, &QPushButton::clicked,
            this, &MainWindow::on_btnStartGame_clicked);

    // System tick — drives grid simulation, display refresh, and MCU updates
    connect(m_tickTimer, &QTimer::timeout, this, &MainWindow::onSysTick);
    m_tickTimer->start(TICK_INTERVAL_MS);

    QStringList valueLabels = {
        "lbl_sunUse",
        "lbl_windUse",
        "lbl_coalUse",
        "lbl_coalUse_2",
        "lbl_nuclearUse",
        "lbl_gasUse",
        "lbl_gasUse_2"
    };

    for (const QString &name : valueLabels) {
        QLabel *label = findChild<QLabel *>(name);
        if (label != nullptr) {
            label->setAttribute(Qt::WA_TransparentForMouseEvents);
        }
    }

    removeWrongDesignerConnections();

    for (int i = 0; i < 6; i++) {
        lineLevel[i] = 0;
        lineSwitchOn[i] = true;
    }

    lineCost[0] = 4500;
    lineCost[1] = 5400;
    lineCost[2] = 6600;
    lineCost[3] = 7800;
    lineCost[4] = 9600;
    lineCost[5] = 12000;

    initialiseUiToZero();
    initialiseTransmissionTable();

    // Build the new Generators upgrade tab programmatically
    buildGeneratorTab();

    connect(m_serial, &SerialHandler::disconnected,
            this, &MainWindow::onDisconnected);

    connect(m_serial, &SerialHandler::connectionError,
            this, &MainWindow::onConnectionError);

    connect(m_scoreTimer, &QTimer::timeout,
            this, &MainWindow::updateScoreOverTime);

    ui->lbl_line1status->setText("Status: Level 0");
    ui->lbl_line2status->setText("Status: Level 0");
    ui->lbl_line3status->setText("Status: Level 0");
    ui->lbl_line4status->setText("Status: Level 0");
    ui->lbl_line5status->setText("Status: Level 0");
    ui->lbl_line6status->setText("Status: Level 0");

    ui->lbl_line1cost->setText("Upgrade Cost: -450 score");
    ui->lbl_line2cost->setText("Upgrade Cost: -540 score");
    ui->lbl_line3cost->setText("Upgrade Cost: -660 score");
    ui->lbl_line4cost->setText("Upgrade Cost: -780 score");
    ui->lbl_line5cost->setText("Upgrade Cost: -960 score");
    ui->lbl_line6cost->setText("Upgrade Cost: -1200 score");

    ui->radioButton->setChecked(true);
    updateSummaryDisplays();
    displayCurrentScore();
    displayYearOnGui(m_simYear);

    setGameControlsEnabled(false);
    m_scoreTimer->stop();

    ui->btnStartgame->setEnabled(true);

    addEvent("Select player count and press Start Game.");

    connect(ui->radioButton, &QRadioButton::clicked, this, [this]() {
        addEvent("Now showing: Demand % supply.");
        updateSummaryDisplays();
    });

    connect(ui->radioButton_2, &QRadioButton::clicked, this, [this]() {
        addEvent("Now showing: Cost per Unit Energy.");
        updateSummaryDisplays();
    });

    connect(ui->radioButton_3, &QRadioButton::clicked, this, [this]() {
        addEvent("Now showing: Power Generated.");
        updateSummaryDisplays();
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

QLCDNumber *MainWindow::findLcd(const QStringList &names) const
{
    for (const QString &name : names) {
        QLCDNumber *lcd = findChild<QLCDNumber *>(name);
        if (lcd != nullptr) {
            return lcd;
        }
    }

    return nullptr;
}

void MainWindow::displayLcd(const QStringList &names, int value)
{
    QLCDNumber *lcd = findLcd(names);

    if (lcd != nullptr) {
        lcd->display(value);
    }
}

/* ------------------------------------------------------------------ */
/*  Generator upgrade tab — built programmatically                    */
/* ------------------------------------------------------------------ */

QString MainWindow::genName(int genIndex)
{
    switch (genIndex) {
    case GEN_SOLAR: return "Solar";
    case GEN_WIND:  return "Wind";
    case GEN_COAL:  return "Coal";
    case GEN_GAS:   return "Gas";
    }
    return "?";
}

void MainWindow::buildGeneratorTab()
{
    QWidget *tab = new QWidget;
    QVBoxLayout *outer = new QVBoxLayout(tab);
    outer->setContentsMargins(16, 16, 16, 16);
    outer->setSpacing(12);

    QLabel *header = new QLabel("Power Plant Upgrades");
    QFont hf = header->font();
    hf.setPointSize(hf.pointSize() + 4);
    hf.setBold(true);
    header->setFont(hf);
    outer->addWidget(header);

    QLabel *intro = new QLabel(
        "Upgrade your generators to boost output. Solar and Wind upgrades "
        "are pure efficiency gains — no extra fuel needed. Coal and Gas "
        "upgrades expand plant capacity but remain bound by fuel availability.");
    intro->setWordWrap(true);
    outer->addWidget(intro);

    for (int i = 0; i < 4; i++) {
        QFrame *frame = new QFrame;
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);

        QVBoxLayout *fLayout = new QVBoxLayout(frame);
        fLayout->setSpacing(6);

        // Top row: name + level badge
        QHBoxLayout *topRow = new QHBoxLayout;
        QLabel *nameLbl = new QLabel(QString("<b>%1 Generator</b>").arg(genName(i)));
        QFont nf = nameLbl->font();
        nf.setPointSize(nf.pointSize() + 2);
        nameLbl->setFont(nf);

        QLabel *levelLbl = new QLabel(QString("Level 0 / %1").arg(GEN_MAX_LEVEL));
        levelLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        topRow->addWidget(nameLbl);
        topRow->addStretch();
        topRow->addWidget(levelLbl);
        fLayout->addLayout(topRow);

        // Description row
        QString descText;
        if (i == GEN_SOLAR || i == GEN_WIND) {
            descText = QString("Each level: +%1% energy from same resource — "
                               "renewable, no extra fuel cost.")
                           .arg(int(GEN_MULTIPLIER * 100));
        } else {
            descText = QString("Each level: +%1% plant capacity — bigger output, "
                               "but constrained by available fuel.")
                           .arg(int(GEN_MULTIPLIER * 100));
        }
        QLabel *descLbl = new QLabel(descText);
        descLbl->setWordWrap(true);
        fLayout->addWidget(descLbl);

        // Action row: upgrade button
        QHBoxLayout *btnRow = new QHBoxLayout;
        QPushButton *btn = new QPushButton(
            QString("Upgrade %1 (-%2 score)").arg(genName(i)).arg(m_genCost[i]));
        btn->setProperty("genIndex", i);
        btn->setEnabled(false);   // off until game starts
        btn->setMinimumHeight(32);

        btnRow->addStretch();
        btnRow->addWidget(btn);
        fLayout->addLayout(btnRow);

        outer->addWidget(frame);

        // Stash pointers for runtime updates
        m_genLevelLbl[i]   = levelLbl;
        m_genDescLbl[i]    = descLbl;
        m_genUpgradeBtn[i] = btn;

        connect(btn, &QPushButton::clicked,
                this, &MainWindow::onUpgradeGeneratorClicked);
    }

    outer->addStretch();

    ui->tabWidget->addTab(tab, "Generators");
}

void MainWindow::refreshGeneratorTab()
{
    for (int i = 0; i < 4; i++) {
        if (m_genLevelLbl[i]) {
            m_genLevelLbl[i]->setText(
                QString("Level %1 / %2").arg(m_genLevel[i]).arg(GEN_MAX_LEVEL));
        }

        if (m_genUpgradeBtn[i]) {
            bool atMax = m_genLevel[i] >= GEN_MAX_LEVEL;
            if (atMax) {
                m_genUpgradeBtn[i]->setText(
                    QString("%1 — Fully Upgraded").arg(genName(i)));
                m_genUpgradeBtn[i]->setEnabled(false);
            } else {
                m_genUpgradeBtn[i]->setText(
                    QString("Upgrade %1 (-%2 score)")
                        .arg(genName(i)).arg(m_genCost[i]));
                m_genUpgradeBtn[i]->setEnabled(gameRunning);
            }
        }
    }
}

void MainWindow::onUpgradeGeneratorClicked()
{
    QPushButton *btn = qobject_cast<QPushButton *>(sender());
    if (!btn) return;

    int idx = btn->property("genIndex").toInt();
    upgradeGenerator(idx);
}

void MainWindow::upgradeGenerator(int genIndex)
{
    if (!gameRunning) {
        addEvent("Start the game before upgrading generators.");
        return;
    }
    if (genIndex < 0 || genIndex >= 4) return;

    if (m_genLevel[genIndex] >= GEN_MAX_LEVEL) {
        addEvent(QString("%1 generator is already fully upgraded.")
                     .arg(genName(genIndex)));
        return;
    }

    int cost = m_genCost[genIndex];
    if (currentScore < cost) {
        addEvent(QString("Cannot afford %1 upgrade. Need %2 score, have %3.")
                     .arg(genName(genIndex)).arg(cost).arg(currentScore));
        return;
    }

    currentScore -= cost;
    if (currentScore < 0) currentScore = 0;
    displayCurrentScore();

    m_genLevel[genIndex]++;
    m_genCost[genIndex] = static_cast<int>(m_genCost[genIndex] * GEN_COST_GROWTH);

    refreshGeneratorTab();

    addEvent(QString("%1 generator upgraded to Level %2 (-%3 score). New multiplier: ×%4")
                 .arg(genName(genIndex))
                 .arg(m_genLevel[genIndex])
                 .arg(cost)
                 .arg(QString::number(genMultiplier(genIndex), 'f', 2)));

    if (m_genLevel[genIndex] >= GEN_MAX_LEVEL) {
        addEvent(QString("%1 generator is now fully upgraded.")
                     .arg(genName(genIndex)));
    }
}

double MainWindow::genMultiplier(int genIndex) const
{
    if (genIndex < 0 || genIndex >= 4) return 1.0;
    return 1.0 + GEN_MULTIPLIER * m_genLevel[genIndex];
}

/* ------------------------------------------------------------------ */
/*  Display year on the Qt GUI (mirrors what the MCU shows)           */
/* ------------------------------------------------------------------ */
void MainWindow::displayYearOnGui(int year)
{
    displayLcd({"lcdNumber_13", "lcd_year", "lcd_Year", "lcdYear",
                "lcd_simYear", "lcd_currentYear"}, year);

    QStringList lblNames = {"lbl_year", "lbl_Year", "lbl_simYear",
                            "lbl_currentYear", "lbl_round"};
    for (const QString &name : lblNames) {
        QLabel *lbl = findChild<QLabel *>(name);
        if (lbl != nullptr) {
            lbl->setText(QString::number(year));
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Display refresh — keeps MCU 7-segment segments lit                */
/* ------------------------------------------------------------------ */
void MainWindow::sendDisplayRefresh()
{
    if (!m_isConnected) return;
    m_serial->queueCommand("cNx");
}

void MainWindow::sendDisplayStartupBurst()
{
    if (!m_isConnected) return;
    for (int i = 0; i < STARTUP_BURST_COUNT; i++) {
        m_serial->queueCommand("cNx");
    }
}

/* ------------------------------------------------------------------ */
/*  System tick                                                       */
/* ------------------------------------------------------------------ */
void MainWindow::onSysTick()
{
    // Refresh the MCU display even before the game starts
    if (m_tickCount % DISPLAY_REFRESH_DIV == 0) {
        sendDisplayRefresh();
    }

    if (!gameRunning) {
        m_tickCount++;
        return;
    }

    m_tickCount++;

    // Advance simulation year periodically
    if (m_tickCount % TICKS_PER_SIM_YEAR == 0) {
        m_simYear++;
        addEvent(QString("Year advanced to %1. Demand has grown.").arg(m_simYear));
        displayYearOnGui(m_simYear);
    }

    evaluateGrid();

    if (m_isConnected) {
        pushMcuState();
    }
}

/* ------------------------------------------------------------------ */
/*  Grid power-flow evaluation                                        */
/*                                                                    */
/*  Topology (per physical board):                                    */
/*    wind  → line2(loadA) <SW1> busbar1                              */
/*    coal  → line1        <SW4> busbar1                              */
/*    busbar1 <SW3> line4(loads D,E,F)                                */
/*    busbar1 <SW2> line3(loads G,H) <SW6> busbar2  [tie line]        */
/*    solar → line5(loadB) <SW5> busbar2                              */
/*    gas   → line6(loadC) <SW7> busbar2                              */
/*                                                                    */
/*  Each feeder line has its load between the generator and the       */
/*  switch, so the load is always tied to the generator regardless    */
/*  of switch state. If the switch is open, the line is islanded and  */
/*  must satisfy its load from local generation alone.                */
/* ------------------------------------------------------------------ */
void MainWindow::evaluateGrid()
{
    // ── 1. Generator outputs (with upgrade multipliers) ──
    double mSolar = genMultiplier(GEN_SOLAR);
    double mWind  = genMultiplier(GEN_WIND);
    double mCoal  = genMultiplier(GEN_COAL);
    double mGas   = genMultiplier(GEN_GAS);

    int solarOut = qRound(qMin(sunUse,  solarPower) * mSolar);
    int windOut  = qRound(qMin(windUse, windPower)  * mWind);
    int coalOut  = qMin(qRound(coalUse * mCoal), coalPower);
    int gasOut   = qMin(qRound(gasUse  * mGas),  gasPower);

    // ── 2. Per-line loads (year scaled). Line upgrades no longer affect
    //       demand; they now affect transmission capacity instead. ──
    double yearFactor = 1.0 + YEAR_GROWTH_RATE * (m_simYear - 2024);
    int load[6];
    for (int i = 0; i < 6; i++) {
        load[i] = qRound(BASE_DEMAND[i] * yearFactor);
        if (BASE_DEMAND[i] == 0) load[i] = 0;   // line 1 stays loadless
    }

    // ── 3. Per-line capacity (grows with line upgrade level) ──
    int capacity[6];
    for (int i = 0; i < 6; i++) {
        capacity[i] = qRound(BASE_CAPACITY[i] * (1.0 + LINE_CAP_GROWTH * lineLevel[i]));
        m_lineCapacity[i] = capacity[i];
    }

    // ── 4. Switch shorthand ──
    bool sw1 = m_switchOn[0]; // line 2 ↔ bus1
    bool sw2 = m_switchOn[1]; // line 3 ↔ bus1
    bool sw3 = m_switchOn[2]; // line 4 ↔ bus1
    bool sw4 = m_switchOn[3]; // line 1 ↔ bus1
    bool sw5 = m_switchOn[4]; // line 5 ↔ bus2
    bool sw6 = m_switchOn[5]; // line 3 ↔ bus2
    bool sw7 = m_switchOn[6]; // line 6 ↔ bus2

    // ── 5. Aggregate supply/demand on each bus ──
    int bus1Supply = 0, bus1Demand = 0;
    int bus2Supply = 0, bus2Demand = 0;

    if (sw4) bus1Supply += coalOut;
    if (sw1) { bus1Supply += windOut; bus1Demand += load[1]; }
    if (sw3) bus1Demand += load[3];

    if (sw5) { bus2Supply += solarOut; bus2Demand += load[4]; }
    if (sw7) { bus2Supply += gasOut;   bus2Demand += load[5]; }

    bool tied = sw2 && sw6;
    if (sw2 && !sw6) bus1Demand += load[2];
    if (sw6 && !sw2) bus2Demand += load[2];

    // ── 6. Bus health ──
    bool bus1Healthy, bus2Healthy;
    int  bus1Headroom, bus2Headroom;

    if (tied) {
        int pool   = bus1Supply + bus2Supply;
        int total  = bus1Demand + bus2Demand + load[2];
        bool ok    = (pool >= total);
        bus1Healthy = bus2Healthy = ok;
        bus1Headroom = bus2Headroom = pool - total;
    } else {
        bus1Healthy  = (bus1Supply >= bus1Demand);
        bus2Healthy  = (bus2Supply >= bus2Demand);
        bus1Headroom = bus1Supply - bus1Demand;
        bus2Headroom = bus2Supply - bus2Demand;
    }

    // ── 7. Per-line supplied & flow ──
    int demanded[6] = {0, 0, 0, 0, 0, 0};
    int supplied[6] = {0, 0, 0, 0, 0, 0};
    int flow[6]     = {0, 0, 0, 0, 0, 0};

    // Line 1: coal feeder, no load. Flow = coalOut crossing SW4 to bus1.
    demanded[0] = 0;
    supplied[0] = sw4 ? coalOut : 0;
    flow[0]     = sw4 ? coalOut : 0;

    // Line 2: wind + loadA. Flow at most-loaded segment depends on direction.
    //   Closed: gen-side carries windOut, bus-side carries |windOut − loadA|.
    //   Open:   line is islanded — only carries what's served to loadA.
    demanded[1] = load[1];
    if (sw1) {
        supplied[1] = bus1Healthy ? load[1]
                                  : qMin(load[1], windOut + qMax(0, bus1Headroom));
        flow[1] = qMax(windOut, qAbs(windOut - load[1]));
    } else {
        supplied[1] = qMin(windOut, load[1]);
        flow[1]     = supplied[1];
    }

    // Line 3: tie + loadGH. Flow = loadGH plus magnitude of bus-to-bus transfer.
    demanded[2] = load[2];
    if (tied) {
        supplied[2] = bus1Healthy ? load[2] : 0;
        // Net imbalance between the two buses (excluding line 3) divided by 2
        // approximates how much power crosses the tie to balance them.
        int net1 = bus1Supply - bus1Demand;
        int net2 = bus2Supply - bus2Demand;
        int transfer = qAbs(net1 - net2) / 2;
        flow[2] = load[2] + transfer;
    } else if (sw2) {
        supplied[2] = bus1Healthy ? load[2] : 0;
        flow[2]     = supplied[2];
    } else if (sw6) {
        supplied[2] = bus2Healthy ? load[2] : 0;
        flow[2]     = supplied[2];
    } else {
        supplied[2] = 0;
        flow[2]     = 0;
    }

    // Line 4: distribution to D/E/F. Flow = loadDEF being delivered.
    demanded[3] = load[3];
    supplied[3] = (sw3 && bus1Healthy) ? load[3] : 0;
    flow[3]     = supplied[3];

    // Line 5: solar + loadB. Same form as line 2.
    demanded[4] = load[4];
    if (sw5) {
        supplied[4] = bus2Healthy ? load[4]
                                  : qMin(load[4], solarOut + qMax(0, bus2Headroom));
        flow[4] = qMax(solarOut, qAbs(solarOut - load[4]));
    } else {
        supplied[4] = qMin(solarOut, load[4]);
        flow[4]     = supplied[4];
    }

    // Line 6: gas + loadC. Same form as line 2.
    demanded[5] = load[5];
    if (sw7) {
        supplied[5] = bus2Healthy ? load[5]
                                  : qMin(load[5], gasOut + qMax(0, bus2Headroom));
        flow[5] = qMax(gasOut, qAbs(gasOut - load[5]));
    } else {
        supplied[5] = qMin(gasOut, load[5]);
        flow[5]     = supplied[5];
    }

    // ── 8. Detect overloads & push to transmission table ──
    for (int i = 0; i < 6; i++) {
        bool wasOverloaded = m_lineOverloaded[i];
        m_lineOverloaded[i] = (flow[i] > capacity[i]);
        m_lineFlow[i] = flow[i];

        // Log a one-shot event when a line transitions into overload
        if (m_lineOverloaded[i] && !wasOverloaded && gameRunning) {
            addEvent(QString("⚠ Line %1 OVERLOAD — carrying %2, capacity %3.")
                         .arg(i + 1).arg(flow[i]).arg(capacity[i]));
        }

        updateTransmissionLine(i + 1, demanded[i], supplied[i],
                               flow[i], capacity[i], m_lineOverloaded[i]);
    }
}

/* ------------------------------------------------------------------ */
/*  Push deltas to the MCU                                            */
/* ------------------------------------------------------------------ */
void MainWindow::pushMcuState()
{
    for (int i = 0; i < 6; i++) {
        int demand = ui->table_transmissionLines->item(i, 1)->text().remove("%").toInt();
        int supplied = ui->table_transmissionLines->item(i, 2)->text().remove("%").toInt();

        // LED on when:
        //   - line has a load and it's fully supplied, OR
        //   - line is loadless (e.g. line 1) but carrying power
        bool ledOn = lineSwitchOn[i]
                     && ((demand > 0 && supplied >= demand)
                         || (demand == 0 && supplied > 0));

        if (ledOn != m_lastLedState[i]) {
            QString cmd = QString("cL%1%2x").arg(i + 1).arg(ledOn ? 1 : 0);
            m_serial->queueCommand(cmd);
            m_lastLedState[i] = ledOn;
        }
    }

    if (m_tickCount % MCU_STATUS_INTERVAL == 0) {
        if (m_simYear != m_lastYearSent) {
            m_serial->queueCommand(QString("cY%1x").arg(m_simYear));
            m_lastYearSent = m_simYear;
        }
        if (currentScore != m_lastScoreSent) {
            m_serial->queueCommand(QString("cS%1x").arg(currentScore));
            m_lastScoreSent = currentScore;
        }
    }
}

QLCDNumber *MainWindow::scoreDisplay() const
{
    return findLcd({"lcd_score", "lcdScore", "lcd_Score"});
}

void MainWindow::removeWrongDesignerConnections()
{
    QList<QSlider*> sliders = {
        ui->horizontalSlider,
        ui->horizontalSlider_2,
        ui->horizontalSlider_3,
        ui->horizontalSlider_4
    };

    QList<QLCDNumber*> allLcds = findChildren<QLCDNumber*>();

    for (QSlider *slider : sliders) {
        for (QLCDNumber *lcd : allLcds) {
            QObject::disconnect(slider, nullptr, lcd, nullptr);
        }
    }
}

void MainWindow::initialiseUiToZero()
{
    ui->horizontalSlider->setRange(0, 0);
    ui->horizontalSlider_2->setRange(0, 0);
    ui->horizontalSlider_3->setRange(0, 0);
    ui->horizontalSlider_4->setRange(0, 0);

    ui->horizontalSlider->setValue(0);
    ui->horizontalSlider_2->setValue(0);
    ui->horizontalSlider_3->setValue(0);
    ui->horizontalSlider_4->setValue(0);

    displayLcd({"lcdNumber_5"}, 0);
    displayLcd({"lcdNumber_6"}, 0);
    displayLcd({"lcdNumber_7"}, 0);
    displayLcd({"IcdNumber_8"}, 0);

    displayLcd({"IcdNumber_9"}, 0);
    displayLcd({"IcdNumber_10"}, 0);

    displayLcd({"lcd_total"}, 0);
    displayLcd({"lcd_solar"}, 0);
    displayLcd({"lcd_wind"}, 0);
    displayLcd({"lcd_coal", "lcd_Coal"}, 0);
    displayLcd({"lcd_gas", "lcd_Gas", "Icd_Nuclear"}, 0);

    setUseLabels({"lbl_sunUse"}, 0);
    setUseLabels({"lbl_windUse"}, 0);
    setUseLabels({"lbl_coalUse", "lbl_coalUse_2"}, 0);
    setUseLabels({"lbl_gasUse", "lbl_gasUse_2", "lbl_nuclearUse"}, 0);

    sunUse = 0;
    windUse = 0;
    coalUse = 0;
    gasUse = 0;

    solarPower = 0;
    windPower = 0;
    coalPower = 0;
    gasPower = 0;

    solarCost = 0;
    windCost = 0;
    coalCost = 0;
    gasCost = 0;

    currentScore = 1000;
    totalDemand = 0;
    totalSupplied = 0;

    displayCurrentScore();
}

void MainWindow::initialiseTransmissionTable()
{
    ui->table_transmissionLines->setRowCount(6);
    ui->table_transmissionLines->setColumnCount(5);

    ui->table_transmissionLines->setHorizontalHeaderLabels(
        {"Line", "Demand (%)", "Supplied (%)", "Flow / Cap", "Status"}
        );

    ui->table_transmissionLines->horizontalHeader()->setVisible(true);
    ui->table_transmissionLines->verticalHeader()->setVisible(false);

    ui->table_transmissionLines->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->table_transmissionLines->setSelectionMode(QAbstractItemView::NoSelection);
    ui->table_transmissionLines->setFocusPolicy(Qt::NoFocus);

    ui->table_transmissionLines->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->table_transmissionLines->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    ui->table_transmissionLines->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->table_transmissionLines->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    for (int row = 0; row < 6; row++) {
        QTableWidgetItem *lineItem     = new QTableWidgetItem(QString("Line %1").arg(row + 1));
        QTableWidgetItem *demandItem   = new QTableWidgetItem("0%");
        QTableWidgetItem *suppliedItem = new QTableWidgetItem("0%");
        QTableWidgetItem *capItem      = new QTableWidgetItem(
            QString("0 / %1").arg(BASE_CAPACITY[row]));
        QTableWidgetItem *statusItem   = new QTableWidgetItem("Idle");

        lineItem->setTextAlignment(Qt::AlignCenter);
        demandItem->setTextAlignment(Qt::AlignCenter);
        suppliedItem->setTextAlignment(Qt::AlignCenter);
        capItem->setTextAlignment(Qt::AlignCenter);
        statusItem->setTextAlignment(Qt::AlignCenter);

        statusItem->setForeground(QBrush(QColor("#aaaaaa")));

        ui->table_transmissionLines->setItem(row, 0, lineItem);
        ui->table_transmissionLines->setItem(row, 1, demandItem);
        ui->table_transmissionLines->setItem(row, 2, suppliedItem);
        ui->table_transmissionLines->setItem(row, 3, capItem);
        ui->table_transmissionLines->setItem(row, 4, statusItem);
    }
}

void MainWindow::updateTransmissionLine(int line, int demand, int supplied,
                                        int flow, int capacity, bool overloaded)
{
    int row = line - 1;

    if (row < 0 || row >= 6) {
        return;
    }

    demand   = qBound(0, demand,   100);
    supplied = qBound(0, supplied, 100);

    QString status;
    QColor  statusColour;

    if (!lineSwitchOn[row]) {
        supplied = 0;
        flow = 0;
        status = "Disconnected";
        statusColour = QColor("#ef4444");
    }
    else if (overloaded) {
        // Overload trumps everything else once the line is connected
        status = "Overload";
        statusColour = QColor("#f97316");   // strong orange
    }
    else if (demand == 0 && supplied == 0) {
        status = "Idle";
        statusColour = QColor("#aaaaaa");
    }
    else if (demand == 0 && supplied > 0) {
        status = "Carrying";
        statusColour = QColor("#4ade80");
    }
    else if (supplied >= demand) {
        status = "Fully supplied";
        statusColour = QColor("#4ade80");
    }
    else if (supplied > 0) {
        status = "Partially supplied";
        statusColour = QColor("#f59e0b");
    }
    else {
        status = "Blackout";
        statusColour = QColor("#ef4444");
    }

    ui->table_transmissionLines->item(row, 1)->setText(QString::number(demand) + "%");
    ui->table_transmissionLines->item(row, 2)->setText(QString::number(supplied) + "%");
    ui->table_transmissionLines->item(row, 3)->setText(
        QString("%1 / %2").arg(flow).arg(capacity));

    // Highlight the flow cell red if overloaded
    QColor flowColour = overloaded ? QColor("#f97316") : QColor("#dddddd");
    ui->table_transmissionLines->item(row, 3)->setForeground(QBrush(flowColour));

    ui->table_transmissionLines->item(row, 4)->setText(status);
    ui->table_transmissionLines->item(row, 4)->setForeground(QBrush(statusColour));
}

void MainWindow::setUseLabels(const QStringList &labelNames, int value)
{
    for (const QString &name : labelNames) {
        QLabel *label = findChild<QLabel *>(name);
        if (label != nullptr) {
            label->setText(QString::number(value));
        }
    }
}

void MainWindow::setResourceMaximum(QSlider *slider,
                                    QLCDNumber *maximumBox,
                                    int maximumValue)
{
    maximumValue = qBound(0, maximumValue, 100);

    maximumBox->display(maximumValue);

    slider->blockSignals(true);
    slider->setRange(0, maximumValue);

    if (slider->value() > maximumValue) {
        slider->setValue(maximumValue);
    }

    slider->blockSignals(false);
}

void MainWindow::updateSummaryDisplays()
{
    int solar = 0;
    int wind = 0;
    int coal = 0;
    int gas = 0;

    if (ui->radioButton->isChecked()) {
        solar = sunUse;
        wind = windUse;
        coal = coalUse;
        gas = gasUse;
    }
    else if (ui->radioButton_2->isChecked()) {
        solar = solarCost;
        wind = windCost;
        coal = coalCost;
        gas = gasCost;
    }
    else if (ui->radioButton_3->isChecked()) {
        // Show effective output (with multipliers) in this view
        solar = qRound(qMin(sunUse, solarPower)  * genMultiplier(GEN_SOLAR));
        wind  = qRound(qMin(windUse, windPower)  * genMultiplier(GEN_WIND));
        coal  = qMin(qRound(coalUse * genMultiplier(GEN_COAL)), coalPower);
        gas   = qMin(qRound(gasUse  * genMultiplier(GEN_GAS)),  gasPower);
    }

    displayLcd({"lcd_solar"}, solar);
    displayLcd({"lcd_wind"}, wind);
    displayLcd({"lcd_coal", "lcd_Coal"}, coal);
    displayLcd({"lcd_gas", "lcd_Gas", "Icd_Nuclear"}, gas);
    displayLcd({"lcd_total"}, solar + wind + coal + gas);
}

int MainWindow::calculateScoreIncrement()
{
    totalDemand = 0;
    totalSupplied = 0;

    for (int i = 0; i < 6; i++) {
        int demand =
            ui->table_transmissionLines->item(i, 1)
                ->text().remove("%").toInt();

        int supplied =
            ui->table_transmissionLines->item(i, 2)
                ->text().remove("%").toInt();

        if (!lineSwitchOn[i]) {
            supplied = 0;
        }

        totalDemand += demand;
        totalSupplied += supplied;
    }

    int demandPercent = 0;

    if (totalDemand > 0) {
        demandPercent = (totalSupplied * 100) / totalDemand;
    }

    int totalUsed = sunUse + windUse + coalUse + gasUse;
    int renewableUsed = sunUse + windUse;

    int renewablePercent = 0;

    if (totalUsed > 0) {
        renewablePercent = (renewableUsed * 100) / totalUsed;
    }

    int increment = 0;
    increment += demandPercent / 10;
    increment += renewablePercent / 10;

    if (increment < 0) {
        increment = 0;
    }

    return increment;
}

void MainWindow::updateScoreOverTime()
{
    if (!gameRunning) {
        return;
    }

    int increment = calculateScoreIncrement();

    // Tally overload penalty: −OVERLOAD_PENALTY per overloaded line per second
    int overloadCount = 0;
    for (int i = 0; i < 6; i++) {
        if (m_lineOverloaded[i]) overloadCount++;
    }
    int penalty = overloadCount * OVERLOAD_PENALTY;

    int delta = increment - penalty;

    if (delta != 0) {
        currentScore += delta;
        if (currentScore < 0) currentScore = 0;
        displayCurrentScore();
    }

    if (penalty > 0) {
        addEvent(QString("Overload penalty: −%1 score (%2 line%3 over capacity).")
                     .arg(penalty)
                     .arg(overloadCount)
                     .arg(overloadCount == 1 ? "" : "s"));
    }
}

void MainWindow::displayCurrentScore()
{
    QLCDNumber *score = scoreDisplay();

    if (score != nullptr) {
        score->display(currentScore);
    }
}

void MainWindow::addEvent(const QString &message)
{
    ui->listWidget_newsFeed->addItem(message);
    ui->listWidget_newsFeed->scrollToBottom();

    if (ui->listWidget_newsFeed->count() > 200) {
        delete ui->listWidget_newsFeed->takeItem(0);
    }
}

void MainWindow::setGameControlsEnabled(bool enabled)
{
    ui->horizontalSlider->setEnabled(enabled);
    ui->horizontalSlider_2->setEnabled(enabled);
    ui->horizontalSlider_3->setEnabled(enabled);
    ui->horizontalSlider_4->setEnabled(enabled);

    ui->btn_line1Upgrade->setEnabled(enabled);
    ui->btn_line2Upgrade->setEnabled(enabled);
    ui->btn_line3Upgrade->setEnabled(enabled);
    ui->btn_line4Upgrade->setEnabled(enabled);
    ui->btn_line5Upgrade->setEnabled(enabled);
    ui->btn_line6Upgrade->setEnabled(enabled);

    // Generator upgrade buttons follow the same gating
    refreshGeneratorTab();
}

void MainWindow::on_btnStartGame_clicked()
{
    if (gameRunning) {
        addEvent("Game is already running.");
        return;
    }

    if (!ui->radioPlayer1->isChecked() && !ui->radioPlayer2->isChecked()) {
        addEvent("Please select Player 1 or Player 2 before starting.");
        return;
    }

    twoPlayerMode = ui->radioPlayer2->isChecked();
    gameRunning = true;

    currentScore = 1000;
    displayCurrentScore();

    // Reset grid simulation state
    m_simYear = 2024;
    m_tickCount = 0;
    m_lastYearSent = -1;
    m_lastScoreSent = -1;
    for (int i = 0; i < 6; i++) m_lastLedState[i] = false;

    // Reset generator upgrades
    int initialCosts[4] = {600, 600, 750, 750};
    for (int i = 0; i < 4; i++) {
        m_genLevel[i] = 0;
        m_genCost[i]  = initialCosts[i];
    }
    refreshGeneratorTab();

    displayYearOnGui(m_simYear);

    setGameControlsEnabled(true);
    m_scoreTimer->start(1000);

    ui->btnStartgame->setEnabled(false);

    if (twoPlayerMode) {
        addEvent("Game started in 2 Player Mode.");
    } else {
        addEvent("Game started in 1 Player Mode.");
    }

    addEvent(QString("Starting year: %1.").arg(m_simYear));
}

void MainWindow::upgradeLine(int lineIndex)
{
    if (!gameRunning) {
        addEvent("Start the game before upgrading lines.");
        return;
    }

    const int maxLevel = 3;

    if (lineIndex < 0 || lineIndex > 5) {
        return;
    }

    if (lineLevel[lineIndex] >= maxLevel) {
        addEvent(QString("Line %1 is already fully upgraded.").arg(lineIndex + 1));
        return;
    }

    int upgradeScoreCost = lineCost[lineIndex] / 10;

    if (currentScore < upgradeScoreCost) {
        addEvent(QString("Cannot afford Line %1 upgrade. Need %2 score, but only have %3.")
                     .arg(lineIndex + 1)
                     .arg(upgradeScoreCost)
                     .arg(currentScore));
        displayCurrentScore();
        return;
    }

    currentScore -= upgradeScoreCost;

    if (currentScore < 0) {
        currentScore = 0;
    }

    lineLevel[lineIndex]++;

    QString statusText = QString("Status: Level %1").arg(lineLevel[lineIndex]);
    bool reachedMax = lineLevel[lineIndex] >= maxLevel;

    QString costText;

    if (reachedMax) {
        costText = "Upgrade Cost: Fully Upgraded";
    } else {
        lineCost[lineIndex] = static_cast<int>(lineCost[lineIndex] * 1.75);
        int nextUpgradeScoreCost = lineCost[lineIndex] / 10;
        costText = QString("Upgrade Cost: -%1 score").arg(nextUpgradeScoreCost);
    }

    switch (lineIndex) {
    case 0:
        ui->lbl_line1status->setText(statusText);
        ui->lbl_line1cost->setText(costText);
        if (reachedMax) {
            ui->btn_line1Upgrade->setText("Fully Upgraded");
            ui->btn_line1Upgrade->setEnabled(false);
        }
        break;

    case 1:
        ui->lbl_line2status->setText(statusText);
        ui->lbl_line2cost->setText(costText);
        if (reachedMax) {
            ui->btn_line2Upgrade->setText("Fully Upgraded");
            ui->btn_line2Upgrade->setEnabled(false);
        }
        break;

    case 2:
        ui->lbl_line3status->setText(statusText);
        ui->lbl_line3cost->setText(costText);
        if (reachedMax) {
            ui->btn_line3Upgrade->setText("Fully Upgraded");
            ui->btn_line3Upgrade->setEnabled(false);
        }
        break;

    case 3:
        ui->lbl_line4status->setText(statusText);
        ui->lbl_line4cost->setText(costText);
        if (reachedMax) {
            ui->btn_line4Upgrade->setText("Fully Upgraded");
            ui->btn_line4Upgrade->setEnabled(false);
        }
        break;

    case 4:
        ui->lbl_line5status->setText(statusText);
        ui->lbl_line5cost->setText(costText);
        if (reachedMax) {
            ui->btn_line5Upgrade->setText("Fully Upgraded");
            ui->btn_line5Upgrade->setEnabled(false);
        }
        break;

    case 5:
        ui->lbl_line6status->setText(statusText);
        ui->lbl_line6cost->setText(costText);
        if (reachedMax) {
            ui->btn_line6Upgrade->setText("Fully Upgraded");
            ui->btn_line6Upgrade->setEnabled(false);
        }
        break;
    }

    int newCapacity = qRound(BASE_CAPACITY[lineIndex]
                             * (1.0 + LINE_CAP_GROWTH * lineLevel[lineIndex]));

    addEvent(QString("Line %1 upgraded to Level %2 (-%3 score). New capacity: %4.")
                 .arg(lineIndex + 1)
                 .arg(lineLevel[lineIndex])
                 .arg(upgradeScoreCost)
                 .arg(newCapacity));

    if (reachedMax) {
        addEvent(QString("Line %1 is now fully upgraded.").arg(lineIndex + 1));
    }

    displayCurrentScore();
}

void MainWindow::on_btn_line1Upgrade_clicked() { upgradeLine(0); }
void MainWindow::on_btn_line2Upgrade_clicked() { upgradeLine(1); }
void MainWindow::on_btn_line3Upgrade_clicked() { upgradeLine(2); }
void MainWindow::on_btn_line4Upgrade_clicked() { upgradeLine(3); }
void MainWindow::on_btn_line5Upgrade_clicked() { upgradeLine(4); }
void MainWindow::on_btn_line6Upgrade_clicked() { upgradeLine(5); }

void MainWindow::on_btn_ScanPorts_clicked()
{
    bool portsAvailable = false;
    ui->combo_SerPorts->clear();

    QString errMsg;

#ifdef Q_OS_WIN
    errMsg = "Check that the device is plugged in.";

    const auto ports = QSerialPortInfo::availablePorts();

    for (const QSerialPortInfo &port : ports) {
        if (port.portName().contains("COM")) {
            ui->combo_SerPorts->addItem(port.portName());
            portsAvailable = true;
        }
    }
#else
    errMsg = "Check that the device is connected and check permissions.";

    const auto ports = QSerialPortInfo::availablePorts();

    for (const QSerialPortInfo &port : ports) {
        if (port.portName().contains("USB") || port.portName().contains("ACM")) {
            ui->combo_SerPorts->addItem(port.portName());
            portsAvailable = true;
        }
    }
#endif

    if (!portsAvailable) {
        QMessageBox::warning(this, "No Devices Found", errMsg, QMessageBox::Ok);
    }
}

void MainWindow::on_combo_SerPorts_currentTextChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
}

void MainWindow::on_btn_line4disconn_clicked()
{
    if (!gameRunning) {
        addEvent("Start the game before disconnecting busbars.");
        return;
    }

    addEvent("Disconnected BusBar B.");
}

void MainWindow::on_pushButton_clicked()
{
    if (!gameRunning) {
        addEvent("Start the game before sending commands.");
        return;
    }

    if (m_isConnected) {
        addEvent("Command sent to NXP.");
    } else {
        addEvent("Cannot send command. Serial is not connected.");
    }
}

/* ------------------------------------------------------------------ */
/*  Serial RX                                                         */
/* ------------------------------------------------------------------ */
void MainWindow::onSerialDataSignalRecieved(const QString &text)
{
    QString cleanedText = text.trimmed();

    if (cleanedText.isEmpty()) {
        return;
    }

    cleanedText.replace(":", "=");

    QStringList parts = cleanedText.split(",", Qt::SkipEmptyParts);

    for (const QString &part : parts) {
        QStringList pair = part.split("=", Qt::SkipEmptyParts);

        if (pair.size() != 2) {
            continue;
        }

        QString name = pair[0].trimmed().toUpper();
        int value = pair[1].trimmed().toInt();

        if (name == "SUN" || name == "SOLAR") {
            solarPower = value;
            setResourceMaximum(ui->horizontalSlider, ui->lcdNumber_5, value);
        }
        else if (name == "WIND") {
            windPower = value;
            setResourceMaximum(ui->horizontalSlider_2, ui->lcdNumber_6, value);
        }
        else if (name == "COAL") {
            coalPower = value;
            setResourceMaximum(ui->horizontalSlider_3, ui->lcdNumber_7, value);
        }
        else if (name == "GAS" || name == "NUCLEAR") {
            gasPower = value;
            setResourceMaximum(ui->horizontalSlider_4, ui->IcdNumber_8, value);
        }
        else if (name == "SOLAR_COST") {
            solarCost = value;
        }
        else if (name == "WIND_COST") {
            windCost = value;
        }
        else if (name == "COAL_COST" || name == "COAL_PRICE") {
            coalCost = value;
            displayLcd({"IcdNumber_9"}, value);
        }
        else if (name == "GAS_COST" || name == "GAS_PRICE" || name == "NUCLEAR_COST") {
            gasCost = value;
            displayLcd({"IcdNumber_10"}, value);
        }
        else if (name == "SW1" || name == "SW2" || name == "SW3" ||
                 name == "SW4" || name == "SW5" || name == "SW6" ||
                 name == "SW7") {

            int swNum = name.right(1).toInt();           // 1..7
            int swIdx = swNum - 1;                       // 0..6
            bool wasOn = m_switchOn[swIdx];
            m_switchOn[swIdx] = (value == 1);

            // Recompute derived per-line "is line connected to anything?"
            //   line 1 ↔ SW4   line 2 ↔ SW1   line 3 ↔ SW2 OR SW6
            //   line 4 ↔ SW3   line 5 ↔ SW5   line 6 ↔ SW7
            lineSwitchOn[0] = m_switchOn[3];
            lineSwitchOn[1] = m_switchOn[0];
            lineSwitchOn[2] = m_switchOn[1] || m_switchOn[5];
            lineSwitchOn[3] = m_switchOn[2];
            lineSwitchOn[4] = m_switchOn[4];
            lineSwitchOn[5] = m_switchOn[6];

            // Note: table cells will refresh on the next system tick
            // (≤100 ms) when evaluateGrid() recomputes flow + capacity.

            // Friendly event log entry for which physical line was affected
            if (wasOn != m_switchOn[swIdx]) {
                QString role;
                switch (swNum) {
                case 1: role = "SW1 — line 2 (Wind feeder)";          break;
                case 2: role = "SW2 — line 3 bus1-side (tie line)";   break;
                case 3: role = "SW3 — line 4 (D/E/F distribution)";   break;
                case 4: role = "SW4 — line 1 (Coal feeder)";          break;
                case 5: role = "SW5 — line 5 (Solar feeder)";         break;
                case 6: role = "SW6 — line 3 bus2-side (tie line)";   break;
                case 7: role = "SW7 — line 6 (Gas feeder)";           break;
                }
                addEvent(QString("%1 %2.")
                             .arg(role)
                             .arg(m_switchOn[swIdx] ? "CLOSED" : "OPEN"));
            }
        }
        else if (name == "YEAR" || name == "SIM_YEAR" || name == "SIMYEAR") {
            if (value > 1900 && value < 9999) {
                m_simYear = value;
                displayYearOnGui(m_simYear);
            }
        }
    }

    updateSummaryDisplays();
}

void MainWindow::onDisconnected()
{
    m_isConnected = false;
    __SER_STAT = STAT_NOTCON;

    disconnect(m_serial, &SerialHandler::lineReceived,
               this, &MainWindow::onSerialDataSignalRecieved);

    ui->btnConnectSerial->setText("Connect Serial");
    this->setWindowTitle("Device Disconnected");

    addEvent("Serial device disconnected.");
}

void MainWindow::onConnectionError(const QString &error)
{
    qDebug() << "Serial error:" << error;
    addEvent("Serial error: " + error);
}

void MainWindow::on_btnConnectSerial_clicked()
{
    QString portName = ui->combo_SerPorts->currentText();

    if (!portName.isEmpty() && !m_isConnected) {

        m_serial->closePort();

        connect(m_serial, &SerialHandler::lineReceived,
                this, &MainWindow::onSerialDataSignalRecieved,
                Qt::UniqueConnection);

        if (!m_serial->openPort(portName, 115200)) {
            disconnect(m_serial, &SerialHandler::lineReceived,
                       this, &MainWindow::onSerialDataSignalRecieved);

            QMessageBox::critical(this,
                                  "Serial Fault",
                                  "Could not open the selected serial port.",
                                  QMessageBox::Ok);

            __SER_STAT = STAT_ERR;
            m_isConnected = false;
        }
        else {
            m_isConnected = true;
            __SER_STAT = STAT_CON;

            ui->btnConnectSerial->setText("Disconnect Serial");
            this->setWindowTitle("Connected to NXP board");

            addEvent("Serial device connected.");

            // Wake the MCU display with a startup burst of blank commands
            sendDisplayStartupBurst();

            // Force re-send of all state on next tick
            for (int i = 0; i < 6; i++) m_lastLedState[i] = !m_lastLedState[i];
            m_lastYearSent = -1;
            m_lastScoreSent = -1;

            m_serial->queueCommand(QString("cY%1x").arg(m_simYear));
            m_serial->queueCommand(QString("cS%1x").arg(currentScore));
            m_lastYearSent = m_simYear;
            m_lastScoreSent = currentScore;
        }
    }
    else if (m_isConnected) {

        m_serial->closePort();

        m_isConnected = false;
        __SER_STAT = STAT_NOTCON;

        disconnect(m_serial, &SerialHandler::lineReceived,
                   this, &MainWindow::onSerialDataSignalRecieved);

        ui->btnConnectSerial->setText("Connect Serial");
        this->setWindowTitle("Device Disconnected");

        addEvent("Serial device disconnected.");
    }
    else {
        QMessageBox::warning(this,
                             "Warning",
                             "There are no known devices. Use Scan Ports first.",
                             QMessageBox::Ok);
    }
}

void MainWindow::on_horizontalSlider_valueChanged(int value)
{
    if (!gameRunning) {
        return;
    }

    sunUse = value;
    setUseLabels({"lbl_sunUse"}, value);
    updateSummaryDisplays();
}

void MainWindow::on_horizontalSlider_2_valueChanged(int value)
{
    if (!gameRunning) {
        return;
    }

    windUse = value;
    setUseLabels({"lbl_windUse"}, value);
    updateSummaryDisplays();
}

void MainWindow::on_horizontalSlider_3_valueChanged(int value)
{
    if (!gameRunning) {
        return;
    }

    coalUse = value;
    setUseLabels({"lbl_coalUse", "lbl_coalUse_2"}, value);
    updateSummaryDisplays();
}

void MainWindow::on_horizontalSlider_4_valueChanged(int value)
{
    if (!gameRunning) {
        return;
    }

    gasUse = value;
    setUseLabels({"lbl_gasUse", "lbl_gasUse_2", "lbl_nuclearUse"}, value);
    updateSummaryDisplays();
}

void MainWindow::on_SomeButtonDebug_clicked()
{
    m_serial->queueCommand("cL61x");
    m_serial->queueCommand("cNx");
    addEvent("DEBUG: sent cL61x + cNx refresh");
}
