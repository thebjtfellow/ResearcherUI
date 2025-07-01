#ifndef EXECUTIVEENGINE_H
#define EXECUTIVEENGINE_H

// #define one of three values:
// SL1_BUILD, Q1_BUILD, Q2_BUILD, or SG_BUILD as 1
#define SG_BUILD 1

// If this is a UV system, define USE_UV
//#define USE_UV 1

// If this is one, Binder will be axis U, binder 2 is V
// #define UTK 1
//#define PROTO_OUTS 1

// If this is a single roller system, ony encode C
// #define SINGLE_ROLLER

// Qt Libs
#include <QString>
#include <QImage>
#include <QSettings>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTime>
#include <QGraphicsView>
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <QPlainTextEdit>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QTableWidgetItem>

#include <gpiod.h>


extern gpiod_chip * ioChip;
extern gpiod_line * ioLine;


enum PH_TYPES {
    S_CLASS,
    Q_CLASS_1S,
    Q_CLASS_2S,
    Q_CLASS_1P,
    Q_CLASS_2P,
    Q_CLASS_SG
};

struct PH_DESCRIPTOR {
    PH_TYPES phType;
    QString Description;
    uint32_t jetCount;
    float_t jetSpacing;
};

struct MaterialINIFile {
    QString MaterialINI;
    bool loadName(QString filePath);
    bool saveName(QString filePath);
};

const PH_DESCRIPTOR phDescriptions[] = {
    {S_CLASS, QString("Single S-Class Module"), 128, 0.508f},
    {Q_CLASS_1S, QString("Single Q-Class S Module"), 256, 0.254f},
    {Q_CLASS_2S, QString("Two Q-Class S Modules"), 256, 0.254f},
    {Q_CLASS_1P, QString("Single Q-Class P Module"), 256, 0.254f},
    {Q_CLASS_2P, QString("Two Q-Class P Modules"), 256, 0.254f},
    {Q_CLASS_SG, QString("Single SG1024 Module"), 1024, 0.0635f}
};

#ifdef SL1_BUILD
#define JET_COUNT 128
#define JET_SPACE 0.508f
#define PASS_COUNT 4
#define PASS_SPACE 0.127f
#define MAX_JET_SHIFT 10.0f
#elif Q1_BUILD
#define JET_COUNT 256
#define JET_SPACE 0.254f
#define PASS_COUNT 2
#define PASS_SPACE 0.127f
#define MAX_JET_SHIFT 20.0f
#elif Q2_BUILD
#define JET_COUNT 256
#define JET_SPACE 0.254f
#define PASS_COUNT 2
#define PASS_SPACE 0.127f
#define HEAD_OFFSET 8.0f
#define MAX_JET_SHIFT 20.0f
#elif SG_BUILD
#define JET_COUNT 1024
#define JET_SPACE 0.0635f
#define PASS_COUNT 1
#define PASS_SPACE 0.0635f
#define MAX_JET_SHIFT 80.0f

#endif

#define IMAGE_COLS 512
#define IMAGE_ROWS 512
#define IMAGE_WID_MM 65.024f
#define IMAGE_WID_H_MM 32.512f
#define PRINT_AREA_WID 59.944f
#define PRINT_AREA_H_WID 29.972f
#define IMAGE_SIZE_BYTES 32768 // ((IMAGE_COLS/8)*IMAGE_ROWS)
#define PASS_SIZE_BYTES 8192 // ((JET_COUNT*IMAGE_ROWS)/8)
#define ODD_OFFSET 64
#define LINE_SIZE_BYTES 16 // (JET_COUNT/8)

#ifndef USE_UV
#ifndef PROTO_OUTS
#define BTT_CAP 1
#define BTT_PUMP 0
#else
#define BTT_CAP 0
#define BTT_PUMP 1
#endif
#define BTT_VAC 2
#define BTT_VIBE 3
#define BTT_FAN1 4
#define BTT_FAN2 5
#elif USE_UV
#define BTT_CAP 1
#define BTT_PUMP 2 // 0 for vitriform
#define BTT_UV 3 // 5 for vitriform
#define BTT_VAC 4
#define BTT_VIBE 5 // 3 for vitriform
#endif

struct CNCAxisPositions{
    float x=0.0f;
    float y=0.0f;
    float z=0.0f;
    float a=0.0f;
    float b=0.0f;
    float c=0.0f;
    float u=0.0f;
    float v=0.0f;
};

struct CNCTemperatures{
    float buildPlateTemp=0.0f;
    float overheadTemp=0.0f; // Tool 0
    float binderTemp=0.0f; // Tool 1
    float ovenTemp=0.0f; // Tool 2
    float feedPlateTemp=0.0f;// Tool 3 Feed plate
};

struct RecoaterSettings{
    float heat_start;
    float heat_end;
    float heat_speed;
    QList<float_t> heat_speeds;
    float temperature;
    uint8_t fan1_pwm;
    uint8_t fan2_pwm;
    float disp_start;
    float disp_end;
    int disp_sections;
    uint8_t disp_power;
    QList<uint32_t> disp_powers;
    float disp_speed;
    QList<float_t> disp_speeds;
    float rolling_start;
    float rolling_end;
    float roller1_rpm;
    float roller2_rpm;
    float rolling_speed;
    QList<float_t> rolling_speeds;
    float rolling_len;
    float rolling_time;
    float roller1_turns;
    float roller2_turns;
    float z_retract;
    bool single_roller_motor;
    float x_move_speed;
    float y_move_speed;
    float z_move_speed;
    float a_move_speed;
    float build_plate_temp;
    uint32_t x_home_sense;
    uint32_t y_home_sense;
    uint32_t z_home_sense;
    uint32_t a_home_sense;
    float rolling_midpoint;
    float rolling_fast_speed;
    float feed_powder_ratio;
    float feed_plate_temp;
    float oven_temp;
    bool useTurnsAsDelay;
    bool enablePowerScaling;

    RecoaterSettings& operator =(const RecoaterSettings& other);
    static bool readFile(const QString& fileName, RecoaterSettings *settings);
    static bool writeFile(const QString& fileName, RecoaterSettings *settings);
};

struct LaydownSettings{ // Default params initialize for an SL, 4 passes, square .127 spacing, .55 pack rate, .2mm
    float jetCount = 128.0;
    float jetSpacing = .508;
    float dropVolume = 80.0;
    float pulseWidth = 10.0;
    QList<float_t> pulseWidths;
    float packingRate = 0.55;
    float passCount = 4.0;
    float printSpeed = 254.0;
    float printFrequency = 2000.0;
    float layerThickness = 0.2;
    QList<float_t> layerThicknesses;
    float passSpacing = .508/4;
    float lineSpacing = 127.0/1000.0;
    float saturation = .8;
    float yAlignZero = 0.5f;
    float startPassX = 220.0f;
    float endPassX = 340.0f;
    float capX = 0.0f;
    float capY = 0.0f;
    float capSafeX = 150.0f;
    float wipeStartX = 0.0f;
    float wipeEndX = 40.0f;
    float wipeY = 5.0f;
    float wipeSpeed = 600.0f;
    float primeX = 60.0;
    int primeTime = 1500;
    int dripTime = 2000;
    float yAxisBacklash = 2.5f;
    float yHomeOffset = 3.0f;
    QList<uint32_t> majorOffsets;
    QList<uint32_t> minorOffsets;
    float tempCheckX = 100.0f;
    float tempCheckY = 5.0f;
    float testLocationX = 100.0f;
    float binderTemp = 20.0f;
    PH_TYPES phType = S_CLASS;
    LaydownSettings& operator=(const LaydownSettings& other);
    static bool readFile(const QString& fileName, LaydownSettings *settings);
    static bool writeFile(const QString& fileName, LaydownSettings *settings);

};
struct IOConfig {
    bool isInput[26];
    bool isOn[26];
    bool saveFile(const QString& fileName);
    bool readFile( const QString& fileName);
};

extern IOConfig ioConfig;


enum ExCommandID{
    RUN_GCODE_FILE,
    EXECUTE_GCODE,
    SET_PRINTDATA
};

struct GCodeFile{
    uint64_t id;
    QString fileName;
    QStringList instructions;
    static bool loadGCodeFile(GCodeFile *toLoad, QString fileName);
    static bool clearFile(GCodeFile *toClear);

};


struct ExCommand{
    ExCommandID id;


};
//----------------------------------------
class CNCComManager : public QSerialPort
{
    Q_OBJECT
public:
    CNCComManager();
    ~CNCComManager();

    bool sendGCodeCommand(QString command, QString *response=nullptr);
    bool sendEmergencyGCodeCommand( QString command, QString *response=nullptr);
    bool getAxisPositions(CNCAxisPositions *axisPositions);
    bool getTemperatures(CNCTemperatures* temps);

    bool homeXAxis();
    bool homeYAxis();
    bool homeZAxis();
    bool homeAAxis();
    bool findPort();

    bool refillReservoir();
    bool setVisualLog( QPlainTextEdit *log ){ return (m_visualLog = log);}
    bool setEnableVLog( bool eVL ){ m_enableVLog = eVL; return true;}
public slots:
    void handleUnexpected();

private:

    bool m_portOpen=false;
    CNCAxisPositions axisPositions{};
    QPlainTextEdit *m_visualLog=nullptr;
    bool m_enableVLog=false;
    bool m_inSendGCode=false;
    bool m_expected = false;
    bool m_inUnexpected = false;
    bool m_inRefill = false;
    CNCTemperatures lastValidTemps{};
    CNCAxisPositions lastValidPosition{};
};

typedef enum {
    NONE_SELECTED,
    SL_SINGLE,
    Q_SINGLE,
    Q_DOUBLE,
    SG1024
} ModuleTypes;

//---------------------------------------
class PHCCommManager : public QSerialPort{
public:
    PHCCommManager();
    ~PHCCommManager();

    bool sendText(QString toSend, QString *response = nullptr);
    bool sendData(uint32_t bank=0);
    bool makeData(QImage &img);
    bool sendDataM(uint32_t colorChannel);
    bool findPort();
    bool setVisualLog( QPlainTextEdit *log );
    bool setEnableVlog( bool eVL ){ m_enableVLog = eVL; return true;}
    bool setModuleType( PH_TYPES module );
    void dumpData();

private:
    bool m_portOpen = false;
    QByteArray imageData;
    QByteArray imageDataM[3];
    QPlainTextEdit *m_visualLog=nullptr;
    bool m_enableVLog = false;
    PH_TYPES currentModule;

};
//----------------------------------------

// The ExecutiveEngine class
class ExecutiveEngine
{

public:

    ExecutiveEngine();
    ~ExecutiveEngine();

    bool addGCodeFile(const QString &gcodeFile);

//-- To be valid
    bool setImageStackPath( const QString &imageStackPath, uint32_t colors = 1);
    bool setGCodePath( const QString &gcodePath);
    QString getGCodePath() {return m_gcodePath;}
    bool setImagePreview( QLabel *previewWindow);
    bool attachInitializeList(const QStringList & initList);
    bool attachCommandList(const QStringList& commandList);
    bool executeCommandList( const QStringList &cmdList, bool async=false );

    bool attachExceptionCommandList(const QStringList& exceptionCommandList);
    bool attachFinalizeList(const QStringList& finalizeList );
    bool setLayerZeroZ( float zeroZ );

    bool setPHC(PHCCommManager *phc);
    bool setCNC(CNCComManager *cnc);
    bool setVisualLog( QPlainTextEdit * visualLog ){ return (m_visualLog = visualLog);}
    bool setUseInitList( QCheckBox *cb ){return (m_useInitList = cb);}
    bool setUseFinalList( QCheckBox* cb ){ return (m_useFinalList = cb);}
    bool setUse2Binders( QCheckBox* cb){ return (m_use2binders=cb);}
    bool setEnablePowerScaling( QCheckBox* cb){ return (m_enablePowerScaling=cb);}
    bool setReducePowerBy(QLineEdit* le){ return (m_reducePowerBy=le);}
    bool setReducePowerEvery(QLineEdit* le){ return(m_reducePowerEvery=le);}
    bool setParams(RecoaterSettings * rs, LaydownSettings * ls){return ((m_recoatSettings=rs)&&(m_laydownSetting=ls));}
    bool setGenerateRecoat( QPushButton * pb){return (m_generateRecoat=pb);}
    bool setRecoatList( QPlainTextEdit * pte){ return(m_recoatList=pte);}
    bool setPowerItem( QTableWidgetItem *twi ){return (m_powerItem=twi);}
    bool setheatSpeedItem( QTableWidgetItem *twi ){return (m_heatSpeedItem=twi);}
    bool setdispSpeedItem( QTableWidgetItem *twi ){return (m_dispSpeedItem=twi);}
    bool setsmoothSpeedItem( QTableWidgetItem *twi ){return (m_rollingSpeedItem=twi);}
    //--
    bool setLayerCounterLabel(QLabel * lcLabel){m_LCLabel = lcLabel; return true;}
    bool setCurrentInstructionLabel( QLabel* lbl){m_currInstLabel = lbl; return true;}
    bool setExceptionFrequency(const uint32_t frequency){m_exceptionFrequency = frequency; return true;}
    bool printLayerPasses(uint32_t layerID);
    bool printLayerPassesM(uint32_t layerID);
    bool incrementLayerCounter();
    bool setLayerCount(uint32_t layers){m_layersToPrint = layers; return true;}
    bool setCurrentLayer( uint32_t newCurLayer );
    bool runInitializeList();
    bool runFinalizeList();
    bool run();
    bool resetLayer();
    bool resetJob();
    bool asynchExecuteGCodeFile(const QString & fileName);
    bool pause();
    bool unpause();
    bool cancel();
    bool valid();
    uint32_t currentLayer(){ return m_layerCounter; }
    bool setBuildBedTemp(float newTemp);
    bool setOverheadTemp(float newTemp);
    bool setBinderTemp(float newTemp);
    bool setOvenTemp(float newTemp);
    bool setFeedBedTemp(float newTemp);

private:

    bool m_isValid = false;
    bool m_isRunning=false;
    bool m_pauseReq=false;
    bool m_isInitialized=false;
    bool m_layerZeroZSet=false;
    bool m_layerDataSent=false;
    bool m_inException = false;
    bool m_inFinalize = false;

    uint32_t m_programCounter=0; // Program counter for current list in execution pipeline
    uint32_t m_gcodePC=0; // Program counter for G-Code execution
    uint32_t m_printLayerPC=0; // Program counter for printLayerPasses
    uint32_t m_colors=0;
    uint32_t m_layerCounter=0;
    uint32_t m_layersToPrint=0;
    uint32_t m_exceptionFrequency=0;
    float m_layerZeroZ=0.0f;

    CNCComManager * m_cnc=nullptr;
    PHCCommManager * m_phc=nullptr;
    RecoaterSettings * m_recoatSettings=nullptr;
    LaydownSettings * m_laydownSetting=nullptr;

    QString m_imageStackPath="";
    QString m_gcodePath="";
    QStringList m_initializeList{};
    QStringList m_layerCommandList{};
    QStringList m_layerExceptionCommandList{};
    QStringList m_finalizeList{};

    // UI Elements
    QPushButton * m_generateRecoat;
    QLabel *m_LCLabel=nullptr;
    QLabel *m_currInstLabel=nullptr;
    QLabel *m_imagePreview=nullptr;
    QCheckBox *m_useInitList=nullptr;
    QCheckBox *m_useFinalList=nullptr;
    QCheckBox *m_use2binders=nullptr;
    QCheckBox *m_enablePowerScaling=nullptr;
    QLineEdit *m_reducePowerBy=nullptr;
    QLineEdit *m_reducePowerEvery=nullptr;
    uint32_t m_basePower=0;
    QPlainTextEdit *m_recoatList=nullptr;
    QTableWidgetItem *m_powerItem=nullptr;
    QTableWidgetItem *m_heatSpeedItem=nullptr;
    QTableWidgetItem *m_dispSpeedItem=nullptr;
    QTableWidgetItem *m_rollingSpeedItem=nullptr;


    bool decodeInstruction(const QString nextInstruction);
    bool showLayer( uint32_t layer );
    GCodeFile m_curGCode; // Current GCode file being executed
    uint32_t m_curGCodeLC; // Line counter for current file
    bool initializeJob();
    bool executeCurrentLayer();
    bool finalizeJob();
    bool executeGCodeFile(const QString & fileName);
    bool positionZ();
    bool positionZF();

    QPlainTextEdit *m_visualLog=nullptr;

};

static const uint8_t BitReverseTable256[] =
{
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
  0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
  0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
  0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
  0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
  0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
  0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
  0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
  0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

static const uint8_t NibbleSwapTable256[] =
{
    0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0,
    0x01, 0x11, 0x21, 0x31, 0x41, 0x51, 0x61, 0x71, 0x81, 0x91, 0xA1, 0xB1, 0xC1, 0xD1, 0xE1, 0xF1,
    0x02, 0x12, 0x22, 0x32, 0x42, 0x52, 0x62, 0x72, 0x82, 0x92, 0xA2, 0xB2, 0xC2, 0xD2, 0xE2, 0xF2,
    0x03, 0x13, 0x23, 0x33, 0x43, 0x53, 0x63, 0x73, 0x83, 0x93, 0xA3, 0xB3, 0xC3, 0xD3, 0xE3, 0xF3,
    0x04, 0x14, 0x24, 0x34, 0x44, 0x54, 0x64, 0x74, 0x84, 0x94, 0xA4, 0xB4, 0xC4, 0xD4, 0xE4, 0xF4,
    0x05, 0x15, 0x25, 0x35, 0x45, 0x55, 0x65, 0x75, 0x85, 0x95, 0xA5, 0xB5, 0xC5, 0xD5, 0xE5, 0xF5,
    0x06, 0x16, 0x26, 0x36, 0x46, 0x56, 0x66, 0x76, 0x86, 0x96, 0xA6, 0xB6, 0xC6, 0xD6, 0xE6, 0xF6,
    0x07, 0x17, 0x27, 0x37, 0x47, 0x57, 0x67, 0x77, 0x87, 0x97, 0xA7, 0xB7, 0xC7, 0xD7, 0xE7, 0xF7,
    0x08, 0x18, 0x28, 0x38, 0x48, 0x58, 0x68, 0x78, 0x88, 0x98, 0xA8, 0xB8, 0xC8, 0xD8, 0xE8, 0xF8,
    0x09, 0x19, 0x29, 0x39, 0x49, 0x59, 0x69, 0x79, 0x89, 0x99, 0xA9, 0xB9, 0xC9, 0xD9, 0xE9, 0xF9,
    0x0A, 0x1A, 0x2A, 0x3A, 0x4A, 0x5A, 0x6A, 0x7A, 0x8A, 0x9A, 0xAA, 0xBA, 0xCA, 0xDA, 0xEA, 0xFA,
    0x0B, 0x1B, 0x2B, 0x3B, 0x4B, 0x5B, 0x6B, 0x7B, 0x8B, 0x9B, 0xAB, 0xBC, 0xCB, 0xDB, 0xEB, 0xFB,
    0x0C, 0x1C, 0x2C, 0x3C, 0x4C, 0x5C, 0x6C, 0x7C, 0x8C, 0x9C, 0xAC, 0xBC, 0xCC, 0xDC, 0xEC, 0xFC,
    0x0D, 0x1D, 0x2D, 0x3D, 0x4D, 0x5D, 0x6D, 0x7D, 0x8D, 0x9D, 0xAD, 0xBD, 0xCD, 0xDD, 0xED, 0xFD,
    0x0E, 0x1E, 0x2E, 0x3E, 0x4E, 0x5E, 0x6E, 0x7E, 0x8E, 0x9E, 0xAE, 0xBE, 0xCE, 0xDE, 0xEE, 0xFE,
    0x0F, 0x1F, 0x2F, 0x3F, 0x4F, 0x5F, 0x6F, 0x7F, 0x8F, 0x9F, 0xAF, 0xBF, 0xCF, 0xDF, 0xEF, 0xFF
};
static const uint8_t NibbleSwapReverseTable256[] =
{
    0x00, 0x80,
};

extern bool SaveGCodeFile(QPlainTextEdit *lines, const QString* outFilePath);
#endif // EXECUTIVEENGINE_H
