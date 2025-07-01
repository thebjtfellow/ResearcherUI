#include "executiveengine.h"
#include "crc.h"
#include <QFile>
#include <QApplication>
#include <QBuffer>
#include <QImageReader>
#include <QImageWriter>

IOConfig ioConfig;

gpiod_chip * ioChip = nullptr;
gpiod_line * ioLine = nullptr;

RecoaterSettings& RecoaterSettings::operator=(const RecoaterSettings& other){
    this->heat_start = other.heat_start;
    this->heat_end = other.heat_end;
    this->heat_speed = other.heat_speed;
    this->heat_speeds.clear();
    for(int i=0; i<other.heat_speeds.length(); i++){
        this->heat_speeds.append(other.heat_speeds[i]);
    }
    this->temperature = other.temperature;
    this->fan1_pwm = other.fan1_pwm;
    this->fan2_pwm = other.fan2_pwm;
    this->disp_start = other.disp_start;
    this->disp_end = other.disp_end;
    this->disp_sections = other.disp_sections;
    this->disp_power = other.disp_power;
    this->disp_powers.clear();
    for(int i=0; i<other.disp_powers.length(); i++){
        this->disp_powers.append(other.disp_powers[i]);
    }
    this->disp_speed = other.disp_speed;
    this->disp_speeds.clear();
    for(int i=0; i<other.disp_speeds.length(); i++){
        this->disp_speeds.append(other.disp_speeds[i]);
    }
    this->rolling_start = other.rolling_start;
    this->rolling_end = other.rolling_end;
    this->roller1_rpm = other.roller1_rpm;
    this->roller2_rpm = other.roller2_rpm;
    this->rolling_speed = other.rolling_speed;
    this->rolling_speeds.clear();
    for(int i=0; i<other.rolling_speeds.length(); i++){
        this->rolling_speeds.append(other.rolling_speeds[i]);
    }
    this->rolling_len = other.rolling_len;
    this->rolling_time = other.rolling_time;
    this->roller1_turns = other.roller1_turns;
    this->roller2_turns = other.roller2_turns;
    this->single_roller_motor = other.single_roller_motor;
    this->z_retract = other.z_retract;
    this->x_move_speed = other.x_move_speed;
    this->y_move_speed = other.y_move_speed;
    this->z_move_speed = other.z_move_speed;
    this->a_move_speed = other.a_move_speed;
    this->x_home_sense = other.x_home_sense;
    this->y_home_sense = other.y_home_sense;
    this->z_home_sense = other.z_home_sense;
    this->a_home_sense = other.a_home_sense;
    this->useTurnsAsDelay = other.useTurnsAsDelay;
    this->enablePowerScaling = other.enablePowerScaling;
    this->build_plate_temp = other.build_plate_temp;
    this->rolling_midpoint = other.rolling_midpoint;
    this->rolling_fast_speed = other.rolling_fast_speed;
    this->feed_powder_ratio = other.feed_powder_ratio;
    this->feed_plate_temp = other.feed_plate_temp;
    this->oven_temp = other.oven_temp;
    return *this;
}

bool RecoaterSettings::writeFile(const QString& fileName, RecoaterSettings *settings){
    QSettings settingsFile(fileName, QSettings::IniFormat);
    QVariant v1;
    settingsFile.beginGroup("Recoater");
    settingsFile.setValue("heat_start",settings->heat_start);
    settingsFile.setValue("heat_end", settings->heat_end);
    settingsFile.setValue("heat_speed", settings->heat_speed);
    v1 = QVariant::fromValue(settings->heat_speeds);
    settingsFile.setValue("heat_speeds", v1);
    settingsFile.setValue("temperature", settings->temperature);
    settingsFile.setValue("fan1_pwm", settings->fan1_pwm);
    settingsFile.setValue("fan2_pwm", settings->fan2_pwm);
    settingsFile.setValue("disp_start", settings->disp_start);
    settingsFile.setValue("disp_end", settings->disp_end);
    settingsFile.setValue("disp_sections", settings->disp_sections);
    settingsFile.setValue("disp_power", settings->disp_power);
    v1 = QVariant::fromValue(settings->disp_powers);
    settingsFile.setValue("disp_powers", v1);
    settingsFile.setValue("disp_speed", settings->disp_speed);
    v1 = QVariant::fromValue(settings->disp_speeds);
    settingsFile.setValue("disp_speeds", v1);
    settingsFile.setValue("rolling_start", settings->rolling_start);
    settingsFile.setValue("rolling_end", settings->rolling_end);
    settingsFile.setValue("roller1_rpm", settings->roller1_rpm);
    settingsFile.setValue("roller2_rpm", settings->roller2_rpm);
    settingsFile.setValue("rolling_speed", settings->rolling_speed);
    v1 = QVariant::fromValue(settings->rolling_speeds);
    settingsFile.setValue("rolling_speeds", v1);
    settingsFile.setValue("rolling_len", settings->rolling_len);
    settingsFile.setValue("rolling_time", settings->rolling_time);
    settingsFile.setValue("roller1_turns", settings->roller1_turns);
    settingsFile.setValue("roller2_turns", settings->roller2_turns);
    settingsFile.setValue("single_roller_motor", settings->single_roller_motor);
    settingsFile.setValue("z_retract", settings->z_retract);
    settingsFile.setValue("x_move_speed", settings->x_move_speed);
    settingsFile.setValue("y_move_speed", settings->y_move_speed);
    settingsFile.setValue("z_move_speed", settings->z_move_speed);
    settingsFile.setValue("a_move_speed", settings->a_move_speed);
    settingsFile.setValue("x_home_sense", settings->x_home_sense);
    settingsFile.setValue("y_home_sense", settings->y_home_sense);
    settingsFile.setValue("z_home_sense", settings->z_home_sense);
    settingsFile.setValue("a_home_sense", settings->a_home_sense);
    settingsFile.setValue("useTurnsAsDelay", settings->useTurnsAsDelay);
    settingsFile.setValue("enablePowerScaling",settings->enablePowerScaling);
    settingsFile.setValue("build_plate_temp", settings->build_plate_temp);
    settingsFile.setValue("rolling_midpoint", settings->rolling_midpoint);
    settingsFile.setValue("rolling_fast_speed", settings->rolling_fast_speed);
    settingsFile.setValue("feed_powder_ratio", settings->feed_powder_ratio);
    settingsFile.setValue("feed_plate_temp", settings->feed_plate_temp);
    settingsFile.setValue("oven_temp", settings->oven_temp);
    settingsFile.endGroup();
    return true;
}

bool RecoaterSettings::readFile(const QString& fileName, RecoaterSettings *settings){
    QSettings settingsFile(fileName, QSettings::IniFormat);
    QVariant v1;
    settingsFile.beginGroup("Recoater");
    settings->heat_start = settingsFile.value("heat_start").toFloat();
    settings->heat_end = settingsFile.value("heat_end").toFloat();
    settings->heat_speed = settingsFile.value("heat_speed").toFloat();
    v1 = settingsFile.value("heat_speeds");
    settings->heat_speeds = v1.value<QList<float_t>>();
    settings->temperature = settingsFile.value("temperature").toFloat();
    settings->fan1_pwm = settingsFile.value("fan1_pwm").toUInt();
    settings->fan2_pwm = settingsFile.value("fan2_pwm").toUInt();
    settings->disp_start = settingsFile.value("disp_start").toFloat();
    settings->disp_end = settingsFile.value("disp_end").toFloat();
    settings->disp_sections = settingsFile.value("disp_sections").toInt();
    settings->disp_power = settingsFile.value("disp_power").toInt();
    v1 = settingsFile.value("disp_powers");
    settings->disp_powers = v1.value<QList<uint32_t>>();
    settings->disp_speed = settingsFile.value("disp_speed").toFloat();
    v1 = settingsFile.value("disp_speeds");
    settings->disp_speeds = v1.value<QList<float_t>>();
    settings->rolling_start = settingsFile.value("rolling_start").toFloat();
    settings->rolling_end = settingsFile.value("rolling_end").toFloat();
    settings->roller1_rpm = settingsFile.value("roller1_rpm").toFloat();
    settings->roller2_rpm = settingsFile.value("roller2_rpm").toFloat();
    settings->rolling_speed = settingsFile.value("rolling_speed").toFloat();
    v1 = settingsFile.value("rolling_speeds");
    settings->rolling_speeds = v1.value<QList<float_t>>();
    settings->rolling_len = settingsFile.value("rolling_len").toFloat();
    settings->rolling_time = settingsFile.value("rolling_time").toFloat();
    settings->roller1_turns = settingsFile.value("roller1_turns").toFloat();
    settings->roller2_turns = settingsFile.value("roller2_turns").toFloat();
    settings->single_roller_motor = settingsFile.value("single_roller_motor").toBool();
    settings->z_retract = settingsFile.value("z_retract").toFloat();
    settings->x_move_speed = settingsFile.value("x_move_speed").toFloat();
    settings->y_move_speed = settingsFile.value("y_move_speed").toFloat();
    settings->z_move_speed = settingsFile.value("z_move_speed").toFloat();
    settings->a_move_speed = settingsFile.value("a_move_speed").toFloat();
    settings->x_home_sense = settingsFile.value("x_home_sense").toFloat();
    settings->y_home_sense = settingsFile.value("y_home_sense").toFloat();
    settings->z_home_sense = settingsFile.value("z_home_sense").toFloat();
    settings->a_home_sense = settingsFile.value("a_home_sense").toFloat();
    settings->useTurnsAsDelay = settingsFile.value("useTurnsAsDelay").toBool();
    settings->enablePowerScaling = settingsFile.value("enablePowerScaling").toBool();
    settings->build_plate_temp = settingsFile.value("build_plate_temp").toFloat();
    settings->rolling_midpoint = settingsFile.value("rolling_midpoint").toFloat();
    settings->rolling_fast_speed = settingsFile.value("rolling_fast_speed").toFloat();
    settings->feed_powder_ratio = settingsFile.value("feed_powder_ratio").toFloat();
    settings->feed_plate_temp = settingsFile.value("feed_plate_temp").toFloat();
    settings->oven_temp = settingsFile.value("oven_temp").toFloat();
    settingsFile.endGroup();
    return true;
}

LaydownSettings& LaydownSettings::operator=(const LaydownSettings& other){
    this->jetCount = other.jetCount;
    this->jetSpacing = other.jetSpacing;
    this->dropVolume = other.dropVolume;
    this->pulseWidth = other.pulseWidth;
    for( int i=0; i<other.pulseWidths.length(); i++)
        this->pulseWidths.append(other.pulseWidths[i]);
    this->packingRate = other.packingRate;
    this->passCount = other.passCount;
    this->printSpeed = other.printSpeed;
    this->printFrequency = other.printFrequency;
    this->layerThickness = other.layerThickness;
    this->passSpacing = other.passSpacing;
    this->lineSpacing = other.lineSpacing;
    this->saturation = other.saturation;
    this->yAlignZero = other.yAlignZero;
    this->startPassX = other.startPassX;
    this->endPassX = other.endPassX;
    this->capX = other.capX;
    this->capY = other.capY;
    this->capSafeX = other.capSafeX;
    this->wipeStartX = other.wipeStartX;
    this->wipeEndX = other.wipeEndX;
    this->wipeY = other.wipeY;
    this->wipeSpeed = other.wipeSpeed;
    this->primeX = other.primeX;
    this->majorOffsets.clear();
    for(int i=0; i<other.majorOffsets.length(); i++)
        this->majorOffsets.append(other.majorOffsets[i]);
    this->minorOffsets.clear();
    for(int i=0; i<other.minorOffsets.length(); i++)
        this->majorOffsets.append(other.minorOffsets[i]);
    this->yAxisBacklash = other.yAxisBacklash;
    this->yHomeOffset = other.yHomeOffset;
    this->tempCheckX = other.tempCheckX;
    this->tempCheckY = other.tempCheckY;
    this->testLocationX = other.testLocationX;
    this->binderTemp = other.binderTemp;
    this->phType = other.phType;
    for( int i=0; i<other.layerThicknesses.length(); i++ )
        this->layerThicknesses.append(other.layerThicknesses[i]);
    return *this;
}

bool LaydownSettings::readFile(const QString &fileName, LaydownSettings* settings){
    QSettings settingsFile(fileName, QSettings::IniFormat);
    settingsFile.beginGroup("Laydown");
    //settingsFile.setValue ,settings->
    settings->jetCount = settingsFile.value("jetCount").toFloat();
    settings->jetSpacing = settingsFile.value("jetSpacing").toFloat();
    settings->dropVolume = settingsFile.value("dropVolume").toFloat();
    settings->pulseWidth = settingsFile.value("pulseWidth").toFloat();
    QVariant v1 = settingsFile.value("pulseWidths");
    settings->pulseWidths = v1.value<QList<float_t>>();
    settings->packingRate = settingsFile.value("packingRate").toFloat();
    settings->passCount = settingsFile.value("passCount").toFloat();
    settings->printSpeed = settingsFile.value("printSpeed").toFloat();
    settings->printFrequency = settingsFile.value("printFrequency").toFloat();
    settings->layerThickness = settingsFile.value("layerThickness").toFloat();
    settings->passSpacing = settingsFile.value("passSpacing").toFloat();
    settings->lineSpacing = settingsFile.value("lineSpacing").toFloat();
    settings->saturation = settingsFile.value("saturation").toFloat();
    settings->yAlignZero = settingsFile.value("yAlignZero").toFloat();
    settings->startPassX = settingsFile.value("startPassX").toFloat();
    settings->endPassX = settingsFile.value("endPassX").toFloat();
    settings->capX = settingsFile.value("capX").toFloat();
    settings->capY = settingsFile.value("capY").toFloat();
    settings->capSafeX = settingsFile.value("capSafeX").toFloat();
    settings->wipeStartX = settingsFile.value("wipeStartX").toFloat();
    settings->wipeEndX = settingsFile.value("wipeEndX").toFloat();
    settings->wipeY = settingsFile.value("wipeY").toFloat();
    settings->wipeSpeed = settingsFile.value("wipeSpeed").toFloat();
    settings->primeX = settingsFile.value("primeX").toFloat();
    settings->primeTime = settingsFile.value("primeTime").toFloat();
    settings->dripTime = settingsFile.value("dripTime").toFloat();
    v1 = settingsFile.value("majorOffsets");
    settings->majorOffsets = v1.value<QList<uint32_t>>();
    v1 = settingsFile.value("minorOffsets");
    settings->minorOffsets = v1.value<QList<uint32_t>>();
    settings->yAxisBacklash = settingsFile.value("yAxisBacklash").toFloat();
    settings->yHomeOffset = settingsFile.value("yHomeOffset").toFloat();
    settings->tempCheckX = settingsFile.value("tempCheckX").toFloat();
    settings->tempCheckY = settingsFile.value("tempCheckY").toFloat();
    settings->testLocationX = settingsFile.value("testLocationX").toFloat();
    settings->binderTemp = settingsFile.value("binderTemp").toFloat();
    settings->phType = (PH_TYPES)settingsFile.value("phType").toUInt();
    v1 = settingsFile.value("layerThicknesses");
    settings->layerThicknesses = v1.value<QList<float_t>>();
    settingsFile.endGroup();
    return true;
}

bool LaydownSettings::writeFile(const QString &fileName, LaydownSettings* settings){
    QSettings settingsFile(fileName, QSettings::IniFormat);
    settingsFile.beginGroup("Laydown");
    //settingsFile.setValue ,settings->
    settingsFile.setValue("jetCount",settings->jetCount);
    settingsFile.setValue("jetSpacing",settings->jetSpacing);
    settingsFile.setValue("dropVolume",settings->dropVolume);
    settingsFile.setValue("pulseWidth",settings->pulseWidth);
    QVariant vp = QVariant::fromValue(settings->pulseWidths);
    settingsFile.setValue("pulseWidths", vp);
    settingsFile.setValue("packingRate",settings->packingRate);
    settingsFile.setValue("passCount",settings->passCount);
    settingsFile.setValue("printSpeed",settings->printSpeed);
    settingsFile.setValue("printFrequency",settings->printFrequency);
    settingsFile.setValue("layerThickness",settings->layerThickness);
    settingsFile.setValue("passSpacing",settings->passSpacing);
    settingsFile.setValue("lineSpacing",settings->lineSpacing);
    settingsFile.setValue("saturation",settings->saturation);
    settingsFile.setValue("yAlignZero",settings->yAlignZero);
    settingsFile.setValue("startPassX",settings->startPassX);
    settingsFile.setValue("endPassX",settings->endPassX);
    settingsFile.setValue("capX",settings->capX);
    settingsFile.setValue("capY",settings->capY);
    settingsFile.setValue("capSafeX",settings->capSafeX);
    settingsFile.setValue("wipeStartX",settings->wipeStartX);
    settingsFile.setValue("wipeEndX",settings->wipeEndX);
    settingsFile.setValue("wipeY",settings->wipeY);
    settingsFile.setValue("wipeSpeed",settings->wipeSpeed);
    settingsFile.setValue("primeX",settings->primeX);
    settingsFile.setValue("primeTime", settings->primeTime);
    settingsFile.setValue("dripTime", settings->dripTime);
    QVariant v1 = QVariant::fromValue(settings->majorOffsets);
    settingsFile.setValue("majorOffsets", v1 );
    QVariant v2 = QVariant::fromValue(settings->minorOffsets);
    settingsFile.setValue("minorOffsets", v2);
    settingsFile.setValue("yAxisBacklash",settings->yAxisBacklash);
    settingsFile.setValue("yHomeOffset",settings->yHomeOffset);
    settingsFile.setValue("tempCheckX",settings->tempCheckX);
    settingsFile.setValue("tempCheckY",settings->tempCheckY);
    settingsFile.setValue("testLocationX", settings->testLocationX);
    settingsFile.setValue("binderTemp", settings->binderTemp);
    settingsFile.setValue("phType",settings->phType);
    QVariant v3 = QVariant::fromValue(settings->layerThicknesses);
    settingsFile.setValue("layerThicknesses", v3);
    settingsFile.endGroup();

    return true;
}

bool IOConfig::saveFile(const QString& fileName){
    QSettings sfile(fileName, QSettings::IniFormat);
    if( sfile.status() != QSettings::NoError )
        return false;
    sfile.beginGroup("IOConfig");
    for( int i=2; i<=27; i++ ){
        sfile.setValue(QString("IO%1isInput").arg(i),this->isInput[i-2]);
        sfile.setValue(QString("IO%1isOn").arg(i),this->isOn[i-2]);
    }
    sfile.endGroup();
    return true;
}
bool IOConfig::readFile(const QString& fileName){
    QSettings sfile(fileName, QSettings::IniFormat);
    if( sfile.status() != QSettings::NoError )
        return false;
    sfile.beginGroup("IOConfig");
    for( int i=2; i<=27; i++ ){
        this->isInput[i-2] = sfile.value(QString("IO%1isInput").arg(i), true).toBool();
        this->isOn[i-2] = sfile.value(QString("IO%1isOn").arg(i), false).toBool();
    }
    sfile.endGroup();
    return true;
}


bool MaterialINIFile::loadName(QString filePath){
    bool ok = true;
    QSettings settingsFile(filePath, QSettings::IniFormat);
    settingsFile.beginGroup("MaterialFile");
    this->MaterialINI = settingsFile.value("curMaterial").toString();
    settingsFile.endGroup();
    if( MaterialINI == "" ) ok = false;
    return ok;
}
bool MaterialINIFile::saveName(QString filePath){
    QSettings settingsFile(filePath, QSettings::IniFormat);
    settingsFile.beginGroup("MaterialFile");
    settingsFile.setValue("curMaterial", this->MaterialINI);
    settingsFile.endGroup();
    return true;
}
//----------------------------- GCODE FILE FUNCTIONS
bool GCodeFile::loadGCodeFile(GCodeFile *toLoad, QString fileName){
    QFile f = QFile(fileName);
    f.open(QIODevice::ReadOnly);
    if( f.isOpen()){
        QByteArray qa = f.readAll();
        QString s = QString(qa);
        toLoad->instructions = s.split('\n');
        toLoad->fileName = fileName;
        return true;
    }
    else
        return false;
}

bool GCodeFile::clearFile( GCodeFile *toClear ){
    toClear->fileName = "";
    toClear->id = 0;
    toClear->instructions.clear();
    return true;
}

//-----------------------------


CNCComManager::CNCComManager() : QSerialPort(){

}
CNCComManager::~CNCComManager(){
    this->close();
}

bool CNCComManager::sendGCodeCommand(QString command, QString *response){ // Send a single command and wait
    bool sawRefill = false;
    if( !m_portOpen ){
        if( m_enableVLog )
            m_visualLog->appendPlainText(QString("PORT CLOSED: sendGCodeCommand : %1").arg(command.left(command.length()-1)));
        return false;
    }

    if( !command.contains('\n')) command.append('\n');


    if( command.left(1) == ";") return true;

    bool ok = false;
    char c1=0;

    if( command.length() < 2 ){
        if( m_enableVLog )
            m_visualLog->appendPlainText("NULL Command Received");
        return true;
    }
    if( m_enableVLog ){
        if( command.left(4) != "M105")
            m_visualLog->appendPlainText(QString("To CNC: %1").arg(command.left(command.length()-1)));
    }
    if( m_inSendGCode ) return false;
    m_inSendGCode = true;
    m_expected = true;
    QString res;
    QByteArray residualText;
    if( this-> bytesAvailable()>0){
        residualText = this->readAll();
        res = QString::fromLocal8Bit(residualText);
        m_visualLog->appendPlainText("Old Text in Buffer:");
        m_visualLog->appendPlainText(res);
    }

    QByteArray qa = command.toLocal8Bit();
    this->clear(QSerialPort::AllDirections);
    ok = this->write(qa.data(), qa.length());
    this->flush();
    QApplication::processEvents();

    qa.clear();
    // Stay for response a minimum of 150ms or until we get ok\n
    QTime inMin = QTime::currentTime().addMSecs(50);
    QTime timeout = QTime::currentTime().addSecs(3);
    const uint32_t lenOfNote = QString("echo:busy: processing\n").length();
    const uint32_t lenOfNote2 = QString("echo:busy: paused for user\n").length();

    while(!res.contains("ok") || (inMin>QTime::currentTime()) ){
        this->waitForReadyRead(10);
        if( this->bytesAvailable() > 0 ){
            qa += this->readAll();
            res = QString::fromLocal8Bit(qa);
            timeout = QTime::currentTime().addSecs(3);
            if( res.contains("echo:busy: processing\n")){
                uint32_t idx = res.indexOf("echo:busy: processing\n");
                if( res.length() > lenOfNote && idx == 0){
                    res = res.mid(lenOfNote);
                    qa.clear();
                    qa = res.toLocal8Bit();
                }
                else
                    qa.clear();
            }
            else if( res.contains("echo:busy: paused for user\n")){
                uint32_t idx = res.indexOf("echo:busy: paused for user\n");
                if( res.length() > lenOfNote2 && idx == 0 ){
                    res = res.mid(lenOfNote2);
                    qa.clear();
                    qa = res.toLocal8Bit();
                }
                else
                    qa.clear();
            }
            else if( res.contains("REFILL")){
                // we sent a command, but the refill cycle already started
                m_expected = false;
                m_inRefill = true;
                while (m_inRefill )
                    QApplication::processEvents();
                timeout = QTime::currentTime().addSecs(3);
                sawRefill = true;
            }

        }
        if( QTime::currentTime() > timeout){
            ok = false;
            if( m_enableVLog ){
                m_visualLog->appendPlainText(QString("Timed out G-Code: %1").arg(command));
                if( res.length() > 0) m_visualLog->appendPlainText(QString("Received: %1").arg(res));
                m_visualLog->appendPlainText("Closing Port. Please restart comms");
                m_portOpen = false;
            }
            break;
        }
    }
    inMin = QTime::currentTime().addMSecs(100);
    timeout = QTime::currentTime().addSecs(10);

    while(!res.contains("\n") || (inMin>QTime::currentTime()) ){
        this->waitForReadyRead(10);
        if( this->bytesAvailable() > 0){
            qa = qa + this->readAll();
            res = QString::fromLocal8Bit(qa);
            timeout = QTime::currentTime().addSecs(10);
        }
        if( QTime::currentTime() > timeout){
            ok = false;
            if( m_enableVLog ){
                m_visualLog->appendPlainText(QString("Timed out G-Code: %1").arg(command));
                if( res.length() > 0) m_visualLog->appendPlainText(QString("Received: %1").arg(res));
                m_visualLog->appendPlainText("Closing Port. Please restart comms");
                m_portOpen = false;
                // attempt 1 port restart
                ok = this->findPort();
            }
            break;
        }
    }
    // re-arm the monitor
    if( sawRefill ) this->sendGCodeCommand("M412 S1");
    if( m_enableVLog && ok ){
        if( command.left(4) != "M105" )
            m_visualLog->appendPlainText(QString("From CNC: %1").arg(res.left(res.length()-1)));
    }
    if( response != nullptr ) *response = res;
    if( !ok ) ok = findPort();
    m_expected = false;
    m_inSendGCode = false;
    return ok;
}

#include <QString>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>
#include <QVector>

bool CNCComManager::getAxisPositions(CNCAxisPositions *axisPositions){
    bool ok= false;

    if( !m_portOpen ){
        if( m_enableVLog ) m_visualLog->appendPlainText("PORT CLOSED: getAxisPositions");
        return false;
    }
    if( m_inSendGCode ){
        axisPositions->a = lastValidPosition.a;
        axisPositions->b = lastValidPosition.b;
        axisPositions->c = lastValidPosition.c;
        axisPositions->x = lastValidPosition.x;
        axisPositions->y = lastValidPosition.y;
        axisPositions->z = lastValidPosition.z;
        axisPositions->u = lastValidPosition.u;
        axisPositions->v = lastValidPosition.v;
        return true;
    }

    m_expected = true;
    QByteArray qa = QString("G92\n").toLocal8Bit();
    QString r;
    this->clear(QSerialPort::AllDirections);
    ok = this->write(qa.data(), qa.length());
    this->flush();

    qa.clear();
    QTime timeout = QTime::currentTime().addSecs(3);
    while( !r.contains("ok\n") ){
        this->waitForReadyRead(10);
        if( this->bytesAvailable() ){
            qa = qa + this->readAll();
            r = QString::fromLocal8Bit(qa);
            timeout = QTime::currentTime().addSecs(3);
        }
        if( QTime::currentTime() > timeout) ok = false;
    }
    if( !ok ) return false;
    if( m_enableVLog ) m_visualLog->appendPlainText(r);
    QRegularExpression re("([A-Z]):(-?\\d+\\.\\d+)");
    QVector<float> values;
    QRegularExpressionMatchIterator i = re.globalMatch(r);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        values.append(match.captured(2).toFloat());
        if (values.size() >= 8) {
            break; // Only extract the first 8 values
        }
    }
    if(values.length() > 0)
        lastValidPosition.x = axisPositions->x = values[0];
    if(values.length() > 1)
        lastValidPosition.y = axisPositions->y = values[1];
    if(values.length() > 2)
        lastValidPosition.z = axisPositions->z = values[2];
    if(values.length() > 3)
        lastValidPosition.a = axisPositions->a = values[3];
    if(values.length() > 4)
        lastValidPosition.b = axisPositions->b = values[4];
    if(values.length() > 5)
        lastValidPosition.c = axisPositions->c = values[5];
    if(values.length() > 6)
        lastValidPosition.u = axisPositions->u = values[6];
    if(values.length() > 7)
        lastValidPosition.v = axisPositions->v = values[7];
    m_expected = false;
    return ok;
}

struct TemperatureInfo {
    float_t current;
    float_t target;
};


bool CNCComManager::getTemperatures(CNCTemperatures *temps){
    bool ok= false;
    QString response;

    if( !m_portOpen ){
        if( m_enableVLog ) m_visualLog->appendPlainText("PORT CLOSED: getTemperatures");
        return false;
    }
    if( m_inSendGCode || m_inUnexpected || m_inRefill ){
        // return the last known values
        temps->binderTemp = lastValidTemps.binderTemp;
        temps->buildPlateTemp = lastValidTemps.buildPlateTemp;
        temps->feedPlateTemp = lastValidTemps.feedPlateTemp;
        temps->ovenTemp = lastValidTemps.ovenTemp;
        temps->overheadTemp = lastValidTemps.overheadTemp;
        return true;
    }
    m_expected = true;
    if (this->sendGCodeCommand(QString("M105\n"), &response)){
        // Decode the complicated response
        QRegularExpression tempRegex(R"((T\d*|B):([-+]?[0-9]*\.?[0-9]+)\s*/\s*([-+]?[0-9]*\.?[0-9]+))");
        QRegularExpressionMatchIterator i = tempRegex.globalMatch(response);

        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            QString label = match.captured(1);
            float_t currentTemp = match.captured(2).toFloat();
            if( label == "B" ) lastValidTemps.buildPlateTemp = temps->buildPlateTemp = currentTemp;
            if( label == "T0" ) lastValidTemps.overheadTemp = temps->overheadTemp = currentTemp;
            if( label == "T1") lastValidTemps.binderTemp = temps->binderTemp = currentTemp;
            if( label == "T2") lastValidTemps.ovenTemp = temps->ovenTemp = currentTemp;
            if( label == "T3") lastValidTemps.feedPlateTemp = temps->feedPlateTemp = currentTemp;
        }
        ok = true;
    }
    else {
        ok = false;
        if( m_enableVLog ) m_visualLog->appendPlainText("getTemperatures failed, response = " + response);
    }
    m_expected = false;
    return ok;
}
void CNCComManager::handleUnexpected(){
    if( m_expected ) return;
    if( m_inUnexpected ) return;
    m_inUnexpected = true;
    QByteArray qa;
    uint32_t loops = 0;
    QTime inMin;
    while( this->bytesAvailable() ){
        if( this->canReadLine() ){
            qa = this->readLine();
            if( QString::fromLocal8Bit(qa).contains("REFILL")) break;
            else if( QString::fromLocal8Bit(qa).contains("echo:busy: processing") )  continue;
            else if( QString::fromLocal8Bit(qa).contains("echo:busy: paused for user") )  continue;
            else if( QString::fromLocal8Bit(qa).contains("V:0")) break;
            inMin = QTime::currentTime().addMSecs(2);
            while (inMin > QTime::currentTime()) QApplication::processEvents();
        }
        else loops++;
        QApplication::processEvents();
        if( loops > 1000 ){
            qa = this->readAll();
            qDebug() << "No Line: " << QString::fromLocal8Bit(qa);
            break;
        }
    }
    if( QString::fromLocal8Bit(qa).contains("REFILL")) m_inRefill = true;
    else if( m_inRefill && QString::fromLocal8Bit(qa).contains("V:0")){
        if( !m_inSendGCode ) this->sendGCodeCommand("M412 S1");
        m_inRefill = false;
    }
    else if( m_enableVLog && !m_inRefill) m_visualLog->appendPlainText("Unexpected: " + QString::fromLocal8Bit(qa));
    m_inUnexpected = false;
    return;
}
bool CNCComManager::findPort()
{//
    bool ok = false;
    if( this->isOpen()){
        m_portOpen = false;
        this->close();
    }
    QList<QSerialPortInfo> availablePorts = QSerialPortInfo::availablePorts();
    for( const QSerialPortInfo &port : availablePorts ){
        if( port.description().contains("MARLIN") ){
            this->setPortName(port.portName());
            this->setBaudRate(QSerialPort::Baud115200);
            this->setParity(QSerialPort::NoParity);
            this->setDataBits(QSerialPort::Data8);
            this->setStopBits(QSerialPort::OneStop);
            m_portOpen = this->open(QIODeviceBase::ReadWrite);
            break;
        }
    }
    if( this->isOpen())
    {   // do a quick check to make sure it actually talks
        QByteArray qa = QString("G1\n").toLocal8Bit();
        m_expected = true;
        this->clear();
        ok = this->write(qa.data(), qa.length());
        this->flush();
        if( ok ){
            this->waitForReadyRead(1000);
            qa.clear();
            qa = this->readAll();
            if( !QString::fromLocal8Bit(qa).contains("ok\n")) {
                this->close();
                m_portOpen = false;
                m_expected = false;
                return false;
            }
        }
        m_expected = false;
        return true;
    }
    else{
        m_expected = false;
        return false;
    }
}

ExecutiveEngine::ExecutiveEngine()
{
    return;
    GCodeFile::clearFile( &m_curGCode );
    m_curGCodeLC = 0;

}
ExecutiveEngine::~ExecutiveEngine()
{
    //loadedFiles.clear();
}

bool ExecutiveEngine::setImageStackPath(const QString &imageStackPath , uint32_t colors)
{
    if( imageStackPath == "" ) return false;

    QString test;
    m_colors = colors;
   /* if( colors == 1){
        test = imageStackPath + QString("/layer_%1.png").arg(0);
    }
    else { */
        test = imageStackPath + QString("/layer_%1_color_%2.png").arg(0).arg(0);
   /* } */
    QFile ftest(test);

    if (ftest.open(QIODevice::ReadOnly)){
        ftest.close();
        m_imageStackPath = imageStackPath;
        if( m_visualLog != nullptr ) m_visualLog->appendPlainText(QString("Image Path Set to: %1").arg(m_imageStackPath));
        if( m_imagePreview != nullptr )
            showLayer(0);
        return true;
    }
    else
        return false;
}

bool ExecutiveEngine::setGCodePath(const QString &gcodePath){
    m_gcodePath = gcodePath;
    if( m_visualLog != nullptr ) m_visualLog->appendPlainText(QString("GCODE Path Set to: %1").arg(m_gcodePath));
    return true;
}

bool ExecutiveEngine::setImagePreview(QLabel *previewWindow)
{
    if( previewWindow == nullptr) return false;
    m_imagePreview = previewWindow;
    return true;
}

bool ExecutiveEngine::attachInitializeList(const QStringList &initList)
{
    m_initializeList.clear();
    m_initializeList = initList;
    return true;
}

bool ExecutiveEngine::addGCodeFile(const QString &gcodeFile){
    // see if file is loaded
  /*  for( GCodeFile file : loadedFiles){
        if( file.fileName == gcodeFile )
            return true;
    }
    GCodeFile gf;
    if( GCodeFile::loadGCodeFile( gf, gcodeFile)){
        loadedFiles.append(gf);
        return true;
    } */
    return false;
}

bool ExecutiveEngine::attachCommandList(const QStringList &commandList)
{
    m_layerCommandList.clear();
    m_layerCommandList = commandList;
    return true;
}

bool ExecutiveEngine::attachExceptionCommandList(const QStringList &exceptionCommandList)
{
    m_layerExceptionCommandList.clear();
    m_layerExceptionCommandList = exceptionCommandList;
    return true;
}

bool ExecutiveEngine::attachFinalizeList(const QStringList &finalizeList)
{
    m_finalizeList.clear();
    m_finalizeList = finalizeList;
    return true;
}

bool ExecutiveEngine::setLayerZeroZ(float zeroZ)
{
    m_layerZeroZ = zeroZ;
    m_layerZeroZSet = true;
    if( m_visualLog != nullptr ) m_visualLog->appendPlainText(QString("Zero Layer Set to: %1").arg(zeroZ));
    return true;
}
bool ExecutiveEngine::printLayerPassesM(uint32_t layerID){
    return false;
}

bool ExecutiveEngine::positionZ(){
    // This function will position the Z-Axis by examing the value encoded in the bitmap in this file
    // The value is not color independent. It's conceivable that a user could load two STL files of different heights
    QImageReader imgR;
    QString fn;
    bool cont = false;
    int i = 0;

    for( i=0; i<m_colors; i++ ) {
        fn = m_imageStackPath + QString("/color%1.ini").arg(i);
        QSettings fs( fn, QSettings::IniFormat );
        fs.beginGroup(QString("Color-%1-Info").arg(i));
        if( fs.value("Layers").toInt() > m_layerCounter ){ cont = true; break; }
    }
    if( !cont ){
        if(m_visualLog) m_visualLog->appendPlainText("positionZ:no more layers");
        return false;
    }

    fn = m_imageStackPath +  QString("/layer_%1RIP_color_%2.png").arg(m_layerCounter).arg(i);
    imgR.setFileName(fn);
    float_t newZ = m_layerZeroZ - imgR.text("expectZOffset").toFloat();
    // Position the Z-Axis,
    m_cnc->sendGCodeCommand("G91");
    m_cnc->sendGCodeCommand(QString("G1 Z%1 F%2").arg(-1.0f*m_recoatSettings->z_retract).arg(m_recoatSettings->z_move_speed));
    m_cnc->sendGCodeCommand("G90");
    return m_cnc->sendGCodeCommand(QString("G1 Z%1 F%2").arg(newZ).arg(m_recoatSettings->z_move_speed));

}
bool ExecutiveEngine::positionZF(){
    // This function will position the Z and F Axis by examing the value encoded in the bitmap in this file
    // The value is not color independent. It's conceivable that a user could load two STL files of different heights
    QImageReader imgR;
    QString fn;
    bool cont = false;
    bool ok = true;
    int i = 0;
    CNCAxisPositions ap{};
    ok = m_cnc->getAxisPositions(&ap);

    for( i=0; i<m_colors; i++ ) {
        fn = m_imageStackPath + QString("/color%1.ini").arg(i);
        QSettings fs( fn, QSettings::IniFormat );
        fs.beginGroup(QString("Color-%1-Info").arg(i));
        if( fs.value("Layers").toInt() > m_layerCounter ){ cont = true; break; }
    }
    if( !cont ){
        if(m_visualLog) m_visualLog->appendPlainText("positionZ:no more layers");
        return false;
    }

    fn = m_imageStackPath +  QString("/layer_%1RIP_color_%2.png").arg(m_layerCounter).arg(i);
    imgR.setFileName(fn);
    float_t newZ = m_layerZeroZ - imgR.text("expectZOffset").toFloat();
    // Position the Z-Axis,
    ok = ok & m_cnc->sendGCodeCommand("G91");
    ok = ok & m_cnc->sendGCodeCommand(QString("G1 Z%1 F%2").arg(-1.0f*m_recoatSettings->z_retract).arg(m_recoatSettings->z_move_speed));
    ok = ok & m_cnc->sendGCodeCommand("G90");
    ok = ok & m_cnc->sendGCodeCommand(QString("G1 Z%1 F%2").arg(newZ).arg(m_recoatSettings->z_move_speed));
    float_t newF = ap.u + m_recoatSettings->feed_powder_ratio * imgR.text("sliceThickness").toFloat(&ok);
    ok = ok & m_cnc->sendGCodeCommand(QString("G1 U%1 F%2").arg(newF).arg(m_recoatSettings->z_move_speed));

    return ok;
}
bool ExecutiveEngine::printLayerPasses(uint32_t layerID){

    bool bOk = true;
//    static uint32_t lastLine=0;
    QImage ripd_img;

    QString fn_rip;
    if( !m_layerDataSent ){
   /*     if( m_colors == 1 ){
            fn_rip = m_imageStackPath +  QString("/layer_%1RIP.png").arg(layerID);
            imgReader.setFileName(fn_rip);
            ripd_img = imgReader.read();
            imgReader.text("expectZ");
            //qDebug() << "RIP: " << fn_rip;
            ripd_img.load(fn_rip);
            if( !m_phc->makeData(ripd_img) ){
                m_visualLog->appendPlainText("m_phc->makeData() FAILED");
                return false;
            }
            m_phc->sendData();
            m_layerDataSent = true;
        }
        else{ */
            for( uint32_t i=0; i<m_colors; i++ ){
                fn_rip = m_imageStackPath +  QString("/layer_%1RIP_color_%2.png").arg(layerID).arg(i);
                ripd_img.load(fn_rip);
                if (!m_phc->makeData(ripd_img)) {
                    m_visualLog->appendPlainText("m_phc->makeData() FAILED");
                    return false;
                }
                m_phc->sendData(i);
                if( !m_use2binders->isChecked() ) i = m_colors;
                m_layerDataSent = true;
            }
       /* } */
    }

    QString fn_gcode = m_imageStackPath + QString("/layer%1.gcode").arg(layerID);
    GCodeFile printfile;
    GCodeFile::loadGCodeFile( &printfile, fn_gcode );
    //qDebug() << "Loaded " << printfile.instructions.length() << " lines from " << fn_gcode;
    while( m_printLayerPC < printfile.instructions.length()){
        QApplication::processEvents();
        if( m_pauseReq ){
            m_visualLog->appendPlainText(QString("Pause printLayerPasses %1 at %2")
                                         .arg(fn_gcode)
                                         .arg(m_printLayerPC));
            return false;
        }
        if( printfile.instructions[m_printLayerPC].length() == 0 ){
            m_printLayerPC ++;
            continue;
        }
        bOk=decodeInstruction( printfile.instructions[m_printLayerPC]);
        if( bOk ){ // Exec Command
            m_printLayerPC++;
        }
        else {
            if( m_visualLog != nullptr )
                m_visualLog->appendPlainText(QString("decodeInstruction failed: %1").arg(printfile.instructions[m_printLayerPC]));
            break;
        }
    }
    // See if we're exiting successfully
    if( bOk )
        m_printLayerPC = 0;

    return bOk;
}

bool ExecutiveEngine::incrementLayerCounter()
{   // This function checks to see if any of the recoating parameters need to be updated
    m_layerCounter++;
    m_layerDataSent = false;

    bool powerChanged = false;
    bool dispSpdChanged = false;
    bool rollingSpdChanged = false;
    bool heatSpdChanged = false;
    bool anyChanged = false;

    QList<uint32_t> keepPwr{};
    QList<float_t> keepDspSpd{};
    QList<float_t> keepRollingSpd{};
    QList<float_t> keepHeatSpd{};
    if( m_laydownSetting->pulseWidths.length() >= 2){
        for( int i=0; i<m_laydownSetting->pulseWidths.length(); i++){
            if( !(i&1)){
                if( (m_laydownSetting->pulseWidths[i]-1) == m_layerCounter ){
                    uint32_t newPA = ceil(m_laydownSetting->pulseWidths[i+1]/0.2f);
                    m_phc->sendText(QString("PA,%1").arg(newPA));
                }
            }
        }
    }
    if( m_enablePowerScaling->isChecked())
    {
        if( m_layerCounter % m_reducePowerEvery->text().toInt() == 0)
        {
            uint32_t newPower = m_basePower - ((m_reducePowerBy->text().toInt())*(m_layerCounter/m_reducePowerEvery->text().toInt()));
            m_powerItem->setText(QString("%1").arg(newPower));
            m_generateRecoat->clicked();
            QApplication::processEvents();
            SaveGCodeFile(m_recoatList, &m_gcodePath);
        }
    }
    // Check for update Dispenser Power
    if( m_recoatSettings->disp_powers.length() >= 2 )
    {
        uint32_t newPower = 0;
        uint32_t oldPower = m_recoatSettings->disp_power;
        for( uint32_t i=0; i<m_recoatSettings->disp_powers.length(); i+=2 )
        {
            if( (m_layerCounter+1) >= m_recoatSettings->disp_powers[i] )
                newPower = m_recoatSettings->disp_powers[i+1];
            if( newPower != oldPower )
            {
                powerChanged = true;
                anyChanged = true;
                keepPwr = m_recoatSettings->disp_powers;
                m_powerItem->setText(QString("%1").arg(newPower));
            }
        }
    }
    // Check for update Dispenser Speed
    if( m_recoatSettings->disp_speeds.length() >= 2 )
    {
        float_t newSpeed = 0;
        float_t oldSpeed = m_recoatSettings->disp_speed;
        for( uint32_t i=0; i<m_recoatSettings->disp_speeds.length(); i+=2 )
        {
            if( (m_layerCounter+1) >= m_recoatSettings->disp_speeds[i] )
                newSpeed = m_recoatSettings->disp_speeds[i+1];
            if( newSpeed != oldSpeed )
            {
                dispSpdChanged = true;
                anyChanged = true;
                keepDspSpd = m_recoatSettings->disp_speeds;
                m_dispSpeedItem->setText(QString("%1").arg(newSpeed/60.0f));
            }
        }
    }
    // Check for update Heating Speed
    if( m_recoatSettings->heat_speeds.length() >= 2 )
    {
        float_t newSpeed = 0;
        float_t oldSpeed = m_recoatSettings->heat_speed;
        for( uint32_t i=0; i<m_recoatSettings->heat_speeds.length(); i+=2 )
        {
            if( (m_layerCounter+1) >= m_recoatSettings->heat_speeds[i] )
                newSpeed = m_recoatSettings->heat_speeds[i+1];
            if( newSpeed != oldSpeed )
            {
                heatSpdChanged = true;
                anyChanged = true;
                keepHeatSpd = m_recoatSettings->heat_speeds;
                m_heatSpeedItem->setText(QString("%1").arg(newSpeed/60.0f));
            }
        }
    }
    // Check for update Rolling Speed
    if( m_recoatSettings->rolling_speeds.length() >= 2 )
    {
        float_t newSpeed = 0;
        float_t oldSpeed = m_recoatSettings->rolling_speed;
        for( uint32_t i=0; i<m_recoatSettings->rolling_speeds.length(); i+=2 )
        {
            if( (m_layerCounter+1) >= m_recoatSettings->rolling_speeds[i] )
                newSpeed = m_recoatSettings->rolling_speeds[i+1];
            if( newSpeed != oldSpeed )
            {
                rollingSpdChanged = true;
                anyChanged = true;
                keepRollingSpd = m_recoatSettings->rolling_speeds;
                m_rollingSpeedItem->setText(QString("%1").arg(newSpeed/60.0f));
            }
        }
    }

    if( anyChanged ){
        m_generateRecoat->clicked();
        QApplication::processEvents();
        SaveGCodeFile(m_recoatList, &m_gcodePath);
        if( powerChanged ){
            m_recoatSettings->disp_powers = keepPwr;
            QString nums="";
            for( uint32_t i=0; i<m_recoatSettings->disp_powers.length(); i++)
            {
                nums = nums + QString("%1").arg(m_recoatSettings->disp_powers[i]);
                if( i < m_recoatSettings->disp_powers.length()-1)
                    nums = nums + ",";
            }
            m_powerItem->setText(nums);
            m_enablePowerScaling->setChecked(false);
        }
        if( dispSpdChanged ){
            m_recoatSettings->disp_speeds = keepDspSpd;
            QString nums="";
            for( uint32_t i=0; i<m_recoatSettings->disp_speeds.length(); i++)
            {
                nums = nums + QString("%1").arg(m_recoatSettings->disp_speeds[i]);
                if( i < m_recoatSettings->disp_speeds.length()-1)
                    nums = nums + ",";
            }
            m_dispSpeedItem->setText(nums);
        }
        if( heatSpdChanged ){
            m_recoatSettings->heat_speeds = keepHeatSpd;
            QString nums="";
            for( uint32_t i=0; i<m_recoatSettings->heat_speeds.length(); i++)
            {
                nums = nums + QString("%1").arg(m_recoatSettings->heat_speeds[i]);
                if( i < m_recoatSettings->heat_speeds.length()-1)
                    nums = nums + ",";
            }
            m_heatSpeedItem->setText(nums);
        }

    }
    return true;
}

bool ExecutiveEngine::runInitializeList()
{
    bool ok = false;
    // Run the init list
    if( m_initializeList.length() > 0 ){
        uint32_t oldPC = m_programCounter;
        m_programCounter = 0;
        ok = executeCommandList(m_initializeList);
        m_programCounter = oldPC;
    }
    else
        ok = true; // nothing to do
    return ok;

}
bool ExecutiveEngine::runFinalizeList(){
    bool ok = false;
    // Run the finalize list
    if( m_finalizeList.length() > 0 ){
        uint32_t oldPC = m_programCounter;
        m_programCounter = 0;
        ok = executeCommandList(m_finalizeList);
        m_programCounter = oldPC;
    }
    else
        ok = true; // nothing to do

    return ok;
}

bool ExecutiveEngine::setPHC(PHCCommManager *phc)
{
    m_phc = phc;
    return true;
}

bool ExecutiveEngine::setCNC(CNCComManager *cnc)
{
    m_cnc = cnc;
    return true;
}

bool ExecutiveEngine::executeCurrentLayer()
{
    bool bOK = false;
    static uint32_t lastCmdIndex=0;
    while ( lastCmdIndex < m_layerCommandList.length() ){
        QString qs = m_layerCommandList[lastCmdIndex];
        if( qs.left(1) == "M" || qs.left(1) == "G") // Simple GCode to dispatch
        {
            bOK = m_cnc->sendGCodeCommand(qs);
            if( bOK ){
                lastCmdIndex++;
                continue;
            }
            else
                break;
        } // GCode Command
        else if( qs.left(2) == ";:" ) {
            // EXEC level command
            if( qs.left(6) == ";:RUN:"){
                qs = m_imageStackPath + qs.mid(6);
                if( m_curGCode.instructions.length() == 0 ){
                    GCodeFile::loadGCodeFile(&m_curGCode, qs);
                    m_curGCodeLC = 0;
                }
                for( ; m_curGCodeLC < m_curGCode.instructions.length(); ){
                    bOK = m_cnc->sendGCodeCommand(m_curGCode.instructions[m_curGCodeLC]);
                    if( bOK ){
                        m_curGCodeLC++;
                    }
                    else
                        break;
                }
                if( bOK ){
                    lastCmdIndex++;
                    m_curGCodeLC = 0;
                    continue;
                }
                else
                    break;
            } // If RUN command
        } // EXEC Command
    }
    return bOK;
}

bool ExecutiveEngine::executeCommandList(const QStringList &cmdList, bool async )
{
    bool bOk = true;
    uint32_t oldPC = m_programCounter;
    if( async ) m_programCounter = 0;
    while( m_programCounter < cmdList.count() ){
        QString cmd = cmdList[m_programCounter];
        if( m_currInstLabel != nullptr ) m_currInstLabel->setText(QString("Exec:%1").arg(cmd));
        QApplication::processEvents();
        if( m_pauseReq ){
            if( async ) m_programCounter = oldPC;
            return false;
        }
        if( cmd.left(2) == ";:" ){ // EXEC Command
            if( cmd.left(6) == ";:RUN:"){     // Load and run a GCODE file
                QString fname = m_gcodePath + "/" + cmd.mid(6,cmd.length()-7);
                bOk = executeGCodeFile( fname );
            }
            else if( cmd.left(6) == ";:PHC:") { // Redirect command to PHC
                bOk = m_phc->sendText(cmd.mid(6));
            }
            else if( cmd == ";:PRINTLAYER:"){ // Print the layer passes
                bOk = printLayerPasses(m_layerCounter);
            }
            else if( cmd == ";:INCLAYER:"){
                bOk = incrementLayerCounter();
            }
            else if( cmd == ";:POSITION_Z:")
                bOk = positionZ();
            else if( cmd == ";:POSITION_ZF:")
                bOk = positionZF();
        }
        else if( cmd.left(1) == "G" || cmd.left(1) == "M" ){ // GCode for controller
            bOk = m_cnc->sendGCodeCommand(cmd);
        }
        if( !bOk ){
            if( async ) m_programCounter = oldPC;
            break; // Dump if anything doesn't work
        }
        else m_programCounter++;
    }
    if( m_programCounter >= cmdList.count()) m_programCounter = 0;
    return bOk;
}

bool ExecutiveEngine:: executeGCodeFile(const QString &fileName)
{

    bool bOk = true;
    GCodeFile gfile{};
    bOk = GCodeFile::loadGCodeFile( &gfile, fileName);
    if( !bOk ) {
        if( m_visualLog != nullptr ) m_visualLog->appendPlainText(QString("exGCode Failed to load: %1").arg(fileName));
        return false;
    }

    //if( m_visualLog != nullptr ) m_visualLog->appendPlainText(QString("exGCode LoadGCode: %1 : %2").arg(fileName).arg(gfile.instructions.length()));
    while ( m_gcodePC < gfile.instructions.length() )
    {
        QApplication::processEvents();
        if( m_pauseReq ){
            if( m_visualLog != nullptr ) m_visualLog->appendPlainText(QString("Pausing GCODE:%1 at #%2").arg(fileName).arg(m_gcodePC));

            return false;
        }
        if( gfile.instructions[m_gcodePC].length() == 0 ){
            m_gcodePC++;
            continue;
        }
        if( gfile.instructions[m_gcodePC].left(1) == "G" || gfile.instructions[m_gcodePC].left(1) == "M")
            bOk = m_cnc->sendGCodeCommand(gfile.instructions[m_gcodePC]);
        else if( gfile.instructions[m_gcodePC].left(6) == ";:PHC:") // redirect
            bOk = m_phc->sendText(gfile.instructions[m_gcodePC].mid(6));
        else {
            if( m_visualLog != nullptr )
                m_visualLog->appendPlainText(QString("Bad GCODE:%1").arg(gfile.instructions[m_gcodePC]));
            bOk = false;
        }
        if( !bOk ){
            if( m_visualLog != nullptr ) m_visualLog->appendPlainText(QString("Command Failed: %1").arg(gfile.instructions[m_gcodePC]));
            break;
        }
        else m_gcodePC++;
    }
    if( m_gcodePC >= gfile.instructions.length() ) m_gcodePC = 0;
    return bOk;

}
bool ExecutiveEngine::asynchExecuteGCodeFile(const QString &fileName)
{

    bool bOk = true;
    GCodeFile gfile{};
    uint32_t curId=0;
    bOk = GCodeFile::loadGCodeFile( &gfile, fileName);
    if( !bOk ) {
        if( m_visualLog != nullptr ) m_visualLog->appendPlainText(QString("asynchEx Failed to load: %1").arg(fileName));
        return false;
    }
    if( m_visualLog != nullptr ) m_visualLog->appendPlainText(QString("asynchEx LoadGCode: %1 : %2").arg(fileName).arg(gfile.instructions.length()));
    while ( curId < gfile.instructions.length() )
    {
        if( gfile.instructions[curId].length() == 0 ){
            curId++;
            continue;
        }
        if( gfile.instructions[curId].left(1) == "G" || gfile.instructions[curId].left(1) == "M")
            bOk = m_cnc->sendGCodeCommand(gfile.instructions[curId]);
        else if( gfile.instructions[curId].left(6) == ";:PHC:") // redirect
            bOk = m_phc->sendText(gfile.instructions[curId].mid(6));
        else {
            if( m_visualLog != nullptr )
                m_visualLog->appendPlainText(QString("asynchEx Bad GCODE:%1").arg(gfile.instructions[curId]));
            bOk = false;
        }
        if( !bOk ){
            if( m_visualLog != nullptr ) m_visualLog->appendPlainText(QString("asynchEx Command Failed: %1").arg(gfile.instructions[curId]));
            break;
        }
        else curId++;
    }

    return bOk;

}

bool ExecutiveEngine::run()
{
    bool ok = true;

    QTime layerTimer = QTime::currentTime();
    QString layerTime;
    uint64_t timeDiff;

    if( !this->valid() )return false;

    if( !m_isInitialized && m_useInitList->isChecked() ){
        ok = m_isInitialized = runInitializeList();
        if( !ok ){
            m_visualLog->appendPlainText("ERROR -- Initialize Failed -- ERROR");
            return false;
        }
    }

    QApplication::processEvents();
    if( m_pauseReq ) return false;

    if(m_enablePowerScaling->isChecked() && m_basePower==0)
        m_basePower = m_recoatSettings->disp_power;

    while( m_layerCounter < m_layersToPrint){
        if( m_pauseReq ) return false;

        showLayer(m_layerCounter);
        if( m_LCLabel != nullptr )
            m_LCLabel->setText(QString("%1").arg(m_layerCounter+1));
        CNCAxisPositions ap{};
        this->m_cnc->getAxisPositions(&ap);
        m_visualLog->appendPlainText(QString("Start Layer %1 at Z = %2").arg(m_layerCounter+1).arg(ap.z));
        if( !m_inException )
            ok = this->executeCommandList(m_layerCommandList);
        if( ok ){
            if( m_layerCounter % this->m_exceptionFrequency == 0) {
                m_inException = true;
                ok = this->executeCommandList(m_layerExceptionCommandList);
                if( ok ) m_inException = false;
            }
        }
        timeDiff = layerTimer.msecsTo(QTime::currentTime())/1000; // in seconds
        uint32_t minutes = timeDiff / 60;
        uint32_t seconds = timeDiff - (minutes*60);
        uint32_t hours = 0;
        //layerTimer = QTime::currentTime() - layerTimer;
        layerTime = QString("Last Layer Time was %1:%2").arg(minutes).arg((uint)seconds,(int)2,(int)10, (QChar)u'0');
        layerTimer = QTime::currentTime();
        m_visualLog->appendPlainText(layerTime);
        timeDiff = (m_layersToPrint - m_layerCounter)*timeDiff;
        hours = timeDiff / 3600;
        minutes = (timeDiff - (hours * 3600))/60;
        seconds = timeDiff - (hours*3600) - (minutes*60);
        layerTime = QString("Time Remaining is %1:%2:%3").arg(hours).arg((uint)minutes,(int)2,(int)10,(QChar)u'0').arg((uint)seconds,(int)2,(int)10,(QChar)u'0');
        m_visualLog->appendPlainText(layerTime);
        QApplication::processEvents();
        if( !ok ) return false;
    }

    if( ok ){
        if( m_useFinalList->isChecked() || m_inFinalize ){
            if( !m_inFinalize ) {
                m_inFinalize = true;
                CNCAxisPositions pos{};
                if( m_cnc->getAxisPositions(&pos) ){
                    int retractsNeeded = floor(pos.z/m_laydownSetting->layerThickness);
                    m_finalizeList.append(QString("G91 ; RELATIVE MODE"));
                    //m_cnc->sendGCodeCommand(QString("G91 ; RELATIVE MODE"));
                    for( uint32_t i=0; i<retractsNeeded; i++ ){
                        m_finalizeList.append(QString("G1 Z%1 F%2 ; backoff").arg(-1.0f*m_recoatSettings->z_retract).arg(m_recoatSettings->z_move_speed));
                        m_finalizeList.append(QString("G1 Z%1 F%2 ; liftup").arg(m_recoatSettings->z_retract-m_laydownSetting->layerThickness).arg(m_recoatSettings->z_move_speed));
                    }
                    m_finalizeList.append(QString("G90 ; ABSOLUTE MODE"));
                    m_finalizeList.append(QString("G1 Z0 F%1 ; Go to zero").arg(m_recoatSettings->z_move_speed));
                    m_finalizeList.append(QString("G1 U0 F%1 ; Feed straight to zero").arg(m_recoatSettings->z_move_speed));
                }
                else ok = false;
            }
            if( ok )
                ok = runFinalizeList();
            if( ok ) m_inFinalize = false;
        }
    }

    return ok;
}

bool ExecutiveEngine::resetLayer()
{
    // Reset the SM variables related to the current layer
    m_printLayerPC = 0;
    m_programCounter = 0;
    m_layerDataSent = false;
    m_gcodePC = 0;
    m_inException = false;
    m_inFinalize = false;
    return true;

}

bool ExecutiveEngine::resetJob()
{
    m_layerCounter = 0;
    if( m_enablePowerScaling->isChecked() && (m_basePower!=0)){
        m_powerItem->setText(QString("%1").arg(m_basePower));
        m_basePower = 0;
    }
    showLayer(m_layerCounter);
    if( m_LCLabel != nullptr )
        m_LCLabel->setText(QString("%1").arg(m_layerCounter+1));
    m_layerZeroZ = 0.0f;
    m_layerZeroZSet = false;
    return resetLayer();
}
bool ExecutiveEngine::setCurrentLayer( uint32_t newCurLayer ){
    if( !valid() )return false;
    if( newCurLayer > m_layersToPrint )return false;
    m_layerCounter = newCurLayer-1;
    return true;
}

bool ExecutiveEngine::pause()
{
    m_pauseReq = true;
    return true;
}

bool ExecutiveEngine::unpause()
{
    m_pauseReq = false;
    return true;
}

bool ExecutiveEngine::valid()
{
    m_isValid = true;
    if( m_cnc == nullptr)
    {
        m_isValid = false;
        if( m_visualLog != nullptr ) m_visualLog->appendPlainText("No CNC Attached");
    }
    if( m_phc == nullptr){
        m_isValid = false;
        if( m_visualLog != nullptr ) m_visualLog->appendPlainText("No PHC Attached");
    }
    if( m_imageStackPath == "" ){
        m_isValid = false;
        if( m_visualLog != nullptr ) m_visualLog->appendPlainText("No Image Stack Selected");
    }
    if( m_layerCommandList.length() == 0 ){
        m_isValid = false;
        if( m_visualLog != nullptr ) m_visualLog->appendPlainText("No Commands in Script");
    }
    if( !m_layerZeroZSet || m_layerZeroZ == 0.0f ){
        m_isValid = false;
        if( m_visualLog != nullptr ) m_visualLog->appendPlainText("Zero Z Not Locked");
    }

    return   m_isValid;

}
bool ExecutiveEngine::setBuildBedTemp(float newTemp){
    CNCTemperatures temps{};
    if( m_cnc->getTemperatures(&temps) ){
        if( temps.buildPlateTemp > 5.0f ) { // Thermocouple is probably connected
            if( m_cnc->sendGCodeCommand(QString("M140 S%1").arg(newTemp)) )
                return true;
        }
    }
    return false;
}

bool ExecutiveEngine::setOverheadTemp(float newTemp){
    CNCTemperatures temps{};
    if( m_cnc->getTemperatures(&temps) ){
        if( temps.overheadTemp > 5.0f ) { // Thermocouple is probably connected
            if( m_cnc->sendGCodeCommand(QString("M104 T0 S%1").arg(newTemp)) )
                return true;
        }
    }
    return false;
}

bool ExecutiveEngine::setBinderTemp(float newTemp){
    CNCTemperatures temps{};
    if( m_cnc->getTemperatures(&temps) ){
        if( temps.binderTemp > 5.0f ) { // Thermocouple is probably connected
            if( m_cnc->sendGCodeCommand(QString("M104 T1 S%1").arg(newTemp)) )
                return true;
        }
    }
    return false;
}

bool ExecutiveEngine::setOvenTemp(float newTemp){
    CNCTemperatures temps{};
    if( m_cnc->getTemperatures(&temps) ){
        if( temps.ovenTemp > 5.0f ) { // Thermocouple is probably connected
            if( m_cnc->sendGCodeCommand(QString("M104 T2 S%1").arg(newTemp)) )
                return true;
        }
    }
    return false;
}

bool ExecutiveEngine::setFeedBedTemp(float newTemp){
    CNCTemperatures temps{};
    if( m_cnc->getTemperatures(&temps) ){
        if( temps.feedPlateTemp > 5.0f ) { // Thermocouple is probably connected
            if( m_cnc->sendGCodeCommand(QString("M104 T3 S%1").arg(newTemp)) )
                return true;
        }
    }
    return false;
}


bool ExecutiveEngine::decodeInstruction(const QString nextInstruction){
    bool bOk = false;
    QString qs;
    // Check to see if it's an EXEC level command, or a machine level command
    if(nextInstruction.left(2) == ";:"){// EXEC LEVEL COMMAND
        if(nextInstruction.left(6) == ";:PHC:") { // PHC Command
            bOk = m_phc->sendText(nextInstruction.mid(6));
            return bOk;
        }
        else if( nextInstruction.left(6) == ";:RUN:"){
            qs = nextInstruction.mid(6);
            qs = m_gcodePath + qs;
            GCodeFile gf;
            GCodeFile::loadGCodeFile(&gf, qs);

            return false;
        }
    }
    else if(nextInstruction.left(1) == "G" || nextInstruction.left(1) == "M"){
        // G/M-CODE to dispatch
        bOk = m_cnc->sendGCodeCommand(nextInstruction);
    }
    else{
        if( m_visualLog != nullptr ) m_visualLog->appendPlainText(QString("Bad Instruction %1").arg(nextInstruction));
        bOk = false; // unsupported command
    }
    return bOk;
}

bool ExecutiveEngine::showLayer(uint32_t layer)
{
    if( m_imageStackPath == "" ) return false;
    if( m_imagePreview == nullptr) return false;
    if( m_colors == 0) return false;
    QString fname;
/*    if( m_colors == 1) {
        fname = m_imageStackPath + QString("/layer_%1.png").arg(layer);
        QPixmap pixmap(fname);
        m_imagePreview->setPixmap(pixmap.scaled(512,512,Qt::KeepAspectRatio));
        return true;
    }
    else{ */
        QImage images[m_colors];
        QImage result;
        QPainter painter;
        for( uint32_t i=0; i<m_colors; i++){
            fname = m_imageStackPath + QString("/layer_%1_color_%2.png").arg(layer).arg(i);
            images[i].load(fname);
            if( i == 0 ){
                result = QImage(images[0].size(),images[0].format());
                painter.begin(&result);
                painter.setCompositionMode(QPainter::CompositionMode_Source);
                painter.drawImage(0,0,images[i]);
            }
            else {
                painter.setCompositionMode(QPainter::CompositionMode_Plus);
                painter.drawImage(0,0,images[i]);
            }
        }
        painter.end();
        m_imagePreview->setPixmap(QPixmap::fromImage(result).scaled(512,512,Qt::KeepAspectRatio));
        //delete painter;
        return true;
   /* } */

}

bool ExecutiveEngine::initializeJob()
{
    bool bOk = false;

    return bOk;
}
PHCCommManager::PHCCommManager() : QSerialPort(){
    //this->findPort();
}
PHCCommManager::~PHCCommManager(){
    this->close();
}

bool PHCCommManager::sendText(QString toSend, QString *response){

    if( !toSend.contains("\r\n")) toSend.append("\r\n");

    if( !m_portOpen ){
        if( m_enableVLog ){
            m_visualLog->appendPlainText(QString("PHC PORT CLOSED : sendText : %1").arg(toSend.left(toSend.length()-2)));
        }
        return false;
    }
    QByteArray qa = toSend.toLocal8Bit();
    if( this->bytesAvailable()){
        qa = this->readAll();
        if( m_visualLog != nullptr )
            m_visualLog->appendPlainText(QString::fromLocal8Bit(qa));
    }

    if( m_enableVLog ){
        m_visualLog->appendPlainText(QString("To PHC:%1").arg(toSend.left(toSend.length()-2)));
    }
    qa = toSend.toLocal8Bit();
    QString res;
    bool ok = false;

    ok = this->clear(QSerialPort::AllDirections);
    if( !ok ) {
        if( m_enableVLog ) m_visualLog->appendPlainText("PHC Clear Fail");
        return false;
    }

    ok = this->write(qa.data(), toSend.length());

    if( !ok ) {
        if( m_enableVLog ) m_visualLog->appendPlainText("PHC Write Fail");
        return false;
    }

    qa.clear();
    this->flush();

    // Stay for response a minimum of 150ms or until we get ok\n
    QTime inMin = QTime::currentTime().addMSecs(150);
    QTime outMax = QTime::currentTime().addSecs(5);
    while(!res.contains("\r\n") || (inMin>QTime::currentTime())){
        this->waitForReadyRead(10);
        qa = qa + this->readAll();
        res = QString::fromLocal8Bit(qa);
        if( QTime::currentTime() > outMax){
            ok = false;
            break;
        }
    }

    if( !ok ){
        if( m_enableVLog ){
            m_visualLog->appendPlainText(QString("Timeout. RX Buffer: %1").arg(res));
        }
    }
    else {
        if( m_enableVLog ){
            m_visualLog->appendPlainText(QString("From PHC: %1").arg(res.left(res.length()-2))); // We know it has \r\n
        }
    }

    if( response != nullptr ) *response = res;
    return ok;

}

bool PHCCommManager::makeData(QImage &img){
    const uchar *cp = img.bits();
    imageData.clear();
    uint32_t loaded=0;
    uint32_t byteLoad = img.width() * img.height() / 8;

    for( int i=0 ; i<byteLoad; i++){
        if( currentModule == Q_CLASS_1S )
            imageData.append(*cp++);
        else if( currentModule == Q_CLASS_2S )
            imageData.append(*cp++);
        else if( currentModule == Q_CLASS_SG )
            imageData.append(*cp++);
        else if( currentModule == S_CLASS  )
            imageData.append(BitReverseTable256[*cp++]);
        else {
            m_visualLog->appendPlainText("ERROR: INVALID PRINTHEAD SELECTED");
            return false;
        }
        loaded++;
    }
    crc thisCRC = crcFast( (unsigned char *)imageData.data(), imageData.length());
    //qDebug() << "CRC = " << thisCRC;
    //qDebug() << "loaded " << loaded << " bytes in makeData()";
    return true;
}
bool PHCCommManager::setModuleType( PH_TYPES module )
{
    currentModule = module;
    return true;
}
bool PHCCommManager::findPort()
{
    QList<QSerialPortInfo> availablePorts = QSerialPortInfo::availablePorts();
    for( const QSerialPortInfo &port : availablePorts ){
        if( port.description().contains("Pico") ){
            this->setPortName(port.portName());
            this->setBaudRate(QSerialPort::Baud115200);
            this->setParity(QSerialPort::NoParity);
            this->setDataBits(QSerialPort::Data8);
            this->setStopBits(QSerialPort::OneStop);
            m_portOpen = this->open(QIODeviceBase::ReadWrite);
            break;
        }
    }
    return m_portOpen;

}

bool PHCCommManager::setVisualLog(QPlainTextEdit *log)
{
    return (m_visualLog = log);
}

void PHCCommManager::dumpData()
{
    // Dump the data to the visual log
    if( m_visualLog != nullptr ){
        QDataStream imageDataStream(imageData);
        imageDataStream.setByteOrder(QDataStream::LittleEndian);
        uint32_t dat[4];
        QString dump="";
        for( int j = 0; j<imageData.length(); j+=16 ){
            dump = QString("%1:").arg((uint32_t)floor(j/16), 4, 10, QLatin1Char('0'));
            for( int i=0; i<4; i++ ){
                imageDataStream >> dat[i];
                dump = dump + QString("0x%1, ").arg(dat[i], 8, 16, QLatin1Char('0')).toUpper();
            }
            m_visualLog->appendPlainText(dump);
        }
    }
}

bool PHCCommManager::sendData(uint32_t bank){
    bool ok = true;

    if( !m_portOpen ){
        if( m_visualLog != nullptr ){
            m_visualLog->appendPlainText("PHC PORT CLOSED : sendData");
        }
        return false;
    }

    if( imageData.length() == 0 ){
        if( m_visualLog != nullptr ) m_visualLog->appendPlainText("NO IMAGE DATA TO SEND");
        return false;
    }

    QString cmd = QString("RM,%1,%2\r\n").arg(bank).arg(imageData.length());
    QByteArray qa = cmd.toLocal8Bit();
    QString res;
    this->clear(QSerialPort::AllDirections);
    ok = this->write(qa.data(), qa.length());
    if( !ok ){
        if( m_enableVLog ){
            m_visualLog->appendPlainText(QString("WRITE Failed : sendData : %1").arg(cmd));
            return false;
        }
    }

    if( m_enableVLog) m_visualLog->appendPlainText(QString("To PHC: %1").arg(cmd.left(cmd.length()-2)));

    this->flush();
    uint32_t bidx=0;
    char dat=0;
    for( uint bursts=0; bursts<(imageData.length()/64); bursts++){
        for( uint bytes=0; bytes<64; bytes++){
            dat = *(const char *)(imageData.constData() + ((bursts*64)+bytes));
            this->write(&dat, 1);
            bidx++;
            this->flush();
        }

        QTime ch_delay = QTime::currentTime().addMSecs(1);
        while( ch_delay > QTime::currentTime() );
    }
//    qDebug() << "Sent " << bidx << " bytes";
    qa.clear();
    // Stay for response a minimum of 1s or until we get \r\n
    QTime inMin = QTime::currentTime().addMSecs(500);
    QTime inMax = QTime::currentTime().addSecs(5);
    while(!res.contains("\r\n") || (inMin>QTime::currentTime())){
        this->waitForReadyRead(10);
        qa = qa + this->readAll();
        res = QString::fromLocal8Bit(qa);
        if( QTime::currentTime() > inMax ) {
            ok = false;
            break;
        }
    }
    if( m_enableVLog) m_visualLog->appendPlainText(QString("From PHC: %1").arg(res.left(res.length()-2)));
//    qDebug() << "Response: " << res;

    return ok;
}
