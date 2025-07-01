#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QLayout>
#include <QVulkanInstance>
#include <QInputDialog>
#include <QMessageBox>
#include <QImageWriter>
#include "executiveengine.h"
#include "moveaxisdialog.h"
#include "crc.h"

/*
 * Some notes about MainWindow. It does all the slicing work and path planning
 *
 * It also holds all of the objects the ExecutiveEngine uses. I probably could integrate the executive directly into
 * the MainWindow, and maybe I will one day
 * */

static uint32_t maxLayers =0;
uint32_t calcMaxLayersVT( const QList<float_t> thicks, float_t maxZ);
uint32_t calcMaxLayersVT( const QList<float_t> thicks, float_t maxZ){
    float_t curLayerTop = 0.0f;
    //float32_t curSliceHeight = 0.0f;
    float_t curLayerInc = 0.0f;
    uint32_t curLayerIndex = 0;
    curLayerTop = curLayerInc = thicks[1];
    //curSliceHeight = curLayerInc/2.0;

    while( (curLayerTop - (curLayerInc/2.0)) < maxZ ){
        curLayerIndex++;
        // Check to see if we need to change the current incremental layer thickness
        for( uint32_t i = 0; i<thicks.length(); i++){
            if( !(i & 0x01) ) { // it's even
                uint32_t j = (uint32_t)thicks[i];
                if( (j-1) == curLayerIndex )
                    curLayerInc = thicks[i+1];
            }
        }
        curLayerTop += curLayerInc;
        //curSliceHeight = curLayerTop - (curLayerInc/2.0);
    }
    return curLayerIndex;
}
float_t calcSliceHeightVT( QList<float_t> thicks, uint32_t layer );
float_t calcSliceHeightVT( QList<float_t> thicks, uint32_t layer ){

    float_t curLayerTop = 0.0f;
    float_t curSliceHeight = 0.0f;
    float_t curLayerInc = 0.0f;
    uint32_t curLayerIndex = 0;

    curLayerTop = curLayerInc = thicks[1];
    curSliceHeight = curLayerInc/2.0;

    while( curLayerIndex < layer ){
        curLayerIndex++;
        for( uint32_t i = 0; i<thicks.length(); i++){
            if( !(i & 0x01) ) { // it's even
                uint32_t j = (uint32_t)thicks[i];
                if( (j-1) == curLayerIndex )
                    curLayerInc = thicks[i+1];
            }
        }
        curLayerTop += curLayerInc;
        curSliceHeight = curLayerTop - (curLayerInc/2.0f);

    }
    return curSliceHeight;
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_tempTimer(new QTimer(this))
{
    ui->setupUi(this);

    vulkanInstance = new QVulkanInstance;
    vulkanInstance->setLayers({ "VK_LAYER_KHRONOS_validation" });
    vulkanInstance->create();
    stl_window = new STLVulkanWindow();
    stl_window->setVulkanInstance(vulkanInstance);
    stl_window->setWidth(ui->viewer->width());
    stl_window->setHeight(ui->viewer->height());
    stl_window->m_sliceHeight = 0.1f;
    holder = QWidget::createWindowContainer(stl_window, ui->viewer);
    holder->setGeometry(0,0,ui->viewer->width(),ui->viewer->height());

    // Give the EE a reference to all of the relevant objects
    m_executiveEngine.setCNC(&m_CNCComPort);
    m_executiveEngine.setPHC(&m_PHCCommPort);
    m_executiveEngine.setVisualLog(ui->txtEngineMessages);
    m_executiveEngine.setImagePreview( ui->graphicsView );
    m_executiveEngine.setCurrentInstructionLabel(ui->lbl_NextExecInst);
    m_executiveEngine.setLayerCounterLabel(ui->lblCurrentLayer);
    m_executiveEngine.setUseInitList(ui->includeInitGCode);
    m_executiveEngine.setUseFinalList(ui->includeEndGCode);
    m_executiveEngine.setUse2Binders(ui->use2binders);
    m_executiveEngine.setEnablePowerScaling(ui->enablePowerScaling);
    m_executiveEngine.setReducePowerBy(ui->reducePowerBy);
    m_executiveEngine.setReducePowerEvery(ui->reducePowerEvery);
    m_executiveEngine.setRecoatList(ui->txtGCode);
    m_executiveEngine.setGenerateRecoat(ui->cmdGenerateRecoaterGCode);
    m_executiveEngine.setPowerItem(ui->recoatTable->item(9,0));
    m_executiveEngine.setheatSpeedItem(ui->recoatTable->item(2,0));
    m_executiveEngine.setdispSpeedItem(ui->recoatTable->item(10,0));
    m_executiveEngine.setsmoothSpeedItem(ui->recoatTable->item(15,0));

    // Give the CNC port driver a direct connection to a visual log
    m_CNCComPort.setVisualLog(ui->txtCNCLog);
    m_CNCComPort.setEnableVLog(ui->chkEnableCNCStream->checkState());
    m_CNCComPort.findPort();

    // Make the PH selector list
    for( uint32_t i=0; i<sizeof(phDescriptions)/sizeof(phDescriptions[0]) ; i++)
        ui->phSelect->addItem(phDescriptions[i].Description);

    // Give the PHC port driver a direct connection to a visual log
    m_PHCCommPort.setVisualLog(ui->txtPHCLog);
    m_PHCCommPort.setEnableVlog(ui->chkEnablePHCStream->checkState());
    m_PHCCommPort.findPort();

    m_curGcodePath = QDir::homePath() + "/printjobs/GCODE";
    m_curImagePath = QDir::homePath() + "/printjobs/IMAGES";
    m_curLaydownIniPath = QDir::homePath() + "/printjobs";
    m_curRecoatIniPath = QDir::homePath() + "/printjobs";
    m_curStlPath = QDir::homePath() + "/printjobs";
    m_curMaterial.loadName(QDir::homePath() + "/printjobs/lastMat.ini");
    ui->curMaterialFile->setText("Material File: " + m_curMaterial.MaterialINI);
    ui->currentLaydownINI->setText("Laydown File: " + m_curMaterial.MaterialINI);
    ui->currentRecoatINI->setText("Recoat File: " + m_curMaterial.MaterialINI);


    refreshFileLists();

    QString fileNameR = m_curMaterial.MaterialINI;
    RecoaterSettings::readFile(fileNameR, &this->recoaterSettings);
    writeRecoatingPage();
    //fileNameR = m_curLaydownIniPath + QString("/laydown.ini");
    LaydownSettings::readFile(fileNameR, &this->laydownSettings);
    ui->phSelect->setCurrentIndex(laydownSettings.phType);
    writeLaydownPage();
    this->on_cmdGenerateRecoaterGCode_clicked();
    this->on_makeTendingFunctions_clicked();
    ioConfig.readFile(fileNameR);
    m_IOStartup = true;

    ioChip = gpiod_chip_open("/dev/gpiochip0");
    if( !ioChip ) qDebug() << "chip not open";
    else {
    for( int i=2; i<=27; i++ ){
        QCheckBox *cbm = new QCheckBox();
        cbm->setChecked(ioConfig.isInput[i-2]);
        cbm->setProperty("IONum", i);
        ioLine = gpiod_chip_get_line( ioChip, i);
        if( ioConfig.isInput[i-2] ){
            gpiod_line_request_input_flags(ioLine, "stl-slicer", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN);
        }
        else {
            gpiod_line_request_output_flags( ioLine, "stl-slicer", 0,0);
        }

        if( gpiod_line_get_value( ioLine ) > 0 )
            ioConfig.isOn[i-2] = true;
        else
            ioConfig.isOn[i-2] = false;
        connect(cbm,SIGNAL(stateChanged(int)),this,SLOT(on_IOConfig_modeChanged(int)));
        QWidget * cont = new QWidget();
        QHBoxLayout *lt = new QHBoxLayout(cont);
        lt->addWidget(cbm);
        lt->setAlignment(Qt::AlignCenter);
        lt->setContentsMargins(0,0,0,0);
        cont->setLayout(lt);
        ui->tblIOPoints->setCellWidget(i-2,0,cont);

        QCheckBox *cbs = new QCheckBox();
        cbs->setChecked(ioConfig.isOn[i-2]);
        cbs->setProperty("IONum", i);
        cbm->setProperty("stateCB", QVariant::fromValue(cbs));
        if( ioConfig.isInput[i-2] ) cbs->setEnabled(false);
        else cbs->setEnabled(true);
        connect(cbs,SIGNAL(stateChanged(int)),this,SLOT(on_IOConfig_stateChanged(int)));
        cont = new QWidget();
        lt = new QHBoxLayout(cont);
        lt->addWidget(cbs);
        lt->setAlignment(Qt::AlignCenter);
        lt->setContentsMargins(0,0,0,0);
        cont->setLayout(lt);
        ui->tblIOPoints->setCellWidget(i-2,1,cont);

    } }
    //gpiod_chip_close(ioChip);
    m_IOStartup = false;
    m_executiveEngine.setParams(&recoaterSettings, &laydownSettings);

    m_serviceTimer = new QTimer(this);
    connect(m_serviceTimer, &QTimer::timeout, this, QOverload<>::of(&MainWindow::doServiceTimer));

    crcInit();
    // poll the temps to start
    ui->PHCInitScript->appendPlainText(QString(";:PHC:SF,%1").arg(laydownSettings.printFrequency));
    pollTemps();
    // Start the temp timer for automatic polling
    connect( m_tempTimer, &QTimer::timeout, this, &MainWindow::pollTemps);
    m_tempTimer->start(5000);
    //connect( &m_CNCComPort, &QSerialPort::readyRead,&m_CNCComPort, &CNCComManager::handleUnexpected );

}

MainWindow::~MainWindow()
{
    delete vulkanInstance;
    delete ui;

    //delete p_engine;

}
void MainWindow::on_IOConfig_modeChanged(int arg1){
    if( m_IOStartup ) return;
    QCheckBox *cb = qobject_cast<QCheckBox*>(sender());
    QCheckBox *cbs = cb->property("stateCB").value<QCheckBox*>();
    if (!cb) return;
    qDebug() << "get ioChip " << (ioChip = gpiod_chip_open("/dev/gpiochip0"));
    qDebug() << "get ioLine " << (ioLine = gpiod_chip_get_line( ioChip, cb->property("IONum").toInt()));

    if( cb->isChecked() ){
        ioConfig.isInput[cb->property("IONum").toInt()-2] = true;
        cbs->setEnabled(false);
        qDebug() << "to Input " << gpiod_line_request_input_flags(ioLine, "stl-slicer", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN);
    }
    else{
        ioConfig.isInput[cb->property("IONum").toInt()-2] = false;
        cbs->setEnabled(true);
        qDebug() << "to Output" << gpiod_line_request_output_flags( ioLine, "stl-slicer", 0, 0);
    }
    gpiod_chip_close(ioChip);
    ioConfig.saveFile(m_curMaterial.MaterialINI);
    return;
}
void MainWindow::on_IOConfig_stateChanged(int arg1){
    if( m_IOStartup ) return;
    QCheckBox *cb = qobject_cast<QCheckBox*>(sender());
    if (!cb) return;

    if( !ioConfig.isInput[cb->property("IONum").toInt()-2] ){
        ioChip = gpiod_chip_open("/dev/gpiochip0");
        ioLine = gpiod_chip_get_line( ioChip, cb->property("IONum").toInt());
        if( cb->isChecked() ){
            gpiod_line_set_value( ioLine, 1);
        }
        else {
            gpiod_line_set_value( ioLine, 0);
        }
    }
    gpiod_chip_close( ioChip );
    qDebug() << "State, Num " << cb->property("IONum").toInt();
    return;
}


void MainWindow::on_pushButton_clicked()
{

}
 void MainWindow::doServiceTimer()
 {

 }

 void MainWindow::pollTemps()
 {
     // Check the temps
     ui->initTempTable->item(0,1)->setText(QString("%1").arg(recoaterSettings.build_plate_temp));
     ui->initTempTable->item(1,1)->setText(QString("%1").arg(recoaterSettings.temperature));

     if( m_CNCComPort.isOpen() ){
         CNCTemperatures temps{};
         if (m_CNCComPort.getTemperatures(&temps)){
            ui->initTempTable->item(0,0)->setText(QString("%1").arg(temps.buildPlateTemp));
            ui->initTempTable->item(1,0)->setText(QString("%1").arg(temps.overheadTemp));
            ui->initTempTable->item(2,0)->setText(QString("%1").arg(temps.binderTemp));
            ui->initTempTable->item(3,0)->setText(QString("%1").arg(temps.ovenTemp));
            ui->initTempTable->item(4,0)->setText(QString("%1").arg(temps.feedPlateTemp));

            ui->tempDisplay->item(0,0)->setText(QString("%1").arg(temps.buildPlateTemp));
            ui->tempDisplay->item(1,0)->setText(QString("%1").arg(temps.overheadTemp));
            ui->tempDisplay->item(2,0)->setText(QString("%1").arg(temps.binderTemp));
            ui->tempDisplay->item(3,0)->setText(QString("%1").arg(temps.ovenTemp));
            ui->tempDisplay->item(4,0)->setText(QString("%1").arg(temps.feedPlateTemp));
         }
     }
     else{
         ui->initTempTable->item(0,0)->setText("Port Closed");
         ui->tempDisplay->item(0,0)->setText("Port Closed");
     }
 }

void MainWindow::on_tabWidget_currentChanged(int index)
{
    //qDebug() << "New Tab: " << index;
    switch(index){
    case 0: // INIT / END Script editor
        break;
    case 1:{ // Reccoater params
        CNCAxisPositions ap{};
        m_CNCComPort.getAxisPositions(&ap);
        ui->lblRecoatCurrentZ->setText(QString("Current Z = %1").arg(ap.z));
        ui->recoatCurrentF->setText(QString("Current F = %1").arg(ap.u));
        break;}
    case 2: // Print Setting
        break;
    case 3: // File Select
        break;
    case 4:{ // Script Builder
        QString gcodePath = m_executiveEngine.getGCodePath();
        ui->lblGCodeSourceDirectory->setText(gcodePath);
        QDir directory(gcodePath);
        QStringList gcodeFiles = directory.entryList(QStringList() << "*.GCODE", QDir::Files);
        ui->gcodeList->clear();
        foreach(QString filename, gcodeFiles) {
            QFileInfo fileInfo(directory.absoluteFilePath(filename));
            ui->gcodeList->addItem(fileInfo.fileName());
        }
        break;}
    case 5:{ // Execute Job
        CNCAxisPositions ap{};
        m_CNCComPort.getAxisPositions(&ap);
        ui->lblCurrentZ->setText(QString("%1").arg(ap.z));
        CNCTemperatures temps{};
        if( m_CNCComPort.getTemperatures(&temps)){
            ui->tempDisplay->item(0,0)->setText(QString("%1").arg(temps.buildPlateTemp));
            ui->tempDisplay->item(1,0)->setText(QString("%1").arg(temps.overheadTemp));
            ui->tempDisplay->item(2,0)->setText(QString("%1").arg(temps.binderTemp));
            ui->tempDisplay->item(3,0)->setText(QString("%1").arg(temps.ovenTemp));
            ui->tempDisplay->item(4,0)->setText(QString("%1").arg(temps.feedPlateTemp));
        }

    }
        break;
    case 6: // Functions
        break;
    }
    return;
}

bool MainWindow::readRecoatingPage(){
    bool ok=false;
    recoaterSettings.heat_start = ui->recoatTable->item(0,0)->text().toFloat(&ok);
    recoaterSettings.heat_end = ui->recoatTable->item(1,0)->text().toFloat(&ok);

    QString nums;
    QStringList numList;
    // Read a heat speed list or value
    nums = ui->recoatTable->item(2,0)->text();
    recoaterSettings.heat_speeds.clear();
    if( nums.contains(",")) {
        numList = nums.split(",");
        for( const QString &str : numList){
            bool ok = false;
            float num = str.toFloat(&ok);
            if( ok )
                recoaterSettings.heat_speeds.append(num);
        }
        recoaterSettings.heat_speed = numList[1].toFloat() * 60.0f;
    }
    else {
        recoaterSettings.heat_speed = ui->recoatTable->item(2,0)->text().toFloat(&ok);
        if(ok) recoaterSettings.heat_speed *=60.0f; // convert to mm/min
    }

    recoaterSettings.temperature = ui->recoatTable->item(3,0)->text().toFloat(&ok);
    recoaterSettings.fan1_pwm = ui->recoatTable->item(4,0)->text().toFloat(&ok);
    recoaterSettings.fan2_pwm = ui->recoatTable->item(5,0)->text().toFloat(&ok);

    recoaterSettings.disp_start = ui->recoatTable->item(6,0)->text().toFloat(&ok);
    recoaterSettings.disp_end = ui->recoatTable->item(7,0)->text().toFloat(&ok);
    recoaterSettings.disp_sections = ui->recoatTable->item(8,0)->text().toFloat(&ok);

    // Read a dispense power list or value
    nums = ui->recoatTable->item(9,0)->text();
    recoaterSettings.disp_powers.clear();
    if( nums.contains(",")){
        numList = nums.split(",");
        for( const QString &str : numList){
            bool ok = false;
            int num = str.toInt(&ok);
            if( ok )
                recoaterSettings.disp_powers.append(num);
        }
        ui->enablePowerScaling->setChecked(false);
        recoaterSettings.disp_power = numList[1].toInt();
    }
    else{
        recoaterSettings.disp_power = nums.toInt();
    }

    // Read a dispenser speed list or value
    nums = ui->recoatTable->item(10,0)->text();
    recoaterSettings.disp_speeds.clear();
    if( nums.contains(",")) {
        numList = nums.split(",");
        for( const QString &str : numList){
            bool ok = false;
            float num = str.toFloat(&ok);
            if( ok )
                recoaterSettings.disp_speeds.append(num);
        }
        recoaterSettings.disp_speed = numList[1].toFloat() * 60.0f;
    }
    else {
        recoaterSettings.disp_speed = ui->recoatTable->item(10,0)->text().toFloat(&ok);
        if(ok) recoaterSettings.disp_speed *=60.0f; // convert to mm/min
    }

    recoaterSettings.rolling_start = ui->recoatTable->item(11,0)->text().toFloat(&ok);
    recoaterSettings.rolling_end = ui->recoatTable->item(12,0)->text().toFloat(&ok);
    recoaterSettings.roller1_rpm = ui->recoatTable->item(13,0)->text().toFloat(&ok);;
    recoaterSettings.roller2_rpm = ui->recoatTable->item(14,0)->text().toFloat(&ok);
    // Read a rolling speed list or value
    nums = ui->recoatTable->item(15,0)->text();
    recoaterSettings.rolling_speeds.clear();
    if( nums.contains(",")) {
        numList = nums.split(",");
        for( const QString &str : numList){
            bool ok = false;
            float num = str.toFloat(&ok);
            if( ok )
                recoaterSettings.rolling_speeds.append(num);
        }
        recoaterSettings.rolling_speed = numList[1].toFloat() * 60.0f;
    }
    else {
        recoaterSettings.rolling_speed = ui->recoatTable->item(15,0)->text().toFloat(&ok);
        if(ok) recoaterSettings.rolling_speed *=60.0f; // convert to mm/min
    }


    recoaterSettings.rolling_len = abs(recoaterSettings.rolling_end - recoaterSettings.rolling_start);
    recoaterSettings.rolling_time = recoaterSettings.rolling_len / recoaterSettings.rolling_speed;
    recoaterSettings.roller1_turns = recoaterSettings.rolling_time * recoaterSettings.roller1_rpm;
    recoaterSettings.roller2_turns = recoaterSettings.rolling_time * recoaterSettings.roller2_rpm;
    recoaterSettings.z_retract = ui->recoatTable->item(16,0)->text().toFloat(&ok);;
    recoaterSettings.single_roller_motor = ui->useSingleRollerMotor->isChecked();
    recoaterSettings.x_move_speed = ui->tblSpeeds->item(0,0)->text().toFloat(&ok);
    if(!ok) recoaterSettings.x_move_speed = 6000.0f;
    else recoaterSettings.x_move_speed *= 60.0f;
    recoaterSettings.y_move_speed = ui->tblSpeeds->item(1,0)->text().toFloat(&ok);
    if(!ok) recoaterSettings.y_move_speed = 2.0f;
    else recoaterSettings.y_move_speed *= 60.0f;
    recoaterSettings.z_move_speed = ui->tblSpeeds->item(2,0)->text().toFloat(&ok);
    if(!ok) recoaterSettings.z_move_speed = 240.0f;
    else recoaterSettings.z_move_speed *= 60.0f;
    recoaterSettings.a_move_speed = ui->tblSpeeds->item(3,0)->text().toFloat(&ok);
    if(!ok) recoaterSettings.a_move_speed = 6000.0f;
    else recoaterSettings.a_move_speed *= 60.0f;
    recoaterSettings.x_home_sense = ui->tblSpeeds->item(4,0)->text().toFloat(&ok);
    recoaterSettings.y_home_sense = ui->tblSpeeds->item(5,0)->text().toFloat(&ok);
    recoaterSettings.z_home_sense = ui->tblSpeeds->item(6,0)->text().toFloat(&ok);
    recoaterSettings.a_home_sense = ui->tblSpeeds->item(7,0)->text().toFloat(&ok);
    recoaterSettings.useTurnsAsDelay = ui->turnsAsOnDelay->isChecked();
    recoaterSettings.enablePowerScaling = ui->enablePowerScaling->isChecked();
    recoaterSettings.build_plate_temp = ui->tblSpeeds->item(8,0)->text().toFloat(&ok);
    recoaterSettings.rolling_midpoint = ui->tblDualZ->item(0,0)->text().toFloat(&ok);
    recoaterSettings.rolling_fast_speed = ui->tblDualZ->item(1,0)->text().toFloat(&ok);
    if( ok ) recoaterSettings.rolling_fast_speed *= 60.0f;
    recoaterSettings.feed_powder_ratio = ui->tblDualZ->item(2,0)->text().toFloat(&ok);

    recoaterSettings.oven_temp = ui->initTempTable->item(3,1)->text().toFloat();
    recoaterSettings.feed_plate_temp = ui->initTempTable->item(4,1)->text().toFloat();

    return ok;
}
void MainWindow::writeRecoatingPage(){

    ui->recoatTable->item(0,0)->setText(QString("%1").arg(recoaterSettings.heat_start));
    ui->recoatTable->item(1,0)->setText(QString("%1").arg(recoaterSettings.heat_end));
    if( recoaterSettings.heat_speeds.length() == 0)
        ui->recoatTable->item(2,0)->setText(QString("%1").arg(recoaterSettings.heat_speed/60.0f));
    else {
        QString nums="";
        for( uint32_t i=0; i<recoaterSettings.heat_speeds.length(); i++){
            nums = nums + QString("%1").arg(recoaterSettings.heat_speeds[i]);
            if( i < recoaterSettings.heat_speeds.length()-1)
                nums = nums + ",";
        }
        ui->recoatTable->item(2,0)->setText(nums);
    }
    ui->recoatTable->item(3,0)->setText(QString("%1").arg(recoaterSettings.temperature));
    ui->recoatTable->item(4,0)->setText(QString("%1").arg(recoaterSettings.fan1_pwm));
    ui->recoatTable->item(5,0)->setText(QString("%1").arg(recoaterSettings.fan2_pwm));
    ui->recoatTable->item(6,0)->setText(QString("%1").arg(recoaterSettings.disp_start));
    ui->recoatTable->item(7,0)->setText(QString("%1").arg(recoaterSettings.disp_end));
    ui->recoatTable->item(8,0)->setText(QString("%1").arg(recoaterSettings.disp_sections));
    if( recoaterSettings.disp_powers.length() == 0 ){
        ui->recoatTable->item(9,0)->setText(QString("%1").arg(recoaterSettings.disp_power));
        ui->enablePowerScaling->setChecked(recoaterSettings.enablePowerScaling);
    }
    else {
        QString nums="";
        for( uint32_t i=0; i<recoaterSettings.disp_powers.length(); i++){
            nums = nums + QString("%1").arg(recoaterSettings.disp_powers[i]);
            if( i < recoaterSettings.disp_powers.length()-1)
                nums = nums + ",";
        }
        ui->recoatTable->item(9,0)->setText(nums);
        ui->enablePowerScaling->setChecked(false);
    }
    if( recoaterSettings.disp_speeds.length() == 0)
        ui->recoatTable->item(10,0)->setText(QString("%1").arg(recoaterSettings.disp_speed/60.0f));
    else {
        QString nums="";
        for( uint32_t i=0; i<recoaterSettings.disp_speeds.length(); i++){
            nums = nums + QString("%1").arg(recoaterSettings.disp_speeds[i]);
            if( i < recoaterSettings.disp_speeds.length()-1)
                nums = nums + ",";
        }
        ui->recoatTable->item(10,0)->setText(nums);
    }
    ui->recoatTable->item(11,0)->setText(QString("%1").arg(recoaterSettings.rolling_start));
    ui->recoatTable->item(12,0)->setText(QString("%1").arg(recoaterSettings.rolling_end));
    ui->recoatTable->item(13,0)->setText(QString("%1").arg(recoaterSettings.roller1_rpm));;
    ui->recoatTable->item(14,0)->setText(QString("%1").arg(recoaterSettings.roller2_rpm));
    if( recoaterSettings.rolling_speeds.length() == 0)
        ui->recoatTable->item(15,0)->setText(QString("%1").arg(recoaterSettings.rolling_speed/60.0f));
    else {
        QString nums="";
        for( uint32_t i=0; i<recoaterSettings.rolling_speeds.length(); i++){
            nums = nums + QString("%1").arg(recoaterSettings.rolling_speeds[i]);
            if( i < recoaterSettings.rolling_speeds.length()-1)
                nums = nums + ",";
        }
        ui->recoatTable->item(15,0)->setText(nums);
    }
    ui->recoatTable->item(16,0)->setText(QString("%1").arg(recoaterSettings.z_retract));
    ui->useSingleRollerMotor->setChecked(recoaterSettings.single_roller_motor);
    ui->tblSpeeds->item(0,0)->setText(QString("%1").arg(recoaterSettings.x_move_speed/60.0f));
    ui->tblSpeeds->item(1,0)->setText(QString("%1").arg(recoaterSettings.y_move_speed/60.0f));
    ui->tblSpeeds->item(2,0)->setText(QString("%1").arg(recoaterSettings.z_move_speed/60.0f));
    ui->tblSpeeds->item(3,0)->setText(QString("%1").arg(recoaterSettings.a_move_speed/60.0f));
    ui->tblSpeeds->item(4,0)->setText(QString("%1").arg(recoaterSettings.x_home_sense));
    ui->tblSpeeds->item(5,0)->setText(QString("%1").arg(recoaterSettings.y_home_sense));
    ui->tblSpeeds->item(6,0)->setText(QString("%1").arg(recoaterSettings.z_home_sense));
    ui->tblSpeeds->item(7,0)->setText(QString("%1").arg(recoaterSettings.a_home_sense));
    ui->tblSpeeds->item(8,0)->setText(QString("%1").arg(recoaterSettings.build_plate_temp));
    ui->turnsAsOnDelay->setChecked(recoaterSettings.useTurnsAsDelay);
    ui->tblDualZ->item(0,0)->setText(QString("%1").arg(recoaterSettings.rolling_midpoint));
    ui->tblDualZ->item(1,0)->setText(QString("%1").arg(recoaterSettings.rolling_fast_speed/60.0f));
    ui->tblDualZ->item(2,0)->setText(QString("%1").arg(recoaterSettings.feed_powder_ratio));

    ui->initTempTable->item(0,1)->setText(QString("%1").arg(recoaterSettings.build_plate_temp));
    ui->initTempTable->item(1,1)->setText(QString("%1").arg(recoaterSettings.temperature));
    ui->initTempTable->item(3,1)->setText(QString("%1").arg(recoaterSettings.oven_temp));
    ui->initTempTable->item(4,1)->setText(QString("%1").arg(recoaterSettings.feed_plate_temp));

}


void MainWindow::addLayerCommand(QPlainTextEdit* cmdList){
    // Add the current command item to the layer
    switch (ui->commandList->currentRow()){
    case 0:{ // Run a GCODE file
        if( ui->gcodeList->currentItem()!= nullptr ){
            cmdList->appendPlainText(QString(";:RUN:%1:").arg(ui->gcodeList->currentItem()->text()));
        }
        else
            QMessageBox::information(this,"Must Choose File", "Please select a gcode file from the list first");
        break;
    }
    case 1: // Do PRINTLAYERS command
    {
            cmdList->appendPlainText(ui->commandList->currentItem()->text());
            break;
    }
    case 2:{ // Move Z down 1 layer
        if( laydownSettings.layerThicknesses.length() == 0 ){
            float z1 = recoaterSettings.z_retract - laydownSettings.layerThickness;
            cmdList->appendPlainText("G91 ; RELATIVE POSITION MODE - INSERT RUN:DRY ABOVE THIS LINE");
            cmdList->appendPlainText(QString("G1 Z%1 F%2 ; MOVE THE BOX BY %1 MILLIMETERS").arg(-1.0*recoaterSettings.z_retract).arg(recoaterSettings.z_move_speed));
            cmdList->appendPlainText("G90 ; ABSOLUTE POSITION MODE - INSERT DISPENSE AFTER THIS LINE");
            cmdList->appendPlainText("G91 ; RELATIVE POSITION MODE");
            cmdList->appendPlainText(QString("G1 Z%1 F%2 ; MOVE THE BOX BY %3, %4 THICKNESS").arg(z1).arg(recoaterSettings.z_move_speed).arg(z1).arg(laydownSettings.layerThickness));
            cmdList->appendPlainText("G90 ; ABSOLUTE POSITON MODE - INSERT SMOOTH AFTER THIS LINE");
        }
        else {
            float z1 = recoaterSettings.z_retract - laydownSettings.layerThicknesses[0];
            cmdList->appendPlainText("G91 ; RELATIVE POSITION MODE - INSERT RUN:DRY ABOVE THIS LINE");
            cmdList->appendPlainText(QString("G1 Z%1 F%2 ; MOVE THE BOX BY %1 MILLIMETERS").arg(-1.0*recoaterSettings.z_retract).arg(recoaterSettings.z_move_speed));
            cmdList->appendPlainText("G90 ; ABSOLUTE POSITION MODE - INSERT DISPENSE AFTER THIS LINE");
            cmdList->appendPlainText("G91 ; RELATIVE POSITION MODE");
            cmdList->appendPlainText(QString("G1 Z%1 F%2 ; MOVE THE BOX BY %3, %4 THICKNESS").arg(z1).arg(recoaterSettings.z_move_speed).arg(z1).arg(laydownSettings.layerThicknesses[0]));
            cmdList->appendPlainText("G90 ; ABSOLUTE POSITON MODE - INSERT SMOOTH AFTER THIS LINE");
            z1 = recoaterSettings.z_retract - laydownSettings.layerThicknesses[1];
            cmdList->appendPlainText("G91 ; RELATIVE POSITION MODE - INSERT RUN:DRY ABOVE THIS LINE");
            cmdList->appendPlainText(QString("G1 Z%1 F%2 ; MOVE THE BOX BY %1 MILLIMETERS").arg(-1.0*recoaterSettings.z_retract).arg(recoaterSettings.z_move_speed));
            cmdList->appendPlainText("G90 ; ABSOLUTE POSITION MODE - INSERT DISPENSE AFTER THIS LINE");
            cmdList->appendPlainText("G91 ; RELATIVE POSITION MODE");
            cmdList->appendPlainText(QString("G1 Z%1 F%2 ; MOVE THE BOX BY %3, %4 THICKNESS").arg(z1).arg(recoaterSettings.z_move_speed).arg(z1).arg(laydownSettings.layerThicknesses[1]));
            cmdList->appendPlainText("G90 ; ABSOLUTE POSITON MODE - INSERT SMOOTH AFTER THIS LINE");

        }
        break;
    }
    case 3:{ // Move Z down 1 layer, Feedbox up by ratio
        float z1 = recoaterSettings.z_retract - laydownSettings.layerThickness;
        cmdList->appendPlainText("G91 ; RELATIVE POS MODE - INSERT RUN:DRY_B2B ABOVE THIS LINE");
        cmdList->appendPlainText(QString("G1 Z%1 F%2 ; LOWER THE BOX BY 0.5 MILLIMETERS").arg(-1.0*recoaterSettings.z_retract).arg(recoaterSettings.z_move_speed));
        cmdList->appendPlainText(QString("G1 Z%1 F%2 ; RAISE THE BOX BY %3").arg(z1).arg(recoaterSettings.z_move_speed).arg(z1));
        cmdList->appendPlainText(QString("G1 U%1 F%2 ; RAISE FEED BOX BY %1").arg(recoaterSettings.feed_powder_ratio*laydownSettings.layerThickness).arg(recoaterSettings.z_move_speed));
        cmdList->appendPlainText("G90 ; ABSOLUTE POS MODE - INSERT SMOOTH_B2B AFTER THIS LINE");
        break;
    }
    case 4: // Do INCLAYER command
    {
            cmdList->appendPlainText(ui->commandList->currentItem()->text());
            break;
    }
    case 5:{ // Move Axis To
        MoveAxisDialog moveDialog = MoveAxisDialog(this);
        if (moveDialog.exec() == QDialog::Accepted) {
            QString axis = moveDialog.getAxis();
            float position = moveDialog.getPosition();
            // Do something with the axis and position values
            cmdList->appendPlainText(QString("G1 %1%2 F600").arg(axis).arg(position));
        }
        break;
    }
    case 6:{ // Set BTT Out
        cmdList->appendPlainText(";:NOT-IMPLEMENTED:");
        break;
    }
    case 7:{ // Wait BTT Input
        cmdList->appendPlainText(";:NOT-IMPLEMENTED:");
        break;
    }
    case 8:{ // Record BTT Input
        cmdList->appendPlainText(";:NOT-IMPLEMENTED:");
        break;
    }
    case 9:{ // Set RPi output
        cmdList->appendPlainText(";:NOT-IMPLEMENTED:");
        break;
    }
    case 10:{ // Get RPi input
        cmdList->appendPlainText(";:NOT-IMPLEMENTED:");
        break;
    }
    case 11:{ // Record RPi input
        cmdList->appendPlainText(";:NOT-IMPLEMENTED:");
        break;
    }
    case 12: {
        cmdList->appendPlainText(";:PRINTLAYERM:");
        break;
    }
    case 13: {
        cmdList->appendPlainText(";:POSITION_Z:");
        break;
    }
    case 14: {
        cmdList->appendPlainText(";:POSITION_ZF:");
        break;
    }
    }

}
void MainWindow::on_btnAddLayerCommand_clicked()
{
    addLayerCommand(ui->txtLayerPipeline);

}


void MainWindow::on_pushButton_4_clicked()
{
    ui->txtLayerPipeline->clear();
}

bool MainWindow::readLaydownPage(){
    bool ok=false;
    // Read the laydown form and update the values
    //laydownSettings.jetCount = ui->tblPhysicalDescriptions->item(0,0)->text().toFloat(&ok);
    //laydownSettings.jetSpacing = ui->tblPhysicalDescriptions->item(1,0)->text().toFloat(&ok);
    laydownSettings.dropVolume = ui->tblPhysicalDescriptions->item(0,0)->text().toFloat(&ok);
    QString widths = ui->tblPhysicalDescriptions->item(1,0)->text();
    if( widths.contains(",")) { // it's a list
        QStringList pulsewidths = widths.split(",");
        laydownSettings.pulseWidths.clear();
        for( const QString &str : pulsewidths){
            float_t num = str.toFloat();
            laydownSettings.pulseWidths.append(num);
        }
        laydownSettings.pulseWidth = laydownSettings.pulseWidths[1];

    }
    else{
        laydownSettings.pulseWidth = ui->tblPhysicalDescriptions->item(1,0)->text().toFloat(&ok);
        laydownSettings.pulseWidths.clear();
    }
    laydownSettings.packingRate = ui->tblPhysicalDescriptions->item(2,0)->text().toFloat(&ok);

    laydownSettings.passCount = ui->tblLaydownDescription->item(0,0)->text().toFloat(&ok);
    laydownSettings.printSpeed = ui->tblLaydownDescription->item(1,0)->text().toFloat(&ok);
    if(ok) laydownSettings.printSpeed *=60.0f; // convert to mm/min
    laydownSettings.printFrequency = ui->tblLaydownDescription->item(2,0)->text().toFloat(&ok);
// Handle layer thickness with special consideration to having a file loaded
    QString thicks = ui->tblLaydownDescription->item(3,0)->text();
    if( !thicks.contains(","))
    {
        laydownSettings.layerThicknesses.clear();
        float old_lt = laydownSettings.layerThickness;
        laydownSettings.layerThickness = ui->tblLaydownDescription->item(3,0)->text().toFloat(&ok);
        if( ok ){
            if( laydownSettings.layerThickness != old_lt ){
                // Check to see if a file is loaded
                if( maxLayers > 0 ){
                    // Calculate the number of layers
                    uint32_t layerCount = ceil(stl_window->stlFiles[0].maxZ / laydownSettings.layerThickness);
                    //uint32_t layerCount = ceil(stl_window->m_stlMaxZ / laydownSettings.layerThickness);
                    m_executiveEngine.setLayerCount(layerCount);
                    ui->lblTotalLayers->setText(QString("%1").arg(layerCount));
                    ui->sbSliceLayer->setMinimum(1);
                    ui->sbSliceLayer->setMaximum(layerCount);
                    maxLayers = layerCount;
                    ui->lblLayersInFile->setText(QString("%1").arg(layerCount));
                }
            }
        }

    }
    else { // It's a list of values. Start Layer, Thickness, in *Pairs*
        QStringList thicknesses = thicks.split(",");
        laydownSettings.layerThicknesses.clear();
        for( const QString &str : thicknesses){
            bool ok = false;
            float_t num = str.toFloat(&ok);
            if( ok )
                laydownSettings.layerThicknesses.append(num);
        }
        laydownSettings.layerThickness = laydownSettings.layerThicknesses[1];
        if( maxLayers > 0){
            // Recalculate the number of layers
            //float_t elt = laydownSettings.layerThicknesses[0] + laydownSettings.layerThicknesses[1];
            //maxLayers = ceil(stl_window->stlFiles[0].maxZ / elt);
            maxLayers = calcMaxLayersVT(laydownSettings.layerThicknesses, stl_window->stlFiles[0].maxZ);
            m_executiveEngine.setLayerCount(maxLayers);
            ui->lblTotalLayers->setText(QString("%1").arg(maxLayers));
            ui->sbSliceLayer->setMinimum(1);
            ui->sbSliceLayer->setMaximum(maxLayers);
            ui->lblLayersInFile->setText(QString("%1").arg(maxLayers));

        }
    }
    laydownSettings.startPassX = ui->tblPassLocations->item(0,0)->text().toFloat(&ok);
    laydownSettings.endPassX = ui->tblPassLocations->item(1,0)->text().toFloat(&ok);
    laydownSettings.yAlignZero = ui->tblPassLocations->item(2,0)->text().toFloat(&ok);

    laydownSettings.primeX = ui->tblPrimingParams->item(0,0)->text().toFloat(&ok);
    laydownSettings.primeTime = ui->tblPrimingParams->item(1,0)->text().toFloat(&ok);
    laydownSettings.dripTime = ui->tblPrimingParams->item(2,0)->text().toFloat(&ok);

    laydownSettings.capX = ui->tblCappingParams->item(0,0)->text().toFloat(&ok);
    laydownSettings.capY = ui->tblCappingParams->item(1,0)->text().toFloat(&ok);
    laydownSettings.capSafeX = ui->tblCappingParams->item(2,0)->text().toFloat(&ok);

    laydownSettings.wipeStartX = ui->tblWipingParams->item(0,0)->text().toFloat(&ok);
    laydownSettings.wipeEndX = ui->tblWipingParams->item(1,0)->text().toFloat(&ok);
    laydownSettings.wipeSpeed = ui->tblWipingParams->item(2,0)->text().toFloat(&ok);
    if(ok) laydownSettings.wipeSpeed *= 60.0f; // conert to mm/min
    laydownSettings.yAxisBacklash = ui->yAxisBacklash->text().toFloat(&ok);
    laydownSettings.yHomeOffset = ui->yHomeOffset->text().toFloat(&ok);
    // make the randomizer lists
    QString nums;
    nums = ui->tblLaydownDescription->item(4,0)->text();
//    qDebug() <<"readLaydownPage item(4,0):" << nums;
    laydownSettings.majorOffsets.clear();
    QStringList numList = nums.split(",");
    for( const QString &str : numList){
        bool ok = false;
        int num = str.toInt(&ok);
        if( ok )
            laydownSettings.majorOffsets.append(num);
    }
    nums.clear();
    numList.clear();
    nums = ui->tblLaydownDescription->item(5,0)->text();
    numList = nums.split(",");
    laydownSettings.minorOffsets.clear();
    for( const QString &str : numList){
        bool ok = false;
        int num = str.toInt(&ok);
        if( ok )
            laydownSettings.minorOffsets.append(num);
    }

    // Calculate values
    laydownSettings.passSpacing = laydownSettings.jetSpacing / laydownSettings.passCount;
    laydownSettings.lineSpacing = (laydownSettings.printSpeed/60.0f) / laydownSettings.printFrequency;
    ui->lblPassSpacing->setText(QString("%1").arg(laydownSettings.passSpacing));
    ui->lblLineSpacing->setText(QString("%1").arg(laydownSettings.lineSpacing));
    if( laydownSettings.layerThicknesses.length() == 0){
        laydownSettings.saturation = (laydownSettings.dropVolume / 1000000.0f) /
                                 ((1.0-laydownSettings.packingRate)*
                                  (laydownSettings.lineSpacing * laydownSettings.passSpacing
                                  * laydownSettings.layerThickness));
        ui->lblSaturation->setText(QString("%1").arg(laydownSettings.saturation*100.0));
    }
    else{
        QString sats;
        float_t fsat = 0.0f;
        for( int i=0; i<laydownSettings.layerThicknesses.length(); i++ ){
            if( !(i&1) ){
                laydownSettings.saturation = (laydownSettings.dropVolume / 1000000.0f) /
                                         ((1.0-laydownSettings.packingRate)*
                                          (laydownSettings.lineSpacing * laydownSettings.passSpacing
                                          * laydownSettings.layerThicknesses[i+1]));
                sats += QString("%1, ").arg((double)(laydownSettings.saturation*100.0f), 5, u'f', 2 ,u'0');
                fsat += laydownSettings.saturation;
            }
        }
        sats += QString("Avg:%1").arg((double)(fsat*100.0f/(float_t)(laydownSettings.layerThicknesses.length()/2.0f)), 5, u'f', 2 ,u'0');
        ui->lblSaturation->setText(sats);
    }
    laydownSettings.tempCheckX = ui->tempCheckParams->item(0,0)->text().toFloat();
    laydownSettings.tempCheckY = ui->tempCheckParams->item(1,0)->text().toFloat();
    laydownSettings.binderTemp = ui->initTempTable->item(2,1)->text().toFloat();

    return ok;
}

void MainWindow::writeLaydownPage()
{
    // Write the laydown form from the values in the current global object

    // ui->tblPhysicalDescriptions->item(0,0)->setText(QString("%1").arg(laydownSettings.jetCount));
    // ui->tblPhysicalDescriptions->item(1,0)->setText(QString("%1").arg(laydownSettings.jetSpacing));
    ui->jetCount->setText(QString("%1").arg(phDescriptions[laydownSettings.phType].jetCount));
    ui->jetSpacing->setText(QString("%1").arg(phDescriptions[laydownSettings.phType].jetSpacing));
    ui->tblPhysicalDescriptions->item(0,0)->setText(QString("%1").arg(laydownSettings.dropVolume));
    if( laydownSettings.pulseWidths.length() <= 2)
        ui->tblPhysicalDescriptions->item(1,0)->setText(QString("%1").arg(laydownSettings.pulseWidth));
    else{
        QString pws;
        for( int i=0; i<laydownSettings.pulseWidths.length(); i++) {
            pws += QString("%1").arg(laydownSettings.pulseWidths[i]);
            if( i < laydownSettings.pulseWidths.length()-1 ) pws += ",";
        }
        ui->tblPhysicalDescriptions->item(1,0)->setText(pws);
        laydownSettings.pulseWidth = laydownSettings.pulseWidths[1];
    }
    ui->tblPhysicalDescriptions->item(2,0)->setText(QString("%1").arg(laydownSettings.packingRate));

    ui->tblLaydownDescription->item(0,0)->setText(QString("%1").arg(laydownSettings.passCount));
    ui->tblLaydownDescription->item(1,0)->setText(QString("%1").arg(laydownSettings.printSpeed/60.0f));
    ui->tblLaydownDescription->item(2,0)->setText(QString("%1").arg(laydownSettings.printFrequency));
    if( laydownSettings.layerThicknesses.length() == 0) {
        ui->tblLaydownDescription->item(3,0)->setText(QString("%1").arg(laydownSettings.layerThickness));
    }
    else {
        QString lts;
        for( int i=0; i<laydownSettings.layerThicknesses.length(); i++){
            lts += QString("%1").arg(laydownSettings.layerThicknesses[i]);
            if( i < laydownSettings.layerThicknesses.length()-1)
                lts += ",";
            ui->tblLaydownDescription->item(3,0)->setText(lts);
            laydownSettings.layerThickness = laydownSettings.layerThicknesses[1];
        }


    }

    ui->tblPassLocations->item(0,0)->setText(QString("%1").arg(laydownSettings.startPassX));
    ui->tblPassLocations->item(1,0)->setText(QString("%1").arg(laydownSettings.endPassX));
    ui->tblPassLocations->item(2,0)->setText(QString("%1").arg(laydownSettings.yAlignZero));

    ui->tblPrimingParams->item(0,0)->setText(QString("%1").arg(laydownSettings.primeX));
    ui->tblPrimingParams->item(1,0)->setText(QString("%1").arg(laydownSettings.primeTime));
    ui->tblPrimingParams->item(2,0)->setText(QString("%1").arg(laydownSettings.dripTime));

    ui->tblCappingParams->item(0,0)->setText(QString("%1").arg(laydownSettings.capX));
    ui->tblCappingParams->item(1,0)->setText(QString("%1").arg(laydownSettings.capY));
    ui->tblCappingParams->item(2,0)->setText(QString("%1").arg(laydownSettings.capSafeX));

    ui->tblWipingParams->item(0,0)->setText(QString("%1").arg(laydownSettings.wipeStartX));
    ui->tblWipingParams->item(1,0)->setText(QString("%1").arg(laydownSettings.wipeEndX));
    ui->tblWipingParams->item(2,0)->setText(QString("%1").arg(laydownSettings.wipeSpeed/60.0f));

    ui->yAxisBacklash->setText(QString("%1").arg(laydownSettings.yAxisBacklash));
    ui->yHomeOffset->setText(QString("%1").arg(laydownSettings.yHomeOffset));
    // make the ranodomizer lists
    QString nums="";
    if( laydownSettings.majorOffsets.length() == 0 ){
        ui->tblLaydownDescription->item(4,0)->setText("");
    }
    else{
        for( uint32_t i=0; i<laydownSettings.majorOffsets.length(); i++){
            nums = nums + QString("%1").arg(laydownSettings.majorOffsets[i]);
            if( i < laydownSettings.majorOffsets.length()-1)
                nums = nums + ",";
        }
        ui->tblLaydownDescription->item(4,0)->setText(nums);
    }

    nums.clear();
    if( laydownSettings.minorOffsets.length() == 0 ){
        ui->tblLaydownDescription->item(5,0)->setText("");
    }
    else{
        for( uint32_t i=0; i<laydownSettings.minorOffsets.length(); i++){
            nums = nums + QString("%1").arg(laydownSettings.minorOffsets[i]);
            if( i < laydownSettings.minorOffsets.length()-1)
                nums = nums + ",";
        }
        ui->tblLaydownDescription->item(5,0)->setText(nums);
    }
    ui->tempCheckParams->item(0,0)->setText(QString("%1").arg(laydownSettings.tempCheckX));
    ui->tempCheckParams->item(1,0)->setText(QString("%1").arg(laydownSettings.tempCheckY));
    // Calculate values
    laydownSettings.passSpacing = laydownSettings.jetSpacing / laydownSettings.passCount;
    laydownSettings.lineSpacing = laydownSettings.printSpeed / laydownSettings.printFrequency;
    laydownSettings.saturation = laydownSettings.dropVolume /
                                 ((1.0-laydownSettings.packingRate)*
                                  (laydownSettings.lineSpacing * laydownSettings.passSpacing
                                  * laydownSettings.layerThickness * 1000000.0));

    ui->lblPassSpacing->setText(QString("%1").arg(laydownSettings.passSpacing));
    ui->lblLineSpacing->setText(QString("%1").arg(laydownSettings.lineSpacing));
    ui->lblSaturation->setText(QString("%1").arg(laydownSettings.saturation*100.0));

    ui->initTempTable->item(2,1)->setText(QString("%1").arg(laydownSettings.binderTemp));

}
bool SaveGCodeFile(QPlainTextEdit *lines, const QString* outFilePath)
{
    uint32_t nest_level = 0;
    QFile file1;
    QTextStream out1;
    QFile file2;
    QTextStream out2;
    QString the_text = lines->toPlainText(); // ui->plainTextEdit is your QPlainTextEdit object
    QStringList the_inst = the_text.split('\n');
    for( uint32_t ic=0; ic<the_inst.length(); ic++ ){
        if( the_inst[ic].left(8) == ";:START:"){
            nest_level++;
            if( nest_level == 1){
                file1.setFileName(*outFilePath + the_inst[ic].mid(8,the_inst[ic].length()-9));
                file1.open(QIODevice::WriteOnly);
                out1.setDevice(&file1);
            }
            else{
                file2.setFileName(*outFilePath + the_inst[ic].mid(8,the_inst[ic].length()-9));
                file2.open(QIODevice::WriteOnly);
                out2.setDevice(&file2);
            }
        }
        else if( the_inst[ic].left(6) == ";:END:" ){
            if( nest_level == 1){
                file1.close();
                out1.reset();
            }
            else{
                file2.close();
                out2.reset();
            }
            nest_level--;
        }
        else {
            the_text = the_inst[ic];
            if( file1.isOpen() )
                out1 << the_text << "\n";
            if( file2.isOpen() )
                out2 << the_text << "\n";
        }
    }
    return true;

}
bool MainWindow::saveGCodeFiles(QPlainTextEdit *lines)
{ // Take any QPTE that is a GCODE file and commit it to disk

    QString pathName;
    pathName = QFileDialog::getExistingDirectory(this, "Select Directory", m_curGcodePath, QFileDialog::ShowDirsOnly) + "/";
    if( pathName == "") return false;

    m_curGcodePath = pathName;
    m_executiveEngine.setGCodePath(pathName);
    return SaveGCodeFile(lines, &pathName);
}
void MainWindow::on_btnUpdateLaydown_clicked()
{
    this->readLaydownPage();
}

void MainWindow::on_btnAddLayerException_clicked()
{
    addLayerCommand(ui->txtLayerExceptionPipeline);
}

struct rgba_stl{
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t alpha;
};

bool MainWindow::ripLayerImageStack(const QString &inFilepath, const QString &outFilepath){
    // This rips a series of images in the standard format for names
    uint32_t layer=0;
    bool ok = true;
    QString fn_raw;
    QImage inImg;
    QImage outImg;

    uint32_t inCol;
    uint32_t outCol;
    uint32_t outRow;
    uint32_t passId;
    uint32_t jetId;
    uint32_t pass_count=0;

    QRgb img_clr;

    while(ok){
        fn_raw = inFilepath + QString("/layer_%1.png").arg(layer);
        if (!inImg.load(fn_raw)){
            ok = false;
            break;
        }
        else { // Load the outImg from the input to insure they're the same size
            outImg.load(fn_raw);
            outImg = outImg.convertToFormat(QImage::Format_Mono);
            outImg.fill(0);
            if( inImg.width() == 512 ) pass_count = 4;
            else pass_count = 8;
            for( passId = 0; passId < pass_count; passId++ ){
                for( jetId = 0; jetId < JET_COUNT; jetId ++){
                    inCol = (jetId * pass_count ) + passId;
                    for( int inRow=0; inRow < inImg.size().height() ; inRow++){
                        img_clr = inImg.pixel(inCol, inRow);
                        outCol = floor(jetId/2) + ((inRow%4)*JET_COUNT);
                        if( !(jetId & 1) ) outCol += ODD_OFFSET;
                        outRow = (passId * JET_COUNT)+floor(inRow/pass_count);
                        if( qBlue(img_clr) > 120 ){
                            outImg.setPixel(outCol, outRow, 1);
                        }
                    }
                }
            } // end of for loop
            fn_raw = outFilepath + QString("/layer_%1RIP.png").arg(layer);
            layer++;
        }
    }
    if( layer > 0 )
        return true;
    else
        return ok;
}

QImage MainWindow::ripSLImage(const QImage &inImg){
    QImage outImg = QImage(inImg.size(), QImage::Format_Mono);
    outImg.fill(0);
    uint32_t inCol, outCol, outRow, passId, jetId;
    QRgb inClr;

    for( passId = 0; passId < laydownSettings.passCount; passId++ ){
        for( jetId = 0; jetId < JET_COUNT; jetId ++){
            inCol = (jetId * laydownSettings.passCount ) + passId;
            for( int inRow=0; inRow < inImg.size().height() ; inRow++){
                inClr = inImg.pixel(inCol, inRow);
                outCol = floor(jetId/2) + ((inRow%(uint32_t)laydownSettings.passCount)*JET_COUNT);
                if( !(jetId & 1) ) outCol += ODD_OFFSET;
                outRow = (passId * JET_COUNT)+floor(inRow/laydownSettings.passCount);
                if( qBlue(inClr) > 120 || qGreen(inClr) > 120 || qRed(inClr) > 120 ){
                    outImg.setPixel(outCol, outRow, 1);
                }
            }
        }
    }
    return outImg;
}

bool MainWindow::ripLayer(uint32_t layer, const QString &filePath)
{
    // This is, essentially, the RIP engine. The whole thing.

    // Let's see what we get with grab
    QImage img = stl_window->grab();
    QImage ripd_img = QImage(img.size(), QImage::Format_Mono);
    ripd_img.fill(0); // Clear the data

    unsigned int inCol;
    unsigned int outCol;
    unsigned int outRow;
    unsigned int passId;
    unsigned int jetId;

    QRgb img_clr;
//  volatile rgba_stl * rgbastl = (rgba_stl*)&img_clr;

    for( passId = 0; passId < laydownSettings.passCount; passId++ ){
        for( jetId = 0; jetId < JET_COUNT; jetId ++){
            inCol = (jetId * laydownSettings.passCount ) + passId;
            for( int inRow=0; inRow < img.size().height() ; inRow++){
                img_clr = img.pixel(inCol, inRow);
                outCol = floor(jetId/2) + ((inRow%4)*JET_COUNT);
                if( !(jetId & 1) ) outCol += ODD_OFFSET;
                outRow = (passId * JET_COUNT)+floor(inRow/laydownSettings.passCount);
                if( qBlue(img_clr) > 120 ){
                    ripd_img.setPixel(outCol, outRow, 1);
                }
            }
        }
    }
    bool ok;
    QString fn_raw = filePath + QString("/layer_%1.png").arg(layer);
    ok = img.save(fn_raw);
    QString fn_rip = filePath + QString("/layer_%1RIP.png").arg(layer);
    ok &= ripd_img.save(fn_rip);
    return ok;
}

#define CHANNEL_WIDTH 64 // It's 64 bit wide because it only holds half the values of any given group
#define CHANNEL_GAP 4

QImage MainWindow::ripQ1Image(const QImage inImg)
{
    // Take an image in and RIP it for a single Q-Class
    QImage outImage = QImage(inImg.size(), QImage::Format_MonoLSB);
    outImage.fill(0);

    uint32_t inCol,outCol,outRow,passId,jetId=0;
    QRgb inClr;
    for( passId = 0; passId < laydownSettings.passCount; passId++ ){
        for( jetId = 0; jetId < (int)laydownSettings.jetCount; jetId ++ ){
            inCol = (jetId * laydownSettings.passCount ) + passId;
            for( int inRow=0; inRow < inImg.size().height() ; inRow++){
                inClr = inImg.pixel(inCol, inRow);
                if( jetId >= 0 && jetId <= 63 ){ // It's the start
                    outCol = jetId * CHANNEL_GAP + 0;
                }
                else if( jetId >= 64 && jetId <= 127 ){ // It's the start
                    outCol = (jetId - 64) * CHANNEL_GAP + 1;
                }
                else if( jetId >= 128 && jetId <= 191 ){ // It's the start
                    outCol = (jetId - 128) * CHANNEL_GAP + 2;
                }
                else{  // jetId >=192 && jetId <= 255
                    outCol = (jetId - 192) * CHANNEL_GAP + 3;
                }
                outCol += (inRow%(int)laydownSettings.passCount)*(int)laydownSettings.jetCount;
                outRow = (passId * (int)laydownSettings.jetCount)+floor(inRow/(int)laydownSettings.passCount);
                if( qBlue(inClr) > 120 ){
                   outImage.setPixel(outCol, outRow, 1);
                }
            }
        }
    }
    return outImage;
}


// SG RIP
//const uint32_t lineOffsets[] = {88, 0, 304, 392, 112, 24, 328, 416 };
const uint32_t lineOffsets[] = {328, 416, 112, 24, 304, 392, 88, 0 };
const uint32_t colIDs[] = {2, 0, 4, 6, 3, 1, 5, 7};
//const uint32_t rowXF[] = { 2, 0, 4, 6, 3, 1, 5, 7 };

//const uint32_t colIDs[] = {4, 1, 16, 64, 8, 2, 32, 128};
QImage MainWindow::ripSGImage(const QImage inImg){
    QImage outImg = QImage( inImg.width(), inImg.height()+416, QImage::Format_MonoLSB);
    outImg.fill(0);
    QRgb inClr;
    for(uint32_t inCol = 0; inCol < inImg.width(); inCol++ ) {
        uint32_t jetRowIdx = inCol % 8;
        uint32_t lo = lineOffsets[jetRowIdx];
        uint32_t columnOut = (((uint32_t)floor(inCol/8))*8) + (7 - colIDs[jetRowIdx]);
        for( uint32_t inRow = 0; inRow < inImg.height(); inRow++) {
            inClr = inImg.pixel(inCol, inRow);
            if( qBlue(inClr) > 120 ){
                outImg.setPixel(columnOut, lo+inRow, 1);
            }
        }
    }
    return outImg;
}
const uint32_t lineOffsetsF[] = {88, 0, 304, 392, 112, 24, 328, 416 };
const uint32_t colIDsF[] = {5, 7, 3, 1, 4, 6, 2, 0};

QImage MainWindow::ripSGImageFwd(const QImage inImg){
    QImage outImg = QImage( inImg.width(), inImg.height()+416, QImage::Format_MonoLSB);
    outImg.fill(0);
    QRgb inClr;
    for(uint32_t inCol = 0; inCol < inImg.width(); inCol++ ) {
        uint32_t jetID = 1023 - inCol;
        uint32_t jetRowIdx = jetID % 8;
        uint32_t lo = lineOffsetsF[jetRowIdx];
        uint32_t columnOut = (uint32_t)floor(jetID/8)*8 + colIDsF[jetRowIdx];
        //uint32_t columnOut = (((uint32_t)floor(jetID/8))*8) + colIDs[jetRowIdx];
        for( uint32_t inRow = 0; inRow < inImg.height(); inRow++) {
            inClr = inImg.pixel(inCol, inRow);
            if( qBlue(inClr) > 120 ){
                outImg.setPixel(columnOut, lo+inRow, 1);
            }
        }
    }
    return outImg;
}

QImage MainWindow::ripQ2Image(const QImage inImg, uint32_t colorChannel)
{
    // Take an image in and RIP it for a single Q-Class
    QImage outImage = QImage(inImg.size(), QImage::Format_MonoLSB);
    outImage.fill(0);

    uint32_t inCol,outCol,outRow,passId,jetId=0;
    QRgb inClr;
    for( passId = 0; passId < laydownSettings.passCount; passId++ ){
        for( jetId = 0; jetId < (int)laydownSettings.jetCount ; jetId ++ ){
            inCol = (jetId * laydownSettings.passCount ) + passId;
            for( int inRow=0; inRow < inImg.size().height() ; inRow++){
                inClr = inImg.pixel(inCol, inRow);
                if( jetId >= 0 && jetId <= 63 ){ // It's the start
                    outCol = jetId * CHANNEL_GAP + 0;
                }
                else if( jetId >= 64 && jetId <= 127 ){ // It's the start
                    outCol = (jetId - 64) * CHANNEL_GAP + 1;
                }
                else if( jetId >= 128 && jetId <= 191 ){ // It's the start
                    outCol = (jetId - 128) * CHANNEL_GAP + 2;
                }
                else{  // jetId >=192 && jetId <= 255
                    outCol = (jetId - 192) * CHANNEL_GAP + 3;
                }
                outCol += (inRow%(int)laydownSettings.passCount)*(int)laydownSettings.jetCount;
                outRow = (passId * (int)laydownSettings.jetCount)+floor(inRow/(int)laydownSettings.passCount);
                if( qBlue(inClr) > 120 || qGreen(inClr) > 120 || qRed(inClr) > 120 || qAlpha(inClr) > 120 )
                    outImage.setPixel(outCol, outRow, 1);
/*                if( colorChannel == 0 ){
                    if( qBlue(inClr) > 120 ){
                        outImage.setPixel(outCol, outRow, 1);}
                }
                else if( colorChannel == 1 ){
                    if( qGreen(inClr) > 120 ){
                        outImage.setPixel(outCol, outRow, 1);}
                }
*/
            }
        }
    }
    return outImage;
}

bool MainWindow::ripLayerQ2(uint32_t layer, const QString &filePath)
{
    // This is, essentially, the RIP engine. The whole thing.

    // Let's see what we get with grab
    QImage img = stl_window->grab();
    QImage ripd_img = QImage(img.size(), QImage::Format_MonoLSB);
    ripd_img.fill(0); // Clear the data


    unsigned int inCol=0;
    unsigned int outCol=0;
    unsigned int outRow=0;
    unsigned int passId=0;
    unsigned int jetId=0;
    unsigned int dataChan=0;

    QRgb img_clr;
  //  volatile rgba_stl * rgbastl = (rgba_stl*)&img_clr;

    for( passId = 0; passId < laydownSettings.passCount; passId++ ){
        for( jetId = 0; jetId < JET_COUNT; jetId ++ ){
            inCol = (jetId * laydownSettings.passCount ) + passId;
            if( jetId & 1 ) { // odd jet
                dataChan = floor((jetId-1)/CHANNEL_WIDTH) + 4;
            }
            else{
                dataChan = floor(jetId/CHANNEL_WIDTH);
            }
            for( int inRow=0; inRow < img.size().height() ; inRow++){
                img_clr = img.pixel(inCol, inRow);
                if( jetId >= 0 && jetId <= 63 ){ // It's the start
                    outCol = jetId * CHANNEL_GAP + 0;
                }
                else if( jetId >= 64 && jetId <= 127 ){ // It's the start
                    outCol = (jetId - 64) * CHANNEL_GAP + 1;
                }
                else if( jetId >= 128 && jetId <= 191 ){ // It's the start
                    outCol = (jetId - 128) * CHANNEL_GAP + 2;
                }
                else{  // jetId >=192 && jetId <= 255
                    outCol = (jetId - 192) * CHANNEL_GAP + 3;
                }
                outCol += (inRow%(int)laydownSettings.passCount)*(int)laydownSettings.jetCount;
                outRow = (passId * (int)laydownSettings.jetCount)+floor(inRow/laydownSettings.passCount);
                if( qBlue(img_clr) > 120 ){
                    ripd_img.setPixel(outCol, outRow, 1);
                }
            }
        }
    }
    bool ok;
    QString fn_raw = filePath + QString("/layer_%1.png").arg(layer);
    ok = img.save(fn_raw);
    QString fn_rip = filePath + QString("/layer_%1RIP.png").arg(layer);
    ok &= ripd_img.save(fn_rip);
    return ok;
}


void MainWindow::on_sbSliceLayer_valueChanged(int value)
{
    if( value < 1) return;
    if( laydownSettings.layerThicknesses.length() == 0)
        stl_window->m_sliceHeight = ((float)value-0.5) * laydownSettings.layerThickness;
    else
        stl_window->m_sliceHeight = calcSliceHeightVT( laydownSettings.layerThicknesses, value-1);
    stl_window->m_cameraTransform.translation = {0.0f, 0.0f, stl_window->m_sliceHeight-0.005f};
    ui->lblSliceHeight->setText(QString("%1").arg(value));
    ui->lblSliceHeightmm->setText(QString("%1").arg(stl_window->m_sliceHeight));

    stl_window->requestUpdate();

}

#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort>

void MainWindow::on_pushButton_5_clicked()
{

}

void MainWindow::on_goStartUV_clicked()
{
    if(this->readRecoatingPage()){
        QString cmd = QString("G1 A%1 F6000\n").arg(recoaterSettings.heat_start);
        m_CNCComPort.sendGCodeCommand("G90\n");
        m_CNCComPort.sendGCodeCommand(cmd);
    }

}


void MainWindow::on_goEndUV_clicked()
{
    if(this->readRecoatingPage()){
        QString cmd = QString("G1 A%1 F6000\n").arg(recoaterSettings.heat_end);
        m_CNCComPort.sendGCodeCommand("G90\n");
        m_CNCComPort.sendGCodeCommand(cmd);
    }
}


void MainWindow::on_tryDryRoutine_clicked()
{
    if(this->readRecoatingPage()){
        if( ui->txtGCode->toPlainText().length() == 0 )
            this->on_cmdGenerateRecoaterGCode_clicked();
        QApplication::processEvents();

        QString recoat = ui->txtGCode->toPlainText();
        int dry_start = recoat.indexOf(";:START:DRY.GCODE:\n") + QString(";:START:DRY.GCODE:\n").length();
        int dry_end = recoat.indexOf(";:END:DRY.GCODE:",dry_start);
        QString dry_cmds = recoat.mid(dry_start, dry_end - dry_start);
        //qDebug() << dry_cmds;
        QStringList dry_list = dry_cmds.split("\n");
        int i=0;
        for( i=0; i<dry_list.length(); i++ ){
            if( !m_CNCComPort.sendGCodeCommand(dry_list[i])) break;
        }
        if( i==dry_list.length())
            m_CNCComPort.sendGCodeCommand(QString("G1 A5 F%1\n").arg(recoaterSettings.a_move_speed));
    }
}


void MainWindow::on_goDispenseStart_clicked()
{
    if(this->readRecoatingPage()){
        QString cmd = QString("G1 A%1 F6000\n").arg(recoaterSettings.disp_start);
        m_CNCComPort.sendGCodeCommand("G90\n");
        m_CNCComPort.sendGCodeCommand(cmd);
    }

}


void MainWindow::on_goDispenseEnd_clicked()
{
    if(this->readRecoatingPage()){
        QString cmd = QString("G1 A%1 F6000\n").arg(recoaterSettings.disp_end);
        m_CNCComPort.sendGCodeCommand("G90\n");
        m_CNCComPort.sendGCodeCommand(cmd);
    }
}


void MainWindow::on_tryDispenseTurns_clicked()
{
    if(this->readRecoatingPage()) {
        if( !ui->turnsAsOnDelay->isChecked() ) {
            QString cmd = QString("G1 B%1 F120\n").arg(recoaterSettings.disp_sections);
            m_CNCComPort.sendGCodeCommand("G90\n");
            m_CNCComPort.sendGCodeCommand("G92 B0\n");
            m_CNCComPort.sendGCodeCommand(QString("M106 P%1 S%2\n").arg(BTT_VIBE).arg(recoaterSettings.disp_power));
            m_CNCComPort.sendGCodeCommand("G4 P500\n");
            m_CNCComPort.sendGCodeCommand(cmd);
            m_CNCComPort.sendGCodeCommand(QString("M106 P%1 S%2\n").arg(BTT_VIBE).arg(0));
        }
        else { // This is the "priming" value for vibration only dispensers
            QString cmd1 = QString("M106 P%1 S%2\n").arg(BTT_VIBE).arg(recoaterSettings.disp_power);
            QString cmd2 = QString("G4 P%1\n").arg(recoaterSettings.disp_sections);
            QString cmd3 = QString("M106 P%1 S0\n").arg(BTT_VIBE);
            m_CNCComPort.sendGCodeCommand(cmd1);
            m_CNCComPort.sendGCodeCommand(cmd2);
            m_CNCComPort.sendGCodeCommand(cmd3);
        }
    }
}


void MainWindow::on_tryDispenseRoutine_clicked()
{
    if(this->readRecoatingPage()){
        this->on_cmdGenerateRecoaterGCode_clicked();
        QApplication::processEvents();
        QString recoat = ui->txtGCode->toPlainText();
        int disp_start = recoat.indexOf(";:START:DISPENSE.GCODE:\n") + QString(";:START:DISPENSE.GCODE:\n").length();
        int disp_end = recoat.indexOf(";:END:DISPENSE.GCODE:\n",disp_start);
        QString disp_cmds = recoat.mid(disp_start, disp_end - disp_start);
        //qDebug() << disp_cmds;
        QStringList disp_list = disp_cmds.split("\n");
        int i=0;
        for( i=0; i<disp_list.length(); i++ ){
            if( !m_CNCComPort.sendGCodeCommand(disp_list[i]) ) break;
        }
        if( i == disp_list.length() )
            m_CNCComPort.sendGCodeCommand("G1 A5 F6000\n");
    }
}


void MainWindow::on_goRollerStart_clicked()
{
    if(this->readRecoatingPage()){
        QString cmd = QString("G1 A%1 F6000\n").arg(recoaterSettings.rolling_start);
        m_CNCComPort.sendGCodeCommand("G90\n");
        m_CNCComPort.sendGCodeCommand(cmd);
    }
}


void MainWindow::on_goRollerEnd_clicked()
{
    if(this->readRecoatingPage()){
        QString cmd = QString("G1 A%1 F6000\n").arg(recoaterSettings.rolling_end);
        m_CNCComPort.sendGCodeCommand("G90\n");
        m_CNCComPort.sendGCodeCommand(cmd);
    }
}


void MainWindow::on_tryRoller1RPM_clicked()
{   // Will turn roller 10 times

    if(this->readRecoatingPage()){
        QString cmd = QString("G1 C10 F%1\n").arg(recoaterSettings.roller1_rpm);
        m_CNCComPort.sendGCodeCommand("G90\n");
        m_CNCComPort.sendGCodeCommand("G92 C0\n");
        m_CNCComPort.sendGCodeCommand(cmd);
    }
}


void MainWindow::on_tryRoller2RPM_clicked()
{
    if(this->readRecoatingPage()){
        QString cmd = QString("G1 U10 F%1\n").arg(recoaterSettings.roller2_rpm);
        m_CNCComPort.sendGCodeCommand("G90\n");
        m_CNCComPort.sendGCodeCommand("G92 U0\n");
        m_CNCComPort.sendGCodeCommand(cmd);
    }

}

void MainWindow::on_tryRollerRoutine_clicked()
{
    bool ok = true;
    if(this->readRecoatingPage()){
        this->on_cmdGenerateRecoaterGCode_clicked();
        QString recoat = ui->txtGCode->toPlainText();
        int smooth_start = recoat.indexOf(";:START:SMOOTH.GCODE:\n") + QString(";:START:SMOOTH.GCODE:\n").length();
        int smooth_end = recoat.indexOf(";:END:SMOOTH.GCODE:\n",smooth_start);
        QString smooth_cmds = recoat.mid(smooth_start, smooth_end - smooth_start);
        //qDebug() << smooth_cmds;
        QStringList smooth_list = smooth_cmds.split("\n");
        for( int i=0; i<smooth_list.length(); i++){
            if( !m_CNCComPort.sendGCodeCommand(smooth_list[i]) ){
                ok = false; break; }
        }
        if( ok ) m_CNCComPort.sendGCodeCommand("G1 A5 F6000\n");
    }
}


void MainWindow::on_goPassStartX_clicked()
{
    if( this->readLaydownPage()){
        QString cmd = QString("G1 X%1 F%2\n").arg(laydownSettings.startPassX).arg(recoaterSettings.x_move_speed);
        m_CNCComPort.sendGCodeCommand("G90\n");
        m_CNCComPort.sendGCodeCommand(cmd);
    }
}

void MainWindow::on_goToZeroA_clicked()
{
    m_CNCComPort.sendGCodeCommand("G90\n");
    m_CNCComPort.sendGCodeCommand(QString("G1 A1 F%1").arg(recoaterSettings.a_move_speed));
}

void MainWindow::on_tryRecoatAll_clicked()
{
    if(this->readRecoatingPage()){
        if( ui->txtGCode->toPlainText().length() == 0 )
            this->on_cmdGenerateRecoaterGCode_clicked();
        QApplication::processEvents();

        QString recoat = ui->txtGCode->toPlainText();
        int recoat_start = recoat.indexOf(";:START:RECOAT.GCODE:\n") + QString(";:START:RECOAT.GCODE:\n").length();
        int recoat_end = recoat.indexOf(";:END:RECOAT.GCODE:",recoat_start);
        QString recoat_cmds = recoat.mid(recoat_start, recoat_end - recoat_start);
        //qDebug() << dry_cmds;
        QStringList recoat_list = recoat_cmds.split("\n");
        int i=0;
        for( i=0; i<recoat_list.length(); i++ ){
            if( !m_CNCComPort.sendGCodeCommand(recoat_list[i])) break;
        }
        if( i==recoat_list.length())
            m_CNCComPort.sendGCodeCommand(QString("G1 A5 F%1\n").arg(recoaterSettings.a_move_speed));
    }

/*    QString fullRoutine = ui->txtGCode->toPlainText();
    QStringList instructions = fullRoutine.split('\n');
    for( const QString instruction : instructions ){
        m_CNCComPort.sendGCodeCommand(instruction );
    }
*/}


void MainWindow::on_goHomeX_clicked()
{
    QString fn = this->m_executiveEngine.getGCodePath() + "/HOMEXY.GCODE";
    m_executiveEngine.asynchExecuteGCodeFile(fn);

}

void MainWindow::on_goHomeY_clicked()
{
    QString fn = this->m_executiveEngine.getGCodePath() + "/HOMEXY.GCODE";
    m_executiveEngine.asynchExecuteGCodeFile(fn);
}

void MainWindow::on_goHomeA_clicked()
{
    QString fn = this->m_executiveEngine.getGCodePath() + "/HOMEA.GCODE";
    m_executiveEngine.asynchExecuteGCodeFile(fn);
}

void MainWindow::on_goHomeZ_clicked()
{
    QString fn = this->m_executiveEngine.getGCodePath() + "/HOMEZ.GCODE";
    m_executiveEngine.asynchExecuteGCodeFile(fn);
}

void MainWindow::on_goSendDirect_clicked()
{
    if( ui->txtCNCDirect->text().length() > 1){
        QStringList sl;
        sl.append(ui->txtCNCDirect->text().toUpper());
        m_executiveEngine.executeCommandList(sl, true);
    }
    return;
}

void MainWindow::on_generateBuildFiles_clicked()
{
    //  This will compile everything for the executive.
    // The first thing to understand, is that the pages are ignored. They are only there to make files.
    // Step 1 - Capture the lists of commands
    QString commandsRaw = ui->txtLayerPipeline->toPlainText();
    QStringList commands = commandsRaw.split("\n");

}

void MainWindow::on_btnLoadSTL_clicked()
{
    // Do Something
    stl_window->m_loading = true;
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open STL Model"), m_curStlPath, tr("STL Files (*.stl)"));
    if( fileName == "" ){ stl_window->m_loading = false; return; }

    stl_window->loadSTLFile(fileName);
//    stl_window->m_STLTransform.translation.y = -29.927;
//    stl_window->m_STLTransform.translation.x = -29.927;
    stl_window->stlFiles[stl_window->stlFiles.size()-1].transform.translation.y = 0.0f;
    stl_window->stlFiles[stl_window->stlFiles.size()-1].transform.translation.x = 0.0f;
    stl_window->stlFiles[stl_window->stlFiles.size()-1].transform.translation.z = 0.0f;
    stl_window->requestUpdate();
    QString qext = QString("Extents Min (%1,%2,%3) Max (%4,%5,%6)\nWidth:%7 Length:%8")
            .arg(stl_window->stlFiles[stl_window->stlFiles.size()-1].minX)
            .arg(stl_window->stlFiles[stl_window->stlFiles.size()-1].minY)
            .arg(stl_window->stlFiles[stl_window->stlFiles.size()-1].minZ)
            .arg(stl_window->stlFiles[stl_window->stlFiles.size()-1].maxX)
            .arg(stl_window->stlFiles[stl_window->stlFiles.size()-1].maxY)
            .arg(stl_window->stlFiles[stl_window->stlFiles.size()-1].maxZ)
            .arg(stl_window->stlFiles[stl_window->stlFiles.size()-1].width)
            .arg(stl_window->stlFiles[stl_window->stlFiles.size()-1].length);
    QMessageBox::information(this, "STL Extents", qext);
    // Calculate the number of layers
    if( laydownSettings.layerThicknesses.length() == 0)
        maxLayers = ceil(stl_window->stlFiles[stl_window->stlFiles.size()-1].maxZ / laydownSettings.layerThickness);
    else {
        maxLayers = calcMaxLayersVT(laydownSettings.layerThicknesses,  stl_window->stlFiles[stl_window->stlFiles.size()-1].maxZ );
    }
    qDebug() << "maxL:" << maxLayers;
    m_executiveEngine.setLayerCount(maxLayers);
    ui->lblCurrentLayer->setText(QString("%1").arg(m_executiveEngine.currentLayer()+1));
    ui->lblTotalLayers->setText(QString("%1").arg(maxLayers));
    ui->sbSliceLayer->setMinimum(1);
    ui->sbSliceLayer->setMaximum(maxLayers);
    ui->sbSliceLayer->setEnabled(true);
    ui->lblLayersInFile->setText(QString("%1").arg(maxLayers));
    //ui->sbSliceLayer->setMinimum(0);
    stl_window->m_loading = false;
    stl_window->requestUpdate();
    QFileInfo fi(fileName);
    m_curStlPath = fi.absoluteFilePath();

}

#include <QTime>
bool MainWindow::makeMultiThicknessStack( QString outFilePath )
{
    // This routine will make a stack of multi-thickness layer images
    // NOTE: The bitmap size and spacing is fixed across all layers

    // This will be called by on_btnMakeImages_clicked(), so certain things can be assumed

    // The first version will only support one color and 1024x1024 bitmaps

    QImage src;
    QImage ripd;

    float curSliceHeight = 0.0f;
    float curLayerTop = 0.0f;
    float curInc;
    float maxZ = stl_window->stlFiles[0].maxZ;
    uint32_t curLayer = 0;
    stl_window->stlFiles[0].bindData = true; // Reveal one object
    while( curLayerTop < maxZ ){
        // Check to see if we need to change the current incremental layer thickness
        for( uint32_t i = 0; i<laydownSettings.layerThicknesses.length(); i++){
            if( !(i & 0x01) ) { // it's even
                uint32_t j = (uint32_t)laydownSettings.layerThicknesses[i];
                if( (j-1) == curLayer )
                    curInc = laydownSettings.layerThicknesses[i+1];
            }
        }
        curLayerTop += curInc;
        curSliceHeight = curLayer - (curInc / 2.0f);
        stl_window->m_sliceHeight = curSliceHeight;

    }

    return false;
}
void MainWindow::on_btnMakeImages_clicked()
{
    // Let's try going through the images stack
    if( stl_window->stlFiles.size() == 0 ) return; // No file loaded, or some other error

    if( !readLaydownPage() ) return;
    if( !readRecoatingPage() ) return;

    static bool b_cancelSlice = false; // Allow cancel
    static bool b_inSlicer = false; // prevent deep re-entry
    if(b_inSlicer & !b_cancelSlice){ // This is the 1st reentrat test.
        b_cancelSlice = true;   // This condition only happens if running ad not cancelled
        return;
    }
    else if( b_inSlicer ) return; // It's because the cancel needs to break the loop below.
    b_inSlicer = true; // Mark we've entered the routine
    QString filePath = QFileDialog::getExistingDirectory(this, "Select Output Folder",m_curImagePath);
    if( filePath == "" ) return;
    m_curImagePath = filePath;
    float32_t curLayerTop = 0.0f;
    float32_t curSliceHeight = 0.0f;
    float32_t curLayerInc = 0.0f;
    float32_t expectZOffset = 0.0f;
    uint32_t curLayerIndex = 0;
    //qDebug() << filePath;
    QImage srcImg;
    QImage ripdImg;
    QImageWriter imgWriter;
    uint32_t lines = (uint32_t)laydownSettings.jetCount * (uint32_t)laydownSettings.passCount;
    if( laydownSettings.phType == Q_CLASS_SG ) lines += 416;
    uint32_t maj;
    uint32_t min;
    float y_shift_axis=0.0f;
    float x_shift_model = -32.512f;
    const float MAX_SHIFT = 5.08;// + (float)(PASS_COUNT-1)*PASS_SPACE;

    ui->btnMakeImages->setText("Cancel"); //
    // hide all objects
    for( uint32_t objCt = 0; objCt < stl_window->stlFiles.size(); objCt++)
        stl_window->stlFiles[objCt].bindData = false;

    // Cycle thru the objects to slice
    for( uint32_t objCt=0; objCt<stl_window->stlFiles.size(); objCt++ ){ // Reset for each object
        if( laydownSettings.layerThicknesses.length() <= 1 ) curLayerTop = laydownSettings.layerThickness;
        else curLayerTop = laydownSettings.layerThicknesses[1];
        curSliceHeight = curLayerTop / 2.0f;
        curLayerInc = curLayerTop;
        expectZOffset = 0.0;
        curLayerIndex = 0;
        stl_window->stlFiles[objCt].bindData = true; // Enable the object for rendering

        while( (curLayerTop - (curLayerInc/2.0)) < stl_window->stlFiles[objCt].maxZ ){ // Slice the layers until we run out of Z
            //qDebug() << "cSH: " << curSliceHeight << " cLI: " << curLayerInc << " eZO: " << expectZOffset << " cLIx: " << curLayerIndex;
            QApplication::processEvents();
            if( b_cancelSlice ){ b_cancelSlice = false; b_inSlicer = false; ui->btnMakeImages->setText("Make Images"); return; } // Dump out immediately
            stl_window->m_sliceHeight = curSliceHeight;
            stl_window->m_cameraTransform.translation = {0.0f, 0.0f, stl_window->m_sliceHeight-0.005f};
            ui->lblSliceHeight->setText(QString("%1").arg(curLayerIndex));
            ui->lblSliceHeightmm->setText(QString("%1").arg(stl_window->m_sliceHeight));
            QApplication::processEvents();
            if( laydownSettings.majorOffsets.length() > 1){
                // Calculate Randomization Shift
                maj = laydownSettings.majorOffsets[((curLayerIndex) % laydownSettings.majorOffsets.length())];
                min = laydownSettings.minorOffsets[((curLayerIndex) % laydownSettings.minorOffsets.length())];
                y_shift_axis = (float)maj * laydownSettings.jetSpacing + (float)min * laydownSettings.passSpacing;
                x_shift_model = y_shift_axis - (MAX_SHIFT/2.0f);
            }
            else {
                y_shift_axis = 0.0f;
                x_shift_model = -1.0*(IMAGE_WID_MM-stl_window->stlFiles[objCt].width)/2.0F;
            }
            stl_window->stlFiles[objCt].transform.translation.x = x_shift_model;


            if( laydownSettings.jetCount * laydownSettings.passCount == 512 ){
                stl_window->ortho_top = -32.512f;
                stl_window->ortho_bottom = 32.512f;
                stl_window->ortho_left = -32.512f;
                stl_window->ortho_right = 32.512f;

                stl_window->requestUpdate();
                QApplication::processEvents();
                srcImg = stl_window->grab();
            }
            else {
                srcImg = makeQuadImage();
            }

            if( laydownSettings.phType == S_CLASS ){
                m_PHCCommPort.setModuleType(S_CLASS);
                ripdImg = this->ripSLImage( srcImg );
            }
            else if( stl_window->stlFiles.size() == 1 && laydownSettings.phType == Q_CLASS_1S ){
                m_PHCCommPort.setModuleType(Q_CLASS_1S);
                ripdImg = this->ripQ1Image( srcImg );
            }
            else if( stl_window->stlFiles.size() > 1 && laydownSettings.phType == Q_CLASS_2S){
                m_PHCCommPort.setModuleType(Q_CLASS_2S);
                ripdImg = this->ripQ2Image( srcImg, objCt );
            }
            else if( laydownSettings.phType == Q_CLASS_SG ){
                m_PHCCommPort.setModuleType(Q_CLASS_SG);
                ripdImg = this->ripSGImageFwd(srcImg);
            }
            bool ok;
         /*   if( stl_window->stlFiles.size() > 1){ */
                QString fn_raw = filePath + QString("/layer_%1_color_%2.png").arg(curLayerIndex).arg(objCt);
                imgWriter.setFileName(fn_raw);
                imgWriter.setFormat("png");
                imgWriter.setText("layer", QString("%1").arg(curLayerIndex));
                imgWriter.setText("sliceHeight", QString("%1").arg(stl_window->m_sliceHeight));
                imgWriter.setText("sliceThickness", QString("%1").arg(curLayerInc));
                imgWriter.setText("expectZOffset", QString("%1").arg(expectZOffset) );
                imgWriter.write(srcImg);
                QString fn_rip = filePath + QString("/layer_%1RIP_color_%2.png").arg(curLayerIndex).arg(objCt);
                imgWriter.setFileName(fn_rip);
                imgWriter.write(ripdImg);
          /*  }
            else {
                QString fn_raw = filePath + QString("/layer_%1.png").arg(curLayerIndex);
                imgWriter.setFileName(fn_raw);
                imgWriter.setFormat("png");
                imgWriter.setText("layer", QString("%1").arg(curLayerIndex));
                imgWriter.setText("sliceHeight", QString("%1").arg(stl_window->m_sliceHeight));
                imgWriter.setText("expectZOffset", QString("%1").arg(expectZOffset) );
                imgWriter.write(srcImg);
                QString fn_rip = filePath + QString("/layer_%1RIP.png").arg(curLayerIndex);
                imgWriter.setFileName(fn_rip);
                imgWriter.write(ripdImg);
            } */

            QFile gcodeFile(filePath + QString("/layer%1.gcode").arg(curLayerIndex));
            if( !gcodeFile.open(QIODevice::WriteOnly)){
                //qDebug() << "Failed to open: " << filePath + QString("/layer%1.gcode").arg(layer);
                break;
            } else {
                QTextStream stream(&gcodeFile);
                stream << "G90; ABSOLUTE MODE\n";
                //            stream << QString("M106 P%1 S255; LOWER CAP\n").arg(BTT_CAP);
                //            stream << "G4 P250 ; WAIT 250MS\n";
                stream << QString("G1 X%1 F%2; MOVE TO CAP SAFE X FOR PASS 1\n")
                          .arg(laydownSettings.capSafeX)
                          .arg(recoaterSettings.x_move_speed);
                //            stream << QString("M106 P%1 S0; RAISE CAP\n").arg(BTT_CAP);
                stream << QString("G1 X%1 F%2; PREPOSITION FOR PASS 1\n")
                          .arg(laydownSettings.startPassX)
                          .arg(recoaterSettings.x_move_speed);
                stream << QString("G1 Y%1 F180; PREPOSITION FOR PASS 1\n")
                          .arg(laydownSettings.yAlignZero + y_shift_axis + laydownSettings.yAxisBacklash);
                stream << QString("G1 Y%1 F120; FINAL POSITION Y\n")
                          .arg(laydownSettings.yAlignZero + y_shift_axis);
                stream << "M18 Y ; SHUTDOWN Y DRIVER\n";
                stream << "G4 P1 ; PAUSE 1MS\n";

                for( int passid=0; passid < laydownSettings.passCount; passid++ ){
                    stream << QString(";:PHC:EP,%1,%2\n")
                              .arg(passid)
                              .arg(lines);
                    stream << QString("G1 X%1 F%2 ; EXECUTE PASS %3\n")
                              .arg(laydownSettings.endPassX)
                              .arg(laydownSettings.printSpeed)
                              .arg(passid); // Execute pass
                    //                stream << "G4 P500 ; PAUSE 500MS\n";
                    stream << QString("G1 X%1 F%2; RETURN TO START / ADV Y\n")
                              .arg(laydownSettings.startPassX)
                              .arg(laydownSettings.printSpeed);
                    //                stream << "G4 P500 ; PAUSE 500MS\n";
                    if( laydownSettings.passCount > 1){
                        stream << "G91 ; RELATIVE POSITION MODE\n";
                        stream << QString("G1 Y%1 F60;  ADVANCE Y\n")
                                  .arg(-laydownSettings.passSpacing);
                        stream << "M18 Y ; SHUTDOWN Y DRIVER\n";
                        stream << "G90 ; ABSOLUTE POSITION MODE\n";
                        stream << "G4 P1 ; PAUSE 50MS\n";
                    }
                }

                stream << QString("G1 X%1 F%2\n")
                          .arg(laydownSettings.capSafeX)
                          .arg(recoaterSettings.x_move_speed); // Return to cap
                //stream << QString("G1 Y%1 F120\n")
                //          .arg(laydownSettings.capY);
                stream << "M18 Y ; SHUTDOWN Y DRIVER\n";
                //stream << "G4 P100 ; PAUSE 100MS\n";
                gcodeFile.close();

                curLayerIndex++;
                // Check to see if we need to change the current incremental layer thickness
                for( uint32_t i = 0; i<laydownSettings.layerThicknesses.length(); i++){
                    if( !(i & 0x01) ) { // it's even
                        uint32_t j = (uint32_t)laydownSettings.layerThicknesses[i];
                        if( (j-1) == curLayerIndex )
                            curLayerInc = laydownSettings.layerThicknesses[i+1];
                    }
                }
                curLayerTop += curLayerInc;
                curSliceHeight = curLayerTop - (curLayerInc/2.0);
                expectZOffset = curLayerTop - curLayerInc;
            } // end of writing gcode file
        } // end of while loop for slicing thru the layers
        QSettings colorInfoFile((filePath + QString("/color%1.ini").arg(objCt)), QSettings::IniFormat);
        colorInfoFile.beginGroup(QString("Color-%1-Info").arg(objCt));
        colorInfoFile.setValue("Layers", curLayerIndex);
        //colorInfoFile.setValue("");
        colorInfoFile.endGroup();
        stl_window->stlFiles[objCt].bindData = false; // Hide it again
    } // End of Object Counter
    for( uint32_t objCt = 0; objCt < stl_window->stlFiles.size(); objCt++) stl_window->stlFiles[objCt].bindData = true;
    ui->btnMakeImages->setText("Make Images");
    stl_window->ortho_top = -32.512f;
    stl_window->ortho_bottom = 32.512f;
    stl_window->ortho_left = -32.512f;
    stl_window->ortho_right = 32.512f;

    stl_window->requestUpdate();
    b_inSlicer = false;
    b_cancelSlice = false;

}



void MainWindow::on_cmdMakeRecoatDefaults_clicked()
{  // Save as INI file
    if( !this->readRecoatingPage() ){
        QMessageBox::critical(this, "Error reading Recoater page", "A bad value was found in the recoating settings page");
        return;
    }

    QString fileNameR = QFileDialog::getSaveFileName(this, "Select File Name", m_curRecoatIniPath, "ini Files (*.ini)");
    if( fileNameR.length() == 0 ) return; // Cancel selected
    m_curMaterial.MaterialINI = fileNameR;
    m_curMaterial.saveName(QDir::homePath() + "/printjobs/lastMat.ini");
    ui->curMaterialFile->setText("Material File: " + m_curMaterial.MaterialINI);
    ui->currentLaydownINI->setText("Laydown File: " + m_curMaterial.MaterialINI);
    ui->currentRecoatINI->setText("Recoat File: " + m_curMaterial.MaterialINI);
    RecoaterSettings::writeFile(fileNameR, &this->recoaterSettings);
    QFileInfo fi(fileNameR);
    QString selectedPath = fi.absoluteFilePath();
    m_curRecoatIniPath = selectedPath;
}


void MainWindow::on_cmdGenerateRecoaterGCode_clicked()
{
    if(!readRecoatingPage()) return;
    ui->txtGCode->clear();
    ui->txtGCode->appendPlainText(";:START:RECOAT.GCODE:");
    ui->txtGCode->appendPlainText(";:START:DRY.GCODE:");
    ui->txtGCode->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 X%1 F%2 ; ENSURE PRINT AXIS HOME").arg(laydownSettings.capX).arg(recoaterSettings.x_move_speed));
#ifndef USE_UV
    ui->txtGCode->appendPlainText(QString("M104 S%1 ; SET THE HEATER TEMPERATURE").arg(recoaterSettings.temperature));
#endif
#ifndef USE_UV
    ui->txtGCode->appendPlainText(QString("M106 P%1 S%2 ; START FAN1").arg(BTT_FAN1).arg(255));
    ui->txtGCode->appendPlainText(QString("M106 P%1 S%2 ; START FAN2").arg(BTT_FAN2).arg(255));
    ui->txtGCode->appendPlainText(QString("G4 P500 ; PAUSE 500MS FOR FAN START"));
    ui->txtGCode->appendPlainText(QString("M106 P%1 S%2 ; SET FAN1 FINAL SPEED").arg(BTT_FAN1).arg(recoaterSettings.fan1_pwm));
    ui->txtGCode->appendPlainText(QString("M106 P%1 S%2 ; SET FAN2 FINAL SPEED").arg(BTT_FAN2).arg(recoaterSettings.fan2_pwm));
    ui->txtGCode->appendPlainText(QString("G4 P50 ; PAUSE 50MS FOR FANS"));
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; MOVE TO HEAT START")
                                  .arg(recoaterSettings.heat_start)
                                  .arg(recoaterSettings.a_move_speed));
#elif USE_UV
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; MOVE TO HEAT START")
                                  .arg(recoaterSettings.heat_start)
                                  .arg(recoaterSettings.a_move_speed));
    ui->txtGCode->appendPlainText(QString("M106 P%1 S255 ; START UV LAMP").arg(BTT_UV));
#endif
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; EXECUTE CURE MOVE").arg(recoaterSettings.heat_end).arg(recoaterSettings.heat_speed));
#ifndef USE_UV
    ui->txtGCode->appendPlainText(QString("M107 P%1 ; STOP FAN 1").arg(BTT_FAN1));
    ui->txtGCode->appendPlainText(QString("M107 P%1 ; STOP FAN 2").arg(BTT_FAN2));
    ui->txtGCode->appendPlainText("G4 P50 ; PAUSE 50MS FOR FAN STOP");
#elif USE_UV
    ui->txtGCode->appendPlainText(QString("M107 P%1 ; STOP UV LAMP").arg(BTT_UV));
#endif
    ui->txtGCode->appendPlainText(";:END:DRY.GCODE:");
    ui->txtGCode->appendPlainText(";:START:DISPENSE.GCODE:");
    ui->txtGCode->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 X%1 F%2 ; ENSURE PRINT AXIS HOME").arg(laydownSettings.capX).arg(recoaterSettings.x_move_speed));
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; GO BACK TO DISPENSE START")
                                  .arg(recoaterSettings.disp_start)
                                  .arg(recoaterSettings.a_move_speed));
    //ui->txtGCode->appendPlainText(QString("M106 P%1 S255; TURN ON FAN / VAC FOR BOX OVERFLOW").arg(BTT_VAC));
    ui->txtGCode->appendPlainText(QString("M106 P%1 S%2; TURN ON DISPENSER VIBE").arg(BTT_VIBE).arg(recoaterSettings.disp_power));
    //ui->txtGCode->appendPlainText("G4 S1 ; PAUSE 1 SECOND FOR FAN");
    if( recoaterSettings.disp_sections>=1 && !recoaterSettings.useTurnsAsDelay){
        ui->txtGCode->appendPlainText("G92 B0 ; RESET DISPENSER AXIS POSITION");
        ui->txtGCode->appendPlainText("G1 B1 F120 ; DEPOSIT 1 SECTION AT EDGE");
        ui->txtGCode->appendPlainText("G92 B0 ; RESET DISPENSER AXIS POSITION");
        ui->txtGCode->appendPlainText(QString("G1 A%1 B%2 F%3 ; EXECUTE DISPENSE MOVE")
                                        .arg(recoaterSettings.disp_end)
                                        .arg(recoaterSettings.disp_sections - 1)
                                        .arg(recoaterSettings.disp_speed));
    }
    else {
        if( recoaterSettings.useTurnsAsDelay )
            ui->txtGCode->appendPlainText(QString("G4 P%1 ; PAUSE FOR %1MS").arg(recoaterSettings.disp_sections));
        ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; EXECUTE DISPENSE MOVE")
                                        .arg(recoaterSettings.disp_end)
                                        .arg(recoaterSettings.disp_speed));
    }
    //float f1 = (recoaterSettings.disp_end - recoaterSettings.disp_start)/(recoaterSettings.disp_speed/60.0f);
    //f1 *= 1000.0f;
    //ui->txtGCode->appendPlainText(QString("G4 P%1 ; WAIT FOR MOVE").arg(f1));
    ui->txtGCode->appendPlainText(QString("M106 P%1 S0; TURN OFF FAN 1 / VAC FOR BOX OVERFLOW").arg(BTT_VAC));
    ui->txtGCode->appendPlainText(QString("M106 P%1 S0; TURN OFF DISPENSER VIBE").arg(BTT_VIBE));
    ui->txtGCode->appendPlainText("M18 B ; TURN OFF DISPENSER MOTOR DRIVE");
    ui->txtGCode->appendPlainText("G4 S1 ; PAUSE 1 SECOND FOR FAN OFF");
    ui->txtGCode->appendPlainText(";:END:DISPENSE.GCODE:");
    ui->txtGCode->appendPlainText(";:START:SMOOTH.GCODE:");
    ui->txtGCode->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 X%1 F%2 ; ENSURE PRINT AXIS HOME").arg(laydownSettings.capX).arg(recoaterSettings.x_move_speed));
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; MOVE TO ROLLING START")
                                  .arg(recoaterSettings.rolling_start)
                                  .arg(recoaterSettings.a_move_speed));
    ui->txtGCode->appendPlainText(QString("M106 P%1 S255; TURN ON FAN 1 = VAC FOR BOX OVERFLOW").arg(BTT_VAC));
    ui->txtGCode->appendPlainText("G4 S1 ; PAUSE 1 SECOND FOR FAN");
    if( !ui->useSingleRollerMotor->isChecked() ){
        ui->txtGCode->appendPlainText("G92 C0 U0 ; RESET ROLLER AXIS POSITION");
        ui->txtGCode->appendPlainText(QString("G1 A%1 C%2 U%3 F%4 ; EXECUTE ROLLING MOVE").arg(recoaterSettings.rolling_end)
                                      .arg(recoaterSettings.roller1_turns).arg(recoaterSettings.roller2_turns)
                                      .arg(recoaterSettings.rolling_speed*1.0));
    }
    else {
        ui->txtGCode->appendPlainText("G92 C0 ; RESET ROLLER AXIS POSITION");
        ui->txtGCode->appendPlainText(QString("G1 A%1 C%2 F%4 ; EXECUTE ROLLING MOVE").arg(recoaterSettings.rolling_end)
                                      .arg(recoaterSettings.roller1_turns)
                                      .arg(recoaterSettings.rolling_speed*1.0));
    }
    //ui->txtGCode->appendPlainText("M107 P1 ; TURN OFF FAN 1");
/*    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; JERK FORWARD")
                                  .arg(recoaterSettings.rolling_end+5.0f)
                                  .arg(recoaterSettings.a_move_speed));
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; JERK BACKWARD")
                                  .arg(recoaterSettings.rolling_end)
                                  .arg(recoaterSettings.a_move_speed));
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; JERK FORWARD")
                                  .arg(recoaterSettings.rolling_end+5.0f)
                                  .arg(recoaterSettings.a_move_speed));
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; JERK BACKWARD")
                                  .arg(recoaterSettings.rolling_end)
                                  .arg(recoaterSettings.a_move_speed));
    if( !ui->useSingleRollerMotor->isChecked() ){
        ui->txtGCode->appendPlainText(QString("G1 C%1 U%2 F%3 ; SPIN ROLLERS FORWARD")
                                      .arg(recoaterSettings.roller1_turns-3)
                                      .arg(recoaterSettings.roller2_turns-3)
                                      .arg(recoaterSettings.roller1_rpm));
        ui->txtGCode->appendPlainText(QString("G1 C%1 U%2 F%3 ; SPIN ROLLERS BACKWARD")
                                      .arg(recoaterSettings.roller1_turns)
                                      .arg(recoaterSettings.roller2_turns)
                                      .arg(recoaterSettings.roller1_rpm));
    }
    else {
        ui->txtGCode->appendPlainText(QString("G1 C%1 F%2 ; SPIN ROLLER FORWARD")
                                      .arg(recoaterSettings.roller1_turns-3)
                                      .arg(recoaterSettings.roller1_rpm));
        ui->txtGCode->appendPlainText(QString("G1 C%1 F%2 ; SPIN ROLLER BACKWARD")
                                      .arg(recoaterSettings.roller1_turns)
                                      .arg(recoaterSettings.roller1_rpm));
    }
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; JERK FORWARD")
                                  .arg(recoaterSettings.rolling_end+5.0f)
                                  .arg(recoaterSettings.a_move_speed));
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; JERK BACKWARD")
                                  .arg(recoaterSettings.rolling_end)
                                  .arg(recoaterSettings.a_move_speed));
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; JERK FORWARD")
                                  .arg(recoaterSettings.rolling_end+5.0f)
                                  .arg(recoaterSettings.a_move_speed));
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; JERK BACKWARD")
                                  .arg(recoaterSettings.rolling_end)
                                  .arg(recoaterSettings.a_move_speed));*/
    ui->txtGCode->appendPlainText("G91 ; RELATIVE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 Z-0.5 F%1 ; RETRACT Z AXIS").arg(recoaterSettings.z_move_speed));
    ui->txtGCode->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; MOVE BACK TO SMOOTH START")
                                  .arg(recoaterSettings.rolling_start)
                                  .arg(recoaterSettings.a_move_speed));
    ui->txtGCode->appendPlainText("G91 ; RELATIVE POSITION MODE");
    ui->txtGCode->appendPlainText("G1 Z0.5 F120 ; RESTORE Z AXIS");
    ui->txtGCode->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("M106 P%1 S0; TURN OFF FAN 1 = VAC FOR BOX OVERFLOW").arg(BTT_VAC));
    if( !ui->useSingleRollerMotor->isChecked() )
        ui->txtGCode->appendPlainText("M18 C U ; TURN OFF ROLLER STEPPER DRIVES");
    else
        ui->txtGCode->appendPlainText("M18 C ; TURN OFF ROLLER STEPPER DRIVE");
    ui->txtGCode->appendPlainText("G4 P50 ; PAUSE 1 SECOND FOR FAN");
    ui->txtGCode->appendPlainText(QString("G1 A5 F%1 ; MOVE TO LEFT END OF MACHINE").arg(recoaterSettings.a_move_speed));
    ui->txtGCode->appendPlainText(";:END:SMOOTH.GCODE:");
    ui->txtGCode->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtGCode->appendPlainText(";:END:RECOAT.GCODE:");
    ui->txtGCode->appendPlainText(";:START:RECOAT_B2B.GCODE:");
    ui->txtGCode->appendPlainText(";:START:DRY_B2B.GCODE:");
    ui->txtGCode->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 X%1 F%2 ; ENSURE PRINT AXIS HOME").arg(laydownSettings.capX).arg(recoaterSettings.x_move_speed));
#ifndef USE_UV
    ui->txtGCode->appendPlainText(QString("M104 S%1 ; SET THE HEATER TEMPERATURE").arg(recoaterSettings.temperature));
    ui->txtGCode->appendPlainText(QString("M106 P%1 S%2 ; START FAN1").arg(BTT_FAN1).arg(255));
    ui->txtGCode->appendPlainText(QString("M106 P%1 S%2 ; START FAN2").arg(BTT_FAN2).arg(255));
    ui->txtGCode->appendPlainText(QString("G4 P1000 ; PAUSE 1S FOR FAN START"));
    ui->txtGCode->appendPlainText(QString("M106 P%1 S%2 ; SET FAN1 FINAL SPEED").arg(BTT_FAN1).arg(recoaterSettings.fan1_pwm));
    ui->txtGCode->appendPlainText(QString("M106 P%1 S%2 ; SET FAN2 FINAL SPEED").arg(BTT_FAN2).arg(recoaterSettings.fan2_pwm));
    ui->txtGCode->appendPlainText(QString("G4 P200 ; PAUSE 200MS FOR FANS"));
    ui->txtGCode->appendPlainText("G91 ; RELATIVE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 U%1 F%2").arg(-1.0f*recoaterSettings.z_retract).arg(recoaterSettings.z_move_speed));
    ui->txtGCode->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; MOVE TO HEAT START")
                                  .arg(recoaterSettings.heat_start)
                                  .arg(recoaterSettings.a_move_speed));
#elif USE_UV
    ui->txtGCode->appendPlainText("G91 ; RELATIVE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 U%1 F%2").arg(-1.0f*recoaterSettings.z_retract).arg(recoaterSettings.z_move_speed));
    ui->txtGCode->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; MOVE TO HEAT START")
                                  .arg(recoaterSettings.heat_start)
                                  .arg(recoaterSettings.a_move_speed));
    ui->txtGCode->appendPlainText(QString("M106 P%1 S255 ; START UV LAMP").arg(BTT_UV));
#endif

    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; EXECUTE DRY/CURE MOVE").arg(recoaterSettings.heat_end).arg(recoaterSettings.heat_speed));

#ifndef USE_UV
    ui->txtGCode->appendPlainText(QString("M107 P%1 ; STOP FAN 1").arg(BTT_FAN1));
    ui->txtGCode->appendPlainText(QString("M107 P%1 ; STOP FAN 2").arg(BTT_FAN2));
    ui->txtGCode->appendPlainText("G4 P50 ; PAUSE 50MS FOR FAN STOP");
#elif USE_UV
    ui->txtGCode->appendPlainText(QString("M107 P%1 ; STOP UV LAMP").arg(BTT_UV));
#endif

    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2").arg(recoaterSettings.rolling_start).arg(recoaterSettings.a_move_speed));
    ui->txtGCode->appendPlainText("G91 ; RELATIVE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 U%1 F%2").arg(recoaterSettings.z_retract).arg(recoaterSettings.z_move_speed));
    ui->txtGCode->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtGCode->appendPlainText(";:END:DRY_B2B.GCODE:");
    ui->txtGCode->appendPlainText(";:START:SMOOTH_B2B.GCODE:");
    ui->txtGCode->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2").arg(recoaterSettings.rolling_start).arg(recoaterSettings.a_move_speed));
    // Calculate the # of turns required for fast portion
    float roller_turns = 0.0f;
    ui->txtGCode->appendPlainText("G92 C0 ; RESET ROLLER POSITION");
    roller_turns = recoaterSettings.roller1_rpm * (abs(recoaterSettings.rolling_midpoint - recoaterSettings.rolling_start)/recoaterSettings.rolling_fast_speed);
    ui->txtGCode->appendPlainText(QString("G1 A%1 C%2 F%3").arg(recoaterSettings.rolling_midpoint).arg(roller_turns).arg(recoaterSettings.rolling_fast_speed));
    roller_turns += recoaterSettings.roller1_rpm * (abs(recoaterSettings.rolling_midpoint - recoaterSettings.rolling_end)/recoaterSettings.rolling_speed);
    ui->txtGCode->appendPlainText(QString("G1 A%1 C%2 F%3").arg(recoaterSettings.rolling_end).arg(roller_turns).arg(recoaterSettings.rolling_speed));
    ui->txtGCode->appendPlainText("G4 P250 ; PAUSE 250MS");
    ui->txtGCode->appendPlainText("G91 ; RELATIVE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 Z%1 U%1 F%2").arg(-1.0f*recoaterSettings.z_retract).arg(recoaterSettings.z_move_speed));
    ui->txtGCode->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; RETURN TO START OF ROLL").arg(recoaterSettings.rolling_start).arg(recoaterSettings.a_move_speed));
    ui->txtGCode->appendPlainText("G91 ; RELATIVE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 Z%1 U%1 F%2").arg(recoaterSettings.z_retract).arg(recoaterSettings.z_move_speed));
    ui->txtGCode->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 A1 F%1").arg(recoaterSettings.a_move_speed));
    ui->txtGCode->appendPlainText(";:END:SMOOTH_B2B.GCODE:");
    ui->txtGCode->appendPlainText(";:END:RECOAT_B2B.GCODE:");

    // Tamp routine At start, build must be down by 2, feed up by 2
    ui->txtGCode->appendPlainText(";:START:SMOOTH_TAMP_B2B.GCODE:");
    ui->txtGCode->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2").arg(recoaterSettings.rolling_start).arg(recoaterSettings.a_move_speed));
    ui->txtGCode->appendPlainText("G92 B0 C0 ; RESET ROLLER POSITION");
    roller_turns = recoaterSettings.roller1_rpm * (abs(recoaterSettings.rolling_midpoint - recoaterSettings.rolling_start)/recoaterSettings.rolling_fast_speed);
    ui->txtGCode->appendPlainText(QString("G1 A%1 C%2 F%3").arg(recoaterSettings.rolling_midpoint).arg(roller_turns).arg(recoaterSettings.rolling_fast_speed));
    roller_turns += recoaterSettings.roller1_rpm * (abs(recoaterSettings.rolling_midpoint - recoaterSettings.rolling_end)/recoaterSettings.rolling_speed);
    ui->txtGCode->appendPlainText(QString("G1 A%1 C%2 F%3").arg(recoaterSettings.rolling_end).arg(roller_turns).arg(recoaterSettings.rolling_speed));
    ui->txtGCode->appendPlainText("G4 P250 ; PAUSE 250MS");
    ui->txtGCode->appendPlainText("G91 ; RELATIVE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 Z%1 U%1 F%2").arg(-1.0f*recoaterSettings.z_retract).arg(recoaterSettings.z_move_speed));
    ui->txtGCode->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; RETURN TO START OF ROLL").arg(recoaterSettings.rolling_start).arg(recoaterSettings.a_move_speed));
    ui->txtGCode->appendPlainText("G91 ; RELATIVE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 Z%1 F%2 ; BUMP UP FOR TAMPING").arg(recoaterSettings.z_retract + laydownSettings.layerThickness).arg(recoaterSettings.z_move_speed));
    ui->txtGCode->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2 ; MOVE TO TAMP START").arg(recoaterSettings.disp_start).arg(recoaterSettings.a_move_speed));
    roller_turns = recoaterSettings.roller1_rpm * (abs(recoaterSettings.disp_end - recoaterSettings.disp_start)/recoaterSettings.disp_speed);
    ui->txtGCode->appendPlainText(QString("G1 A%1 C%2 B%3 F%4")
                                  .arg(recoaterSettings.disp_end)
                                  .arg(roller_turns)
                                  .arg(recoaterSettings.disp_sections)
                                  .arg(recoaterSettings.disp_speed));
    roller_turns = recoaterSettings.roller1_rpm * (abs(recoaterSettings.rolling_end - recoaterSettings.disp_end)/recoaterSettings.rolling_speed);
    ui->txtGCode->appendPlainText(QString("G1 A%1 C%2 F%3")
                                  .arg(recoaterSettings.rolling_end)
                                  .arg(roller_turns)
                                  .arg(recoaterSettings.rolling_speed));
    ui->txtGCode->appendPlainText("G91 ; RELATIVE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 Z%1 F%2 ; RETRACT").arg(-1.0*recoaterSettings.z_retract).arg(recoaterSettings.z_move_speed));
    ui->txtGCode->appendPlainText("G90 ; ABS POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 A%1 F%2").arg(recoaterSettings.rolling_start).arg(recoaterSettings.a_move_speed));
    ui->txtGCode->appendPlainText("G91 ; RELATIVE POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 Z%1 U%1 F%2 ; RESTORE").arg(recoaterSettings.z_retract).arg(recoaterSettings.z_move_speed));
    ui->txtGCode->appendPlainText("G90 ; ABS POSITION MODE");
    ui->txtGCode->appendPlainText(QString("G1 A1 F%1").arg(recoaterSettings.a_move_speed));
    ui->txtGCode->appendPlainText(";:END:SMOOTH_TAMP_B2B.GCODE:");


    return;
}


void MainWindow::on_cmdSaveRecoatGCodeAs_clicked()
{
    // The recoating files will be exported up to two at a time. Meaning, the parser will accept up to one nest
    // The command list MUST start with ;:START:FILENAME.GCODE:
    this->saveGCodeFiles(ui->txtGCode);

}


void MainWindow::on_txtCNCDirect_returnPressed()
{
    this->on_goSendDirect_clicked();
    ui->txtCNCDirect->clear();
}

void MainWindow::on_btnRunPause_clicked()
{
    static bool b_isRunning = false;
    bool ok = false;
    if( !b_isRunning ){
        // Attach the needed command lists
        QString il = ui->txtInitScript->toPlainText();
        QStringList lil = il.split('\n');
        m_executiveEngine.attachInitializeList(lil);

        il = ui->txtFinalizeScript->toPlainText();
        lil = il.split('\n');
        m_executiveEngine.attachFinalizeList(lil);

        il = ui->txtLayerPipeline->toPlainText();
        lil = il.split('\n');
        m_executiveEngine.attachCommandList(lil);

        il = ui->txtLayerExceptionPipeline->toPlainText();
        lil = il.split('\n');
        m_executiveEngine.attachExceptionCommandList(lil);

        bool ok=false; uint32_t eFreq=0;
        eFreq = ui->txtExceptionFrequency->text().toInt(&ok);
        if( ok ) m_executiveEngine.setExceptionFrequency(eFreq);

    }

    if( !b_isRunning ){
        ui->btnRunPause->setText("Pause");
        m_tempTimer->stop();
        QString lc = QString("%1").arg(m_executiveEngine.currentLayer());
        ui->lblCurrentLayer->setText(lc);
        b_isRunning = true;
        QApplication::processEvents();
        ok = m_executiveEngine.run();
        b_isRunning = false;
        ui->btnRunPause->setText("Run");
        m_executiveEngine.unpause();
        //QApplication::processEvents();
    }
    else{
        m_executiveEngine.pause();
        m_tempTimer->start(5000);
        //b_isRunning = false;
    }
    return;
}


void MainWindow::on_goWipeStartX_clicked()
{
    if( this->readLaydownPage())
        m_CNCComPort.sendGCodeCommand(QString("G1 X%1 F%2").arg(laydownSettings.wipeStartX).arg(recoaterSettings.x_move_speed));
}


void MainWindow::on_attachCommands_clicked()
{
    // Attach the current command list to the executive
    QString pt = ui->txtLayerPipeline->toPlainText();
    QStringList ptl = pt.split('\n');
    m_executiveEngine.attachCommandList(ptl);

    pt = ui->txtLayerExceptionPipeline->toPlainText();
    ptl = pt.split('\n');
    m_executiveEngine.attachExceptionCommandList(ptl);

    bool ok=false; uint32_t eFreq=0;
    eFreq = ui->txtExceptionFrequency->text().toInt(&ok);
    if( ok ) m_executiveEngine.setExceptionFrequency(eFreq);
}


void MainWindow::on_setImageStackSource_clicked()
{
    if( stl_window->stlFiles.size() == 0)
    {
        QMessageBox::information(this,"Pick STL", "Please load an STL file first");
        return;
    }
    // Get the source path for the images
    QString filePath = QFileDialog::getExistingDirectory(this, "Select Source Folder", m_curImagePath);
    if( filePath == "" ) return;
    m_executiveEngine.setImageStackPath(filePath, stl_window->stlFiles.size());
    m_curImagePath = filePath;
    return;
}


void MainWindow::on_lockZeroZ_clicked()
{
    // Set the Z
    CNCAxisPositions ap;
    bool ok = true;
    if( this->m_CNCComPort.getAxisPositions(&ap) ){
        for( int i = 0; i<stl_window->stlFiles.size(); i++){
            if( ap.z - stl_window->stlFiles[i].maxZ < 0.0f ) ok = false;
        }
        if( ok ){ m_executiveEngine.setLayerZeroZ( ap.z ); return; }
        else QMessageBox::information(this, "No Space in Box", "The Z-Axis is too low\nto print the selected files");
    }
    else QMessageBox::information(this, "Comm Fail", "The Z Position could not be read.\nRestart Comms");

}
void MainWindow::refreshFileLists(){
    ui->lblGCodeSourceDirectory->setText(m_curGcodePath);
    ui->lblInitFiles->setText(QString("Files from: %1").arg(m_curGcodePath));
    QDir directory(m_curGcodePath);
    m_executiveEngine.setGCodePath(m_curGcodePath);
    QStringList gcodeFiles = directory.entryList(QStringList() << "*.GCODE", QDir::Files);
    ui->gcodeList->clear();
    ui->txtInitFilesAvailable->clear();
    foreach(QString filename, gcodeFiles) {
        QFileInfo fileInfo(directory.absoluteFilePath(filename));
        ui->gcodeList->addItem(fileInfo.fileName());
        ui->txtInitFilesAvailable->appendPlainText(fileInfo.fileName());
    }

}

void MainWindow::on_chooseGCODESource_clicked()
{
    // Pick a path for the GCODE List
    QString filePath = QFileDialog::getExistingDirectory(this, "Select Source Directory", m_curGcodePath);
    if( filePath == "" ) return;
    m_curGcodePath = filePath;
    this->refreshFileLists();


}


void MainWindow::on_goWipeEndX_clicked()
{
    if( this->readLaydownPage())
        m_CNCComPort.sendGCodeCommand(QString("G1 X%1 F%2").arg(laydownSettings.wipeEndX).arg(recoaterSettings.x_move_speed));
}


void MainWindow::on_tryDispensePower_clicked()
{
    if( this->readRecoatingPage() )
    {
        m_CNCComPort.sendGCodeCommand(QString("M106 P%1 S%2").arg(BTT_VIBE).arg(recoaterSettings.disp_power));
        m_CNCComPort.sendGCodeCommand("G4 S2");
        m_CNCComPort.sendGCodeCommand(QString("M107 P%1").arg(BTT_VIBE));
    }
    //
}


void MainWindow::on_makeTendingFunctions_clicked()
{
    // Make tending functions for the printhead
    if( !this->readLaydownPage() ) return;

    ui->txtHeadRoutines->clear();
    // Home Z
    ui->txtHeadRoutines->appendPlainText(";:START:HOMEZ.GCODE:");
    ui->txtHeadRoutines->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtHeadRoutines->appendPlainText(QString("M914 Z%1").arg(recoaterSettings.z_home_sense));
    ui->txtHeadRoutines->appendPlainText("G28 Z ; HOME Z AXIS");
    ui->txtHeadRoutines->appendPlainText("G4 P500 ; WAIT 500MS");
    ui->txtHeadRoutines->appendPlainText(QString("M914 Z%1").arg(recoaterSettings.z_home_sense));
    ui->txtHeadRoutines->appendPlainText("G28 Z ; HOME Z AXIS");
    ui->txtHeadRoutines->appendPlainText("G4 P500 ; WAIT 500MS");
    ui->txtHeadRoutines->appendPlainText(";:END:HOMEZ.GCODE");

    // Home U
    ui->txtHeadRoutines->appendPlainText(";:START:HOMEU.GCODE:");
    ui->txtHeadRoutines->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtHeadRoutines->appendPlainText(QString("M914 U%1").arg(recoaterSettings.z_home_sense));
    ui->txtHeadRoutines->appendPlainText("G28 U ; HOME Z AXIS");
    ui->txtHeadRoutines->appendPlainText("G4 P500 ; WAIT 500MS");
    ui->txtHeadRoutines->appendPlainText(QString("M914 U%1").arg(recoaterSettings.z_home_sense));
    ui->txtHeadRoutines->appendPlainText("G28 U ; HOME U AXIS");
    ui->txtHeadRoutines->appendPlainText("G4 P500 ; WAIT 500MS");
    ui->txtHeadRoutines->appendPlainText(";:END:HOMEU.GCODE");

    // Home A
    ui->txtHeadRoutines->appendPlainText(";:START:HOMEA.GCODE:");
    ui->txtHeadRoutines->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtHeadRoutines->appendPlainText(QString("M914 A%1").arg(recoaterSettings.a_home_sense));
    ui->txtHeadRoutines->appendPlainText("G28 A ; HOME A (recoater) AXIS");
    ui->txtHeadRoutines->appendPlainText("G4 P500 ; WAIT 500MS");
    ui->txtHeadRoutines->appendPlainText("G1 A0.4 F600");
    ui->txtHeadRoutines->appendPlainText("G4 P500 ; WAIT 500MS");
    ui->txtHeadRoutines->appendPlainText("G92 A0 ; SET A to ZERO");
    ui->txtHeadRoutines->appendPlainText(";:END:HOMEA.GCODE");

    // Make HOMEXY
    ui->txtHeadRoutines->appendPlainText(";:START:HOMEXY.GCODE:");
    ui->txtHeadRoutines->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtHeadRoutines->appendPlainText(QString("M914 X%1").arg(recoaterSettings.x_home_sense));
    ui->txtHeadRoutines->appendPlainText("G28 X ; HOME X AXIS");
    ui->txtHeadRoutines->appendPlainText("G4 P500 ; WAIT 500MS");
    ui->txtHeadRoutines->appendPlainText("G1 X0.4 F600 ; TAKE UP X BACKLASH");
    ui->txtHeadRoutines->appendPlainText("G4 P500 ; WAIT 500MS");
    ui->txtHeadRoutines->appendPlainText("G92 X0 ; RESET X ZERO POSITION");
    ui->txtHeadRoutines->appendPlainText(QString("M914 Y%1").arg(recoaterSettings.y_home_sense));
    ui->txtHeadRoutines->appendPlainText("G28 Y ; HOME Y AXIS");
    ui->txtHeadRoutines->appendPlainText("G4 P500 ; WAIT 500MS");
    ui->txtHeadRoutines->appendPlainText(QString("G1 Y%1 F%2 ; TAKE UP Y HOME OFFSET")
                                         .arg(laydownSettings.yHomeOffset)
                                         .arg(recoaterSettings.y_move_speed));
    float f1 = laydownSettings.yAxisBacklash/(recoaterSettings.y_move_speed/60.0f);
    f1 *= 1000.0f;
    ui->txtHeadRoutines->appendPlainText(QString("G4 P%1 ; WAIT FOR MOVE").arg((int)f1));
    ui->txtHeadRoutines->appendPlainText("G92 Y0 ; DEFINE THIS AS Y=0");
    ui->txtHeadRoutines->appendPlainText("G4 P100 ; WAIT 100MS");
    ui->txtHeadRoutines->appendPlainText(QString("G1 Y%1 F%2 ; TAKE UP Y BACKLASH")
                                         .arg(laydownSettings.yAxisBacklash)
                                         .arg(recoaterSettings.y_move_speed));
    ui->txtHeadRoutines->appendPlainText(QString("G4 P%1 ; WAIT FOR MOVE").arg((int)f1));
    ui->txtHeadRoutines->appendPlainText(QString("G1 Y0 F%1").arg(recoaterSettings.y_move_speed));
    ui->txtHeadRoutines->appendPlainText(QString("G4 P%1 ; WAIT FOR MOVE").arg((int)f1));
//    ui->txtHeadRoutines->appendPlainText(QString("G1 X%1 F%2 ; MOVE TO CAP SAFE X POSITION")
//                                         .arg(laydownSettings.capSafeX)
//                                         .arg(recoaterSettings.x_move_speed));
//    ui->txtHeadRoutines->appendPlainText(QString("G1 Y%1 F120 ; MOVE TO CAP Y POSITION").arg(laydownSettings.capY));
    ui->txtHeadRoutines->appendPlainText(QString("G1 X%1 F%2 ; MOVE TO CAP X POSITION")
                                         .arg(laydownSettings.capX)
                                         .arg(recoaterSettings.x_move_speed/2.0f));
    ui->txtHeadRoutines->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtHeadRoutines->appendPlainText(";:END:HOMEXY.GCODE:");

    // Make a CAPHEAD function
    ui->txtHeadRoutines->appendPlainText(";:START:CAPHEAD.GCODE:");
    ui->txtHeadRoutines->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtHeadRoutines->appendPlainText(QString("G1 X%1 F%2 ; MOVE TO X CAP SAFE")
                                         .arg(laydownSettings.capSafeX)
                                         .arg(recoaterSettings.x_move_speed));
    ui->txtHeadRoutines->appendPlainText(QString("G1 X%1 F%2 ; MOVE TO X CAP POSITION")
                                         .arg(laydownSettings.capX)
                                         .arg(recoaterSettings.x_move_speed));
    ui->txtHeadRoutines->appendPlainText(";:END:CAPHEAD.GCODE:");

    // PRIMING FUNCTIONS
    // Make various priming functions
    // PRIMEWIPEFIRE
    ui->txtHeadRoutines->appendPlainText(";:START:PRIMEWIPEFIRE.GCODE:");
    ui->txtHeadRoutines->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtHeadRoutines->appendPlainText(QString("G1 X%1 F%2 ; MOVE TO PRIME POSITION")
                                         .arg(laydownSettings.primeX)
                                         .arg(recoaterSettings.x_move_speed));
    ui->txtHeadRoutines->appendPlainText("G4 P250 ; WAIT 250MS");
#ifndef UTK
    ui->txtHeadRoutines->appendPlainText("G28 V ; FILL RESERVOIR");
#else
    if( !ui->use2binders->isChecked() )
        ui->txtHeadRoutines->appendPlainText("G28 U ; FILL RESERVOIR");
    else
        ui->txtHeadRoutines->appendPlainText("G28 V U ; FILL RESERVOIR");
#endif
    ui->txtHeadRoutines->appendPlainText(QString("M106 P%1 S255 ; ENABLE PRIMER").arg(BTT_PUMP));
    ui->txtHeadRoutines->appendPlainText(QString("G4 P%1 ; PRIME FOR %1 MS").arg(laydownSettings.primeTime));
    ui->txtHeadRoutines->appendPlainText(QString("M106 P%1 S0 ; DISABLE PRIMER").arg(BTT_PUMP));
    ui->txtHeadRoutines->appendPlainText(QString("G4 P%1 ; DRIP FOR %1 MS").arg(laydownSettings.dripTime));
#ifndef UTK
    ui->txtHeadRoutines->appendPlainText("G28 V ; FILL RESERVOIR");
#else
    if( !ui->use2binders->isChecked() )
        ui->txtHeadRoutines->appendPlainText("G28 U ; FILL RESERVOIR");
    else
        ui->txtHeadRoutines->appendPlainText("G28 V U ; FILL RESERVOIR");
#endif
    ui->txtHeadRoutines->appendPlainText(QString("G1 X%1 F%2 ; MOVE TO END WIPE X")
                                         .arg(laydownSettings.wipeEndX)
                                         .arg(recoaterSettings.x_move_speed));
    ui->txtHeadRoutines->appendPlainText(QString("G1 Y%1 F120 ; MOVE TO WIPE Y").arg(laydownSettings.wipeY));
    ui->txtHeadRoutines->appendPlainText(QString("G1 X%1 F%2 ; MOVE TO START WIPE X").arg(laydownSettings.wipeStartX).arg(laydownSettings.wipeSpeed));
    ui->txtHeadRoutines->appendPlainText("G4 P100 ; WAIT 100MS");
    ui->txtHeadRoutines->appendPlainText(QString("G1 X%1 F%2 ; MOVE TO END WIPE X").arg(laydownSettings.wipeEndX).arg(laydownSettings.wipeSpeed));
    ui->txtHeadRoutines->appendPlainText(QString("G1 X%1 F%2 ; MOVE TO PRIME POSITION")
                                         .arg(laydownSettings.primeX)
                                         .arg(recoaterSettings.x_move_speed));
    f1 = (abs(laydownSettings.primeX - laydownSettings.wipeEndX)) / (recoaterSettings.x_move_speed/60.0f);
    f1 *= 1000.0f;
    f1 *= 1.1f; // Add a tiny bit of extra time
    uint32_t pause = (int)f1;
    ui->txtHeadRoutines->appendPlainText(QString("G4 P%1 ; PAUSE FOR MOVE TO COMPLETE").arg(pause));
    ui->txtHeadRoutines->appendPlainText(";:PHC:VE,1");
    ui->txtHeadRoutines->appendPlainText(QString(";:PHC:SF,%1").arg(laydownSettings.printFrequency));
    ui->txtHeadRoutines->appendPlainText(";:PHC:FA,1000");
    f1 = 1000000.0f / laydownSettings.printFrequency;
    pause = (int)f1;
    ui->txtHeadRoutines->appendPlainText(QString("G4 P%1 ; WAIT %1MS").arg(pause));
    ui->txtHeadRoutines->appendPlainText(QString("G1 X%1 F%2 ; MOVE TO CAP SAFE X POSITION")
                                         .arg(laydownSettings.capSafeX)
                                         .arg(recoaterSettings.x_move_speed));
    ui->txtHeadRoutines->appendPlainText(";:END:PRIMEWIPEFIRE.GCODE:");

 // PRIME
    ui->txtHeadRoutines->appendPlainText(";:START:PRIME.GCODE:");
    ui->txtHeadRoutines->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtHeadRoutines->appendPlainText(QString("G1 X%1 F%2 ; MOVE TO PRIME POSITION")
                                         .arg(laydownSettings.primeX)
                                         .arg(recoaterSettings.x_move_speed));
#ifndef UTK
    ui->txtHeadRoutines->appendPlainText("G28 V ; FILL RESERVOIR");
#else
    if( !ui->use2binders->isChecked() )
        ui->txtHeadRoutines->appendPlainText("G28 U ; FILL RESERVOIR");
    else
        ui->txtHeadRoutines->appendPlainText("G28 V U ; FILL RESERVOIR");
#endif
    ui->txtHeadRoutines->appendPlainText("G4 P250 ; WAIT 250MS");
    ui->txtHeadRoutines->appendPlainText(QString("M106 P%1 S255 ; ENABLE PRIMER").arg(BTT_PUMP));
    ui->txtHeadRoutines->appendPlainText(QString("G4 P%1 ; PRIME FOR %1 MS").arg(laydownSettings.primeTime));
    ui->txtHeadRoutines->appendPlainText(QString("M106 P%1 S0 ; DISABLE PRIMER").arg(BTT_PUMP));
    ui->txtHeadRoutines->appendPlainText(QString("G4 P%1 ; DRIP FOR %1 MS").arg(laydownSettings.dripTime));
#ifndef UTK
    ui->txtHeadRoutines->appendPlainText("G28 V ; FILL RESERVOIR");
#else
    if( !ui->use2binders->isChecked() )
        ui->txtHeadRoutines->appendPlainText("G28 U ; FILL RESERVOIR");
    else
        ui->txtHeadRoutines->appendPlainText("G28 V U ; FILL RESERVOIR");
#endif
    ui->txtHeadRoutines->appendPlainText(QString("G1 X%1 F%2 ; MOVE TO CAP SAFE X POSITION")
                                         .arg(laydownSettings.capSafeX)
                                         .arg(recoaterSettings.x_move_speed));
    ui->txtHeadRoutines->appendPlainText("G4 P250 ; WAIT 250MS");
    ui->txtHeadRoutines->appendPlainText(";:END:PRIME.GCODE:");

// WIPE
    ui->txtHeadRoutines->appendPlainText(";:START:WIPE.GCODE:");
    ui->txtHeadRoutines->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtHeadRoutines->appendPlainText(QString("G1 X%1 F%2 ; MOVE TO START WIPE X")
                                         .arg(laydownSettings.wipeStartX)
                                         .arg(recoaterSettings.x_move_speed));
    ui->txtHeadRoutines->appendPlainText(QString("G1 X%1 F%2 ; MOVE TO END WIPE X")
                                         .arg(laydownSettings.wipeEndX)
                                         .arg(laydownSettings.wipeSpeed));
    ui->txtHeadRoutines->appendPlainText("G4 P100 ; WAIT 100MS");
    ui->txtHeadRoutines->appendPlainText(QString("G1 X%1 F%2 ; MOVE TO START WIPE X")
                                         .arg(laydownSettings.wipeStartX)
                                         .arg(laydownSettings.wipeSpeed));
    ui->txtHeadRoutines->appendPlainText("G4 P100 ; WAIT 100MS");
    ui->txtHeadRoutines->appendPlainText(QString("G1 X%1 F%2 ; MOVE TO END WIPE X")
                                         .arg(laydownSettings.wipeEndX)
                                         .arg(laydownSettings.wipeSpeed));
    ui->txtHeadRoutines->appendPlainText(QString("G1 X%1 F%2 ; MOVE TO CAP SAFE X POSITION")
                                         .arg(laydownSettings.capSafeX)
                                         .arg(recoaterSettings.x_move_speed));
    ui->txtHeadRoutines->appendPlainText(";:END:WIPE.GCODE:");
    ui->txtHeadRoutines->appendPlainText(";:START:GETTEMP.GCODE:");
    ui->txtHeadRoutines->appendPlainText("G90 ; ABSOLUTE POSITION MODE");
    ui->txtHeadRoutines->appendPlainText(QString("G1 A5 F%1").arg(recoaterSettings.a_move_speed));
    ui->txtHeadRoutines->appendPlainText(QString("G1 X%1 F%2")
                                         .arg(laydownSettings.tempCheckX)
                                         .arg(recoaterSettings.x_move_speed));
    ui->txtHeadRoutines->appendPlainText(QString("G1 Y%1 F%2")
                                         .arg(laydownSettings.tempCheckY + laydownSettings.yAxisBacklash)
                                         .arg(recoaterSettings.y_move_speed));
    ui->txtHeadRoutines->appendPlainText(QString("G1 Y%1 F%2")
                                         .arg(laydownSettings.tempCheckY)
                                         .arg(recoaterSettings.y_move_speed));
    ui->txtHeadRoutines->appendPlainText("G4 P1500 ; ALLOW SENSOR TO SETTLE");
    ui->txtHeadRoutines->appendPlainText(";:PHC:RT:");
    ui->txtHeadRoutines->appendPlainText(QString("G1 X%1 F%2")
                                         .arg(laydownSettings.capSafeX)
                                         .arg(recoaterSettings.x_move_speed));
    ui->txtHeadRoutines->appendPlainText(";:END:GETTEMP.GCODE:");


}

void MainWindow::on_saveTendingFunctions_clicked()
{
    //
    this->saveGCodeFiles(ui->txtHeadRoutines);

}


void MainWindow::on_tryPrimeRoutine_clicked()
{
    if(this->readLaydownPage()){
        if( !m_CNCComPort.sendGCodeCommand("G1 A5 F6000\n")) return;
        if( ui->txtHeadRoutines->toPlainText().length() == 0)
            this->on_makeTendingFunctions_clicked();
        QApplication::processEvents();
        QString prime = ui->txtHeadRoutines->toPlainText();
        int prime_start = prime.indexOf(";:START:PRIME.GCODE:\n") + QString(";:START:PRIME.GCODE:\n").length();
        int prime_end = prime.indexOf(";:END:PRIME.GCODE:\n",prime_start);
        QString prime_cmds = prime.mid(prime_start, prime_end - prime_start);
        //qDebug() << disp_cmds;
        QStringList prime_list = prime_cmds.split("\n");
        for( int i=0; i<prime_list.length(); i++ ){
            if (!m_CNCComPort.sendGCodeCommand(prime_list[i])) return;
        }
    }
}


void MainWindow::on_gotoCapX_clicked()
{
    if( this->readLaydownPage())
        m_CNCComPort.sendGCodeCommand(QString("G1 X%1 F%2").arg(laydownSettings.capX).arg(recoaterSettings.x_move_speed));

}


void MainWindow::on_gotoCapY_clicked()
{
    if( this->readLaydownPage())
        m_CNCComPort.sendGCodeCommand(QString("G1 Y%1 F%2").arg(laydownSettings.capY).arg(recoaterSettings.y_move_speed));

}


void MainWindow::on_gotoCapSafeX_clicked()
{
    if( this->readLaydownPage())
        m_CNCComPort.sendGCodeCommand(QString("G1 X%1 F%2").arg(laydownSettings.capSafeX).arg(recoaterSettings.x_move_speed));

}


void MainWindow::on_saveScripts_clicked()
{
    // Save a printing script
    QString fileName = QFileDialog::getSaveFileName(this, "Select Script Filename", QString(DEFAULT_PATH), "*.SCODE");
    if( fileName.length() == 0) return;

    QString cmds = ui->txtLayerPipeline->toPlainText();
    QFile fileout(fileName);
    fileout.open(QIODevice::WriteOnly);
    QTextStream out(&fileout);
    out << cmds;
    fileout.close();


}


void MainWindow::on_loadScript_clicked()
{
    // Save a printing script
    QString fileName = QFileDialog::getOpenFileName(this, "Select Script", QString(DEFAULT_PATH), "*.SCODE");
    if( fileName.length() == 0 ) return;
    ui->txtLayerPipeline->clear();
    QFile filein(fileName);
    filein.open(QIODevice::ReadOnly);
    QTextStream in(&filein);
    QString cmds = in.readAll();
    ui->txtLayerPipeline->appendPlainText(cmds);

}


void MainWindow::on_updateLaydownINI_clicked()
{
    // Update the INI File
    if( !this->readLaydownPage() ){
        //qDebug() << "Invalid Recoating Page";
        return;
    }
    QString fileNameR = QFileDialog::getSaveFileName(this, "Select File Name", m_curLaydownIniPath, "ini Files (*.ini)");
    if( fileNameR == "") return;

    LaydownSettings::writeFile(fileNameR, &this->laydownSettings);
    QFileInfo fi(fileNameR);
    m_curLaydownIniPath = fi.absoluteFilePath();

    m_curMaterial.MaterialINI = fileNameR;
    ui->curMaterialFile->setText("Material File: " + m_curMaterial.MaterialINI);
    ui->currentLaydownINI->setText("Laydown File: " + m_curMaterial.MaterialINI);
    ui->currentRecoatINI->setText("Recoat File: " + m_curMaterial.MaterialINI);
    m_curMaterial.saveName(QDir::homePath() + "/printjobs/lastMat.ini");

}


void MainWindow::on_restartComs_clicked()
{
    //  Restart comm ports
    m_CNCComPort.close();
    m_CNCComPort.findPort();
    m_PHCCommPort.close();
    m_PHCCommPort.findPort();

}


void MainWindow::on_runInitializeScript_clicked()
{
    m_executiveEngine.attachInitializeList(ui->txtInitScript->toPlainText().split('\n'));
    m_executiveEngine.runInitializeList();
}


void MainWindow::on_txtPHCDirect_returnPressed()
{
    if( ui->txtPHCDirect->text().length() >= 2){
        m_PHCCommPort.sendText(ui->txtPHCDirect->text().toUpper());
    }
    ui->txtPHCDirect->clear();
}


void MainWindow::on_tryWipeRoutine_clicked()
{
    if(this->readLaydownPage()){
        this->on_makeTendingFunctions_clicked();
        QApplication::processEvents();
        QString prime = ui->txtHeadRoutines->toPlainText();
        int prime_start = prime.indexOf(";:START:WIPE.GCODE:\n") + QString(";:START:WIPE.GCODE:\n").length();
        int prime_end = prime.indexOf(";:END:WIPE.GCODE:\n",prime_start);
        QString prime_cmds = prime.mid(prime_start, prime_end - prime_start);
        //qDebug() << disp_cmds;
        QStringList prime_list = prime_cmds.split("\n");
        for( int i=0; i<prime_list.length(); i++ ){
            m_CNCComPort.sendGCodeCommand(prime_list[i]);
        }
        //m_CNCComPort.sendGCodeCommand("G1 A5 F6000\n");
    }

}


void MainWindow::on_goSendDirectPHC_clicked()
{
    on_txtPHCDirect_returnPressed();
}


void MainWindow::on_goPassEndX_clicked()
{
    if( this->readLaydownPage()){
        QString cmd = QString("G1 X%1 F6000\n").arg(laydownSettings.endPassX);
        m_CNCComPort.sendGCodeCommand("G90\n");
        m_CNCComPort.sendGCodeCommand(cmd);
    }

}


void MainWindow::on_goPassAlignY_clicked()
{
    if( this->readLaydownPage()){
        QString cmd = QString("G1 Y%1 F120\n").arg(laydownSettings.yAlignZero+2.0);
        m_CNCComPort.sendGCodeCommand("G90\n");
        m_CNCComPort.sendGCodeCommand(cmd);
        cmd = QString("G1 Y%1 F120\n").arg(laydownSettings.yAlignZero);
        m_CNCComPort.sendGCodeCommand(QString("G4 P250"));
        m_CNCComPort.sendGCodeCommand(cmd);
    }

}


void MainWindow::on_tryPass_clicked()
{
    if( this->readLaydownPage()){
        QImage img((int)laydownSettings.passCount*(int)laydownSettings.jetCount,(int)laydownSettings.passCount*(int)laydownSettings.jetCount, QImage::Format_MonoLSB );
        img.fill(1);
        m_PHCCommPort.makeData(img);
        m_PHCCommPort.sendData();
        m_PHCCommPort.sendText(QString("VE,1\r\n"));
        m_PHCCommPort.sendText(QString("SF,%1\r\n").arg(laydownSettings.printFrequency));
        m_PHCCommPort.sendText(QString("LP,%1\r\n").arg((img.width()==1024 ? 1 : 0)));
        m_CNCComPort.sendGCodeCommand("G90\n");
        m_CNCComPort.sendGCodeCommand(QString("G1 X%1 F%2\n").arg(laydownSettings.startPassX).arg(recoaterSettings.x_move_speed));
        m_CNCComPort.sendGCodeCommand(QString("G1 Y%1 F180\n").arg(laydownSettings.yAlignZero+2.0));
        m_CNCComPort.sendGCodeCommand("G4 P250\n");
        m_CNCComPort.sendGCodeCommand(QString("G1 Y%1 F120\n").arg(laydownSettings.yAlignZero));
        m_CNCComPort.sendGCodeCommand("G4 P250\n");
        m_PHCCommPort.sendText(QString("EP,0,%1\r\n").arg(img.width()));
        m_CNCComPort.sendGCodeCommand(QString("G1 X%1 F%2\n").arg(laydownSettings.endPassX).arg(laydownSettings.printSpeed));
        m_CNCComPort.sendGCodeCommand("G4 P250\n");
        m_CNCComPort.sendGCodeCommand(QString("G1 X%1 F%2\n").arg(laydownSettings.startPassX).arg(laydownSettings.printSpeed));

    }

}


void MainWindow::on_clearCNCLog_clicked()
{
    ui->txtCNCLog->clear();
}


void MainWindow::on_clearPHCLog_clicked()
{
    ui->txtPHCLog->clear();
}


void MainWindow::on_clearExecLog_clicked()
{
    ui->txtEngineMessages->clear();
}


void MainWindow::on_raiseZToTop_clicked()
{
    m_CNCComPort.sendGCodeCommand("G1 Z63.5 F240");
}


void MainWindow::on_setZPosition_clicked()
{
    CNCAxisPositions pos;
    bool ok = false;
    m_CNCComPort.getAxisPositions(&pos);
    float newZ = ui->txtRaiseZTo->text().toFloat(&ok);
    if( ok ){
        if( newZ < 0.0f ){ newZ = 0.0f; ui->txtRaiseZTo->setText("0.0");}
        if( newZ < pos.z ){ // do backlash move
            if( pos.z > 0.5 )
                m_CNCComPort.sendGCodeCommand(QString("G1 Z%1 F240\n").arg(newZ-0.5f));
            else
                m_CNCComPort.sendGCodeCommand("G1 Z0 F180\n");
            m_CNCComPort.sendGCodeCommand(QString("G1 Z%1 F240\n").arg(newZ));
        }
        else { // simply move up
            if( newZ > 63.5f ){ newZ = 63.5f; ui->txtRaiseZTo->setText("63.5");}
            m_CNCComPort.sendGCodeCommand(QString("G1 Z%1 F240\n").arg(newZ));
        }
    }
    m_CNCComPort.getAxisPositions(&pos);
    ui->lblRecoatCurrentZ->setText(QString("Current Z = %1").arg(pos.z));
}


void MainWindow::on_chkEnableCNCStream_stateChanged(int arg1)
{
    m_CNCComPort.setEnableVLog( ui->chkEnableCNCStream->isChecked() );
}


void MainWindow::on_chkEnablePHCStream_stateChanged(int arg1)
{
    m_PHCCommPort.setEnableVlog( ui->chkEnablePHCStream->isChecked() );
}


void MainWindow::on_lowerZToBottom_clicked()
{
    m_CNCComPort.sendGCodeCommand(QString("G1 Z0 F%1\n").arg(recoaterSettings.z_move_speed));
}

QImage MainWindow::makeQuadImage()
{
    // This assumes a file was already loaded and drawn at least once.
    QImage bigImage(1024, 1024, QImage::Format_Mono);
    bigImage.fill(0);
    QRgb img_clr;
    uint32_t inCol=0;
    uint32_t inRow=0;
    // Add quad 1, upper left
    stl_window->ortho_top = -32.512f;
    stl_window->ortho_bottom = 0.0f;
    stl_window->ortho_left = -32.512f;
    stl_window->ortho_right = 0.0f;
    stl_window->requestUpdate();
    QApplication::processEvents();
    QImage img = stl_window->grab();

    for( inCol = 0; inCol < 512; inCol ++){
        for( inRow = 0; inRow < 512; inRow ++){
            img_clr = img.pixel(inCol, inRow);
            if( qBlue(img_clr) > 120 || qGreen(img_clr) > 120 || qRed(img_clr) > 120 ){
                bigImage.setPixel(inCol, inRow, 1);
            }
        }
    }
    // Add quad 2, upper left
    stl_window->ortho_top = -32.512f;
    stl_window->ortho_bottom = 0.0f;
    stl_window->ortho_left = 0.0f;
    stl_window->ortho_right = 32.512f;
    stl_window->requestUpdate();
    QApplication::processEvents();
    img = stl_window->grab();

    for( inCol = 0; inCol < 512; inCol ++){
        for( inRow = 0; inRow < 512; inRow ++){
            img_clr = img.pixel(inCol, inRow);
            if( qBlue(img_clr) > 120 || qGreen(img_clr) > 120 || qRed(img_clr) > 120  ){
                bigImage.setPixel(inCol+512, inRow, 1);
            }
        }
    }

    // Add quad 3, lower left
    stl_window->ortho_top = 0.0f;
    stl_window->ortho_bottom = 32.512f;
    stl_window->ortho_left = -32.512f;
    stl_window->ortho_right = 0.0f;
    stl_window->requestUpdate();
    QApplication::processEvents();
    img = stl_window->grab();

    for( inCol = 0; inCol < 512; inCol ++){
        for( inRow = 0; inRow < 512; inRow ++){
            img_clr = img.pixel(inCol, inRow);
            if( qBlue(img_clr) > 120 || qGreen(img_clr) > 120 || qRed(img_clr) > 120 ){
                bigImage.setPixel(inCol, inRow+512, 1);
            }
        }
    }

    // Add quad 4, lower right
    stl_window->ortho_top = 0.0f;
    stl_window->ortho_bottom = 32.512f;
    stl_window->ortho_left = 0.0f;
    stl_window->ortho_right = 32.512f;
    stl_window->requestUpdate();
    QApplication::processEvents();
    img = stl_window->grab();

    for( inCol = 0; inCol < 512; inCol ++){
        for( inRow = 0; inRow < 512; inRow ++){
            img_clr = img.pixel(inCol, inRow);
            if( qBlue(img_clr) > 120 || qGreen(img_clr) > 120 || qRed(img_clr) > 120 ){
                bigImage.setPixel(inCol+512, inRow+512, 1);
            }
        }
    }
    return bigImage;
}

void MainWindow::on_loadRecoatINI_clicked()
{
    // Load a recoater INI file, post it, and generate the G-CODE
    QString fileName = QFileDialog::getOpenFileName(this, "Select Recoat INI file", m_curRecoatIniPath, "ini files (*.ini)");
    if( fileName == "") return;

    recoaterSettings.readFile(fileName, &recoaterSettings);
    this->writeRecoatingPage();
    this->on_cmdGenerateRecoaterGCode_clicked();
    QFileInfo fi(fileName);
    QString nfd = fi.absoluteFilePath();
    m_curRecoatIniPath = nfd;

    m_curMaterial.MaterialINI = fileName;
    m_curMaterial.saveName(QDir::homePath() + "/printjobs/lastMat.ini");
    ui->curMaterialFile->setText("Material File: " + m_curMaterial.MaterialINI);
    ui->currentLaydownINI->setText("Laydown File: " + m_curMaterial.MaterialINI);
    ui->currentRecoatINI->setText("Recoat File: " + m_curMaterial.MaterialINI);

}


void MainWindow::on_loadLaydownINI_clicked()
{
    // Load a laydown INI file, post it, and generate the G-CODE
    QString fileName = QFileDialog::getOpenFileName(this, "Select Laydown INI file", m_curLaydownIniPath, "ini files (*.ini)");
    if( fileName == "") return;

    laydownSettings.readFile(fileName, &laydownSettings);
    this->writeLaydownPage();
    this->on_makeTendingFunctions_clicked();
    QFileInfo fi(fileName);
    m_curLaydownIniPath = fi.absoluteFilePath();

    m_curMaterial.MaterialINI = fileName;
    m_curMaterial.saveName(QDir::homePath() + "/printjobs/lastMat.ini");
    ui->curMaterialFile->setText("Material File: " + m_curMaterial.MaterialINI);
    ui->currentLaydownINI->setText("Laydown File: " + m_curMaterial.MaterialINI);
    ui->currentRecoatINI->setText("Recoat File: " + m_curMaterial.MaterialINI);

}


void MainWindow::on_resetLayer_clicked()
{
    m_executiveEngine.resetLayer();
}


void MainWindow::on_resetJob_clicked()
{
    m_executiveEngine.resetJob();
}

QImage MainWindow::makeNPassImage()
{

    uint32_t bitSize = (int)laydownSettings.jetCount * laydownSettings.passCount;
    const uint32_t maxBits = 512;
    const float orthoMax = 32.512;

    QImage outImage(bitSize, bitSize, QImage::Format_Mono);
    outImage.fill(0);
    QRgb img_clr;
    uint32_t inCol=0;
    uint32_t inRow=0;
    uint32_t endCol=512;
    uint32_t endRow=512;

    float dispWid = (laydownSettings.jetSpacing/laydownSettings.passCount)*512.0f;
    QImage img;

    if( bitSize <= maxBits ){ // Simplest case - final image is smaller, essentially zoom out
        stl_window->ortho_top = -orthoMax;
        stl_window->ortho_bottom = dispWid-orthoMax;
        stl_window->ortho_left = -orthoMax;
        stl_window->ortho_right = dispWid-orthoMax;
        stl_window->requestUpdate();
        QApplication::processEvents();
        img = stl_window->grab();
        for( inCol = 0; inCol < bitSize; inCol ++){
            for( inRow = 0; inRow < bitSize; inRow ++){
                img_clr = img.pixel(inCol, inRow);
                if( qBlue(img_clr) > 120 ){
                    outImage.setPixel(inCol, inRow, 1);
                }
            }
        }
        return outImage;
    }

    // If we get to here, we must build a composite image
    //QTime ch_delay;

    uint32_t panes = ceil( (float)bitSize / (float)maxBits );
    //qDebug() << "Panes = " << panes << " ; bitSize= " << bitSize;
    for( uint32_t panesLong=0 ; panesLong < panes; panesLong++ ){
        for( uint32_t panesWide=0; panesWide < panes; panesWide ++ ){
            stl_window->ortho_top = -orthoMax + ((float)panesLong * dispWid);
            stl_window->ortho_bottom = -orthoMax + (float(panesLong+1) * dispWid );
            stl_window->ortho_left = -orthoMax + ((float)panesWide * dispWid);
            stl_window->ortho_right = -orthoMax + (((float)panesWide+1) * dispWid );
            stl_window->requestUpdate();
            QApplication::processEvents();
            //ch_delay = QTime::currentTime().addMSecs(2000);
            //while( ch_delay > QTime::currentTime() );
            img = stl_window->grab();
            if( (panesWide+1) * maxBits < bitSize ) endCol = maxBits;
            else endCol = bitSize % maxBits;
            if( endCol == 0 ) endCol = 512;
            if( (panesLong+1) * maxBits < bitSize ) endRow = maxBits;
            else endRow = bitSize % maxBits;
            if( endRow == 0 ) endRow = 512;
            for( inRow = 0; inRow < endRow ; inRow ++ ) {
                for( inCol = 0; inCol < endCol; inCol ++ ) {
                    img_clr = img.pixel( inCol, inRow );
                    if( qBlue(img_clr) > 120 || qRed(img_clr) > 120 || qGreen(img_clr) > 120 ){
                        outImage.setPixel( (inCol + (panesWide*maxBits)), (inRow + (panesLong*maxBits)), 1);
                    }
                }
            }
        }
    }
    outImage.save(QString("/home/BJ2/testN.png"));

    return outImage;

}

void MainWindow::on_RIPImageStack_clicked()
{
    makeNPassImage();
    return;
    // Try a 3 pass zoom in quad 1
    uint32_t bitSize = (int)laydownSettings.jetCount * 3;
    const uint32_t maxBits = 512;

    QImage bigImage( bitSize, bitSize, QImage::Format_Mono);
    bigImage.fill(0);
    QRgb img_clr;
    uint32_t inCol=0;
    uint32_t inRow=0;
    float dispWid = (laydownSettings.jetSpacing/3.0f)*512.0f;

    // Add quad 1, upper left
    stl_window->ortho_top = -32.512f;
    stl_window->ortho_bottom = dispWid-32.512f;
    stl_window->ortho_left = -32.512f;
    stl_window->ortho_right = dispWid-32.512f;
    stl_window->requestUpdate();
    QApplication::processEvents();
    //return;
    QImage img = stl_window->grab();
    if( bitSize < maxBits ){
        for( inCol = 0; inCol < bitSize; inCol ++){
            for( inRow = 0; inRow < bitSize; inRow ++){
                img_clr = img.pixel(inCol, inRow);
                if( qBlue(img_clr) > 120 ){
                    bigImage.setPixel(inCol, inRow, 1);
                }
            }
        }
        bigImage.save(QString("/home/BJ2/test3.png"));
        return;
    }
    for( inCol = 0; inCol < 512; inCol ++){
        for( inRow = 0; inRow < 512; inRow ++){
            img_clr = img.pixel(inCol, inRow);
            if( qBlue(img_clr) > 120 ){
                bigImage.setPixel(inCol, inRow, 1);
            }
        }
    }

    // Add quad 2, upper right
    stl_window->ortho_top = -32.512f;
    stl_window->ortho_bottom = dispWid-32.512f;
    stl_window->ortho_left = dispWid-32.512f;
    stl_window->ortho_right = (2.0f*dispWid)-32.512f;
    stl_window->requestUpdate();
    QApplication::processEvents();
    img = stl_window->grab();

    for( inCol = 0; inCol < 256; inCol ++){
        for( inRow = 0; inRow < 512; inRow ++){
            img_clr = img.pixel(inCol, inRow);
            if( qBlue(img_clr) > 120 ){
                bigImage.setPixel(inCol+512, inRow, 1);
            }
        }
    }

    // Add quad 3, lower left
    stl_window->ortho_top = dispWid-32.512f;
    stl_window->ortho_bottom = (2.0f*dispWid)-32.512f;
    stl_window->ortho_left = -32.512f;
    stl_window->ortho_right = dispWid-32.512f;
    stl_window->requestUpdate();
    QApplication::processEvents();
    img = stl_window->grab();

    for( inCol = 0; inCol < 512; inCol ++){
        for( inRow = 0; inRow < 256; inRow ++){
            img_clr = img.pixel(inCol, inRow);
            if( qBlue(img_clr) > 120 ){
                bigImage.setPixel(inCol, inRow+512, 1);
            }
        }
    }

    // Add quad 4, lower right
    stl_window->ortho_top = dispWid-32.512f;
    stl_window->ortho_bottom = (2.0f*dispWid)-32.512f;
    stl_window->ortho_left = dispWid-32.512f;
    stl_window->ortho_right = (2.0f*dispWid)-32.512f;
    stl_window->requestUpdate();
    QApplication::processEvents();
    img = stl_window->grab();

    for( inCol = 0; inCol < 256; inCol ++){
        for( inRow = 0; inRow < 256; inRow ++){
            img_clr = img.pixel(inCol, inRow);
            if( qBlue(img_clr) > 120 ){
                bigImage.setPixel(inCol+512, inRow+512, 1);
            }
        }
    }
    bigImage.save(QString("/home/BJ2/test3.png"));

    stl_window->ortho_top = -32.512f;
    stl_window->ortho_bottom = 32.512f;
    stl_window->ortho_left = -32.512f;
    stl_window->ortho_right = 32.512f;
    stl_window->requestUpdate();
    QApplication::processEvents();

}


void MainWindow::on_rip2ColorImageStack_clicked()
{

    QString filePath = QFileDialog::getExistingDirectory(this, "Select Output Folder",DEFAULT_PATH);

    // Let's see what we get with grab
    QImage img = stl_window->grab();
    QImage ripd_img = QImage(img.size(), QImage::Format_Mono);
    QImage ripd_img2 = QImage( img.size(), QImage::Format_Mono );
    ripd_img.fill(0); // Clear the data
    ripd_img2.fill(0);

    uint32_t layer = ui->sbSliceLayer->value();
    unsigned int inCol;
    unsigned int outCol;
    unsigned int outRow;
    unsigned int passId;
    unsigned int jetId;
    QString fn_raw;
    QRgb img_clr;
//  volatile rgba_stl * rgbastl = (rgba_stl*)&img_clr;
    for( uint32_t stlobjects = 0; stlobjects < stl_window->stlFiles.size();  stlobjects++ )
        stl_window->stlFiles[stlobjects].bindData = false;
    for( uint32_t stlobjects = 0; stlobjects < stl_window->stlFiles.size();  stlobjects++ ){
        stl_window->stlFiles[stlobjects].bindData = true;
        stl_window->requestUpdate();
        QApplication::processEvents();
        img = stl_window->grab();
        for( passId = 0; passId < laydownSettings.passCount; passId++ ){
            for( jetId = 0; jetId < JET_COUNT; jetId ++){
                inCol = (jetId * laydownSettings.passCount ) + passId;
                for( int inRow=0; inRow < img.size().height() ; inRow++){
                    img_clr = img.pixel(inCol, inRow);
                    outCol = floor(jetId/2) + ((inRow%4)*JET_COUNT);
                    if( !(jetId & 1) ) outCol += ODD_OFFSET;
                    outRow = (passId * JET_COUNT)+floor(inRow/laydownSettings.passCount);
                    if( qBlue(img_clr) > 10 ){
                        ripd_img.setPixel(outCol, outRow, 1);
                    }
                    if( qGreen(img_clr) > 10 ){
                        ripd_img2.setPixel(outCol, outRow, 1);
                    }
                    //if( qBlue(img_clr) > 120 && qGreen(img_clr)>120 ) qDebug() << "Two Colors";
                }
            }
        }
        fn_raw = filePath + QString("/layer_%1_color_%2.png").arg(layer).arg(stlobjects);
        img.save(fn_raw);
        stl_window->stlFiles[stlobjects].bindData = false;
    }
    for( uint32_t stlobjects = 0; stlobjects < stl_window->stlFiles.size();  stlobjects++ )
        stl_window->stlFiles[stlobjects].bindData = true;
    stl_window->requestUpdate();
    QApplication::processEvents();
    bool ok;
    img = stl_window->grab();
    fn_raw = filePath + QString("/layer_%1.png").arg(layer);
    ok = img.save(fn_raw);
    QString fn_rip = filePath + QString("/layer_%1RIP_1.png").arg(layer);
    ok &= ripd_img.save(fn_rip);
    fn_rip = filePath + QString("/layer_%1RIP_2.png").arg(layer);
    ripd_img2.save(fn_rip);

    return;

}


void MainWindow::on_gotoPrimeX_clicked()
{
    if( this->readLaydownPage() )
        m_CNCComPort.sendGCodeCommand(QString("G1 X%1 F%2").arg(laydownSettings.primeX).arg(recoaterSettings.x_move_speed));
}


void MainWindow::on_turnsAsOnDelay_stateChanged(int arg1)
{

}


void MainWindow::on_lowerZby1_clicked()
{
    CNCAxisPositions ap;
    this->readLaydownPage();
    m_CNCComPort.getAxisPositions(&ap);
    QString cmd = QString("G1 Z%1 F120\n").arg(ap.z-0.5f);
    m_CNCComPort.sendGCodeCommand(cmd);
    cmd = QString("G1 Z%1 F120\n").arg(ap.z-laydownSettings.layerThickness);
    m_CNCComPort.sendGCodeCommand(cmd);
    m_CNCComPort.getAxisPositions(&ap);
    ui->lblRecoatCurrentZ->setText(QString("Current Z = %1").arg(ap.z));

}


void MainWindow::on_setCurrentLayer_clicked()
{
    if( maxLayers > 0 ){
        bool ok= false;
        int nextLayer = QInputDialog::getInt(this, QString("Enter New Layer"), QString("Enter a value from 1 to %1").arg(maxLayers), 1, 1,(int)maxLayers, 1, &ok);
        if(ok)
            m_executiveEngine.setCurrentLayer(nextLayer);
        ui->lblCurrentLayer->setText(QString("%1").arg(m_executiveEngine.currentLayer()+1));
    }
}


void MainWindow::on_gotoTempCheckX_clicked()
{
    float_t pos = ui->tempCheckParams->item(0,0)->text().toFloat();
    QString cmd = QString("G1 X%1 F%2").arg(pos).arg(recoaterSettings.x_move_speed);
    m_CNCComPort.sendGCodeCommand("G90\n");
    m_CNCComPort.sendGCodeCommand(cmd);
}


void MainWindow::on_gotoTempCheckY_clicked()
{
    float_t pos = ui->tempCheckParams->item(1,0)->text().toFloat();
    QString cmd = QString("G1 Y%1 F%2").arg(pos).arg(recoaterSettings.y_move_speed);
    m_CNCComPort.sendGCodeCommand("G90\n");
    m_CNCComPort.sendGCodeCommand(cmd);
}


void MainWindow::on_fireSample_clicked()
{
    // Build a command list to execute the dropmass firing sequence
    uint32_t firings = ui->tblDropmass->item(1,0)->text().toInt();
    laydownSettings.testLocationX = ui->tblDropmass->item(0,0)->text().toFloat();
    QStringList fireSampleRoutine;
    fireSampleRoutine.append(";:RUN:PRIMEWIPEFIRE.GCODE:");
    fireSampleRoutine.append(QString("G1 X%1 F%2").arg(laydownSettings.testLocationX).arg(recoaterSettings.x_move_speed));
    fireSampleRoutine.append("G4 P1000");
    fireSampleRoutine.append(QString(";:PHC:FC,%1").arg(firings));
    uint32_t delay = (uint32_t) ((float)firings/laydownSettings.printFrequency * 1000.0f);
    delay += 250; // add extra quarter second
    fireSampleRoutine.append(QString("G4 P%1").arg(delay));
    fireSampleRoutine.append(";:RUN:CAPHEAD.GCODE:");
    m_executiveEngine.executeCommandList(fireSampleRoutine, true);
    //for( int i=0; i<fireSampleRoutine.length(); i++ )
    //    ui->txtCNCLog->appendPlainText(fireSampleRoutine[i]);
}


void MainWindow::on_calculateDropmass_clicked()
{
    float mass = ui->tblDropmass->item(3,0)->text().toFloat();
    uint32_t firings = ui->tblDropmass->item(1,0)->text().toInt();
    mass += ui->tblDropmass->item(4,0)->text().toFloat();
    mass += ui->tblDropmass->item(5,0)->text().toFloat();
    mass = mass / 3.0f; // average the readings
    mass = mass / (float)firings; // divide by drops per jet
    mass = mass / (float)laydownSettings.jetCount; // divide by jets
    mass = mass * (1.0f/ui->tblDropmass->item(2,0)->text().toFloat());
    mass = mass * 1000000000.0f;
    ui->tblDropmass->item(6,0)->setText(QString("%1").arg(mass));


}


void MainWindow::on_raiseFby1_clicked()
{
    if( this->readRecoatingPage() ){
        CNCAxisPositions pos{};
        m_CNCComPort.getAxisPositions(&pos);
        pos.u += recoaterSettings.feed_powder_ratio * laydownSettings.layerThickness;
        m_CNCComPort.sendGCodeCommand("G90");
        m_CNCComPort.sendGCodeCommand(QString("G1 U%1 F%2").arg(pos.u).arg(recoaterSettings.z_move_speed));
    }
}


void MainWindow::on_lowerFto0_clicked()
{
    m_CNCComPort.sendGCodeCommand("G90");
    m_CNCComPort.sendGCodeCommand(QString("G1 U0 F%1").arg(recoaterSettings.z_move_speed));

}


void MainWindow::on_phSelect_currentIndexChanged(int index)
{
    PH_TYPES type = (PH_TYPES)index;
    laydownSettings.jetCount = phDescriptions[type].jetCount;
    laydownSettings.jetSpacing = phDescriptions[type].jetSpacing;
    laydownSettings.passCount = laydownSettings.jetSpacing / laydownSettings.passSpacing;
    laydownSettings.phType = type;
    m_PHCCommPort.setModuleType(type);
    this->writeLaydownPage();
    this->readLaydownPage();

}


void MainWindow::on_preheatBuildplate_clicked()
{
   m_executiveEngine.setBuildBedTemp(recoaterSettings.build_plate_temp);
}


void MainWindow::on_preheatOverhead_clicked()
{
    m_executiveEngine.setOverheadTemp(recoaterSettings.temperature);
}


void MainWindow::on_runPHCInitScript_clicked()
{
    m_executiveEngine.attachInitializeList(ui->PHCInitScript->toPlainText().split('\n'));
    m_executiveEngine.runInitializeList();
}


void MainWindow::on_preheatBinder_clicked()
{
    float newTemp;
    bool ok=false;
    newTemp = ui->initTempTable->item(2,1)->text().toFloat(&ok);
    if( ok ) m_executiveEngine.setBinderTemp(newTemp);
}


void MainWindow::on_preheatOven_clicked()
{
    float newTemp;
    bool ok=false;
    newTemp = ui->initTempTable->item(3,1)->text().toFloat(&ok);
    if( ok ) m_executiveEngine.setOvenTemp(newTemp);
}


void MainWindow::on_preheatFeedplate_clicked()
{
    float newTemp;
    bool ok=false;
    newTemp = ui->initTempTable->item(4,1)->text().toFloat(&ok);
    if( ok ) m_executiveEngine.setFeedBedTemp(newTemp);

}


void MainWindow::on_tryFan1_clicked()
{
    static bool running = false;
    if( readRecoatingPage() ){
        if( !running ) {
            m_CNCComPort.sendGCodeCommand("M106 P4 S255");
            m_CNCComPort.sendGCodeCommand("G4 P500");
            m_CNCComPort.sendGCodeCommand(QString("M106 P4 S%1").arg(recoaterSettings.fan1_pwm));
            ui->tryFan1->setText("Stop Fan 1");
            running = true;
        }
        else {
            m_CNCComPort.sendGCodeCommand("M106 P4 S0");
            ui->tryFan1->setText("Try Fan 1");
            running = false;
        }
    }
}


void MainWindow::on_tryFan2_clicked()
{
    static bool running = false;
    if( readRecoatingPage() ){
        if( !running ) {
            m_CNCComPort.sendGCodeCommand("M106 P5 S255");
            m_CNCComPort.sendGCodeCommand("G4 P500");
            m_CNCComPort.sendGCodeCommand(QString("M106 P5 S%1").arg(recoaterSettings.fan1_pwm));
            ui->tryFan2->setText("Stop Fan 2");
            running = true;
        }
        else {
            m_CNCComPort.sendGCodeCommand("M106 P5 S0");
            ui->tryFan2->setText("Try Fan 2");
            running = false;
        }
    }

}


void MainWindow::on_setFPosition_clicked()
{
    CNCAxisPositions pos;
    bool ok = false;
    m_CNCComPort.getAxisPositions(&pos);
    float newU = ui->txtRaiseFTo->text().toFloat(&ok);
    if( ok ){
        if( newU < 0.0f ){ newU = 0.0f; ui->txtRaiseFTo->setText("0.0");}
        if( newU < pos.u ){ // do backlash move
            if( pos.u > 0.5 )
                m_CNCComPort.sendGCodeCommand(QString("G1 U%1 F240\n").arg(newU-0.5f));
            else
                m_CNCComPort.sendGCodeCommand("G1 U0 F180\n");
            m_CNCComPort.sendGCodeCommand(QString("G1 U%1 F240\n").arg(newU));
        }
        else { // simply move up
            if( newU > 63.5f ){ newU = 63.5f; ui->txtRaiseZTo->setText("63.5");}
            m_CNCComPort.sendGCodeCommand(QString("G1 U%1 F240\n").arg(newU));
        }
    }
    m_CNCComPort.getAxisPositions(&pos);
    ui->recoatCurrentF->setText(QString("Current U = %1").arg(pos.u));

}


void MainWindow::on_raiseFToTop_clicked()
{
    CNCAxisPositions pos;
    m_CNCComPort.sendGCodeCommand("G90");
    m_CNCComPort.sendGCodeCommand(QString("G1 U63.5 F%1").arg(recoaterSettings.z_move_speed));
    m_CNCComPort.getAxisPositions(&pos);
    ui->recoatCurrentF->setText(QString("Current U = %1").arg(pos.u));

}

void MainWindow::on_goRollMidpoint_clicked()
{
    if(this->readRecoatingPage()){
        QString cmd = QString("G1 A%1 F%2\n").arg(recoaterSettings.rolling_midpoint).arg(recoaterSettings.a_move_speed);
        m_CNCComPort.sendGCodeCommand("G90\n");
        m_CNCComPort.sendGCodeCommand(cmd);
    }

}


void MainWindow::on_trySmooth_B2B_clicked()
{
    if(this->readRecoatingPage()){
        if( ui->txtGCode->toPlainText().length() == 0 )
            this->on_cmdGenerateRecoaterGCode_clicked();
        QApplication::processEvents();

        QString recoat = ui->txtGCode->toPlainText();
        int sb2b_start = recoat.indexOf(";:START:SMOOTH_B2B.GCODE:\n") + QString(";:START:SMOOTH_B2B.GCODE:\n").length();
        int sb2b_end = recoat.indexOf(";:END:SMOOTH_B2B.GCODE:",sb2b_start);
        QString sb2b_cmds = recoat.mid(sb2b_start, sb2b_end - sb2b_start);
        //qDebug() << dry_cmds;
        QStringList sb2b_list = sb2b_cmds.split("\n");
        int i=0;
        for( i=0; i<sb2b_list.length(); i++ ){
            if( !m_CNCComPort.sendGCodeCommand(sb2b_list[i])) break;
        }
        if( i==sb2b_list.length())
            m_CNCComPort.sendGCodeCommand(QString("G1 A5 F%1\n").arg(recoaterSettings.a_move_speed));
    }

}


void MainWindow::on_enablePowerScaling_stateChanged(int arg1)
{

}


void MainWindow::on_unloadAllSTL_clicked()
{
    // Attempt to unload a file.
    stl_window->unloadAllFiles();
    maxLayers = 0;
    ui->sbSliceLayer->setMaximum(0);
    ui->sbSliceLayer->setMinimum(0);
    ui->sbSliceLayer->setEnabled(false);
    ui->lblSliceHeight->setText("0");
    ui->lblSliceHeightmm->setText("0");
    ui->lblLayersInFile->setText("0");
    stl_window->requestUpdate();

}


void MainWindow::on_cancelCNCCommand_clicked()
{
    m_CNCComPort.sendGCodeCommand("M108");
}

/*void oldSliceCode(){
    for( uint32_t layer = 0; layer< maxLayers; layer++){
        QApplication::processEvents();
        if( b_cancelSlice ) break;
        for( uint32_t objCt=0; objCt < stl_window->stlFiles.size(); objCt++){
            stl_window->stlFiles[objCt].bindData = true; // Reveal one object
            if( laydownSettings.layerThicknesses.length() == 0)
                stl_window->m_sliceHeight = ((float)layer+0.5f) * laydownSettings.layerThickness;
            else
                stl_window->m_sliceHeight = 0.5f*laydownSettings.layerThicknesses[0] + (0.5f * layer * (laydownSettings.layerThicknesses[0]+laydownSettings.layerThicknesses[1]));
            stl_window->m_cameraTransform.translation = {0.0f, 0.0f, stl_window->m_sliceHeight-0.005f};
            ui->lblSliceHeight->setText(QString("%1").arg(layer));
            ui->lblSliceHeightmm->setText(QString("%1").arg(stl_window->m_sliceHeight));
            if( laydownSettings.majorOffsets.length() > 1){
                // Calculate Randomization Shift
                maj = laydownSettings.majorOffsets[((layer) % laydownSettings.majorOffsets.length())];
                min = laydownSettings.minorOffsets[((layer) % laydownSettings.minorOffsets.length())];
                y_shift_axis = (float)maj * laydownSettings.jetSpacing + (float)min * laydownSettings.passSpacing;
                x_shift_model = y_shift_axis - (MAX_SHIFT/2.0f);
            } //-IMAGE_WID_H_MM + x_shift_axis;
            else {
                y_shift_axis = 0.0f;
                x_shift_model = -1.0*(IMAGE_WID_MM-stl_window->stlFiles[objCt].width)/2.0F;
            }
            //        qDebug() << "layer : x_shift_axis : x_shift_model : maj : min= " << layer <<" : "
            //                 << y_shift_axis << " : " << x_shift_model << " : " << maj << " : " << min;
            stl_window->stlFiles[objCt].transform.translation.x = x_shift_model;
            if( laydownSettings.jetCount * laydownSettings.passCount == 512 ){
                stl_window->ortho_top = -32.512f;
                stl_window->ortho_bottom = 32.512f;
                stl_window->ortho_left = -32.512f;
                stl_window->ortho_right = 32.512f;

                stl_window->requestUpdate();
                QApplication::processEvents();
                srcImg = stl_window->grab();
            }
            else {
                srcImg = makeQuadImage();
            }
            if( laydownSettings.phType == S_CLASS ){
                m_PHCCommPort.setModuleType(S_CLASS);
                ripdImg = this->ripSLImage( srcImg );
            }
            else if( stl_window->stlFiles.size() == 1 && laydownSettings.phType == Q_CLASS_1S ){
                m_PHCCommPort.setModuleType(Q_CLASS_1S);
                ripdImg = this->ripQ1Image( srcImg );
            }
            else if( stl_window->stlFiles.size() > 1 && laydownSettings.phType == Q_CLASS_2S){
                m_PHCCommPort.setModuleType(Q_CLASS_2S);
                ripdImg = this->ripQ2Image( srcImg, objCt );
            }
            else if( laydownSettings.phType == Q_CLASS_SG ){
                m_PHCCommPort.setModuleType(Q_CLASS_SG);
                ripdImg = this->ripSGImageFwd(srcImg);
            }
            //QTime end = QTime::currentTime().addSecs(1);
            //while( end > QTime::currentTime())
            bool ok;
            if( stl_window->stlFiles.size() > 1){
                QString fn_raw = filePath + QString("/layer_%1_color_%2.png").arg(layer).arg(objCt);
                imgWriter.setFileName(fn_raw);
                imgWriter.setFormat("png");
                imgWriter.setText("layer", QString("%1").arg(layer));
                imgWriter.setText("sliceHeight", QString("%1").arg(stl_window->m_sliceHeight));
                imgWriter.write(srcImg);
                QString fn_rip = filePath + QString("/layer_%1RIP_color_%2.png").arg(layer).arg(objCt);
                imgWriter.setFileName(fn_rip);
                imgWriter.write(ripdImg);
            }
            else {
                QString fn_raw = filePath + QString("/layer_%1.png").arg(layer);
                imgWriter.setFileName(fn_raw);
                imgWriter.setFormat("png");
                imgWriter.setText("layer", QString("%1").arg(layer));
                imgWriter.setText("sliceHeight", QString("%1").arg(stl_window->m_sliceHeight));
                imgWriter.write(srcImg);
                QString fn_rip = filePath + QString("/layer_%1RIP.png").arg(layer).arg(objCt);
                imgWriter.setFileName(fn_rip);
                imgWriter.write(ripdImg);
            }
            stl_window->stlFiles[objCt].bindData = false; // Hide it again

} */
