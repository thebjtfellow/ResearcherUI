#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVulkanInstance>
#include <QVulkanWindow>
#include <QVulkanWindowRenderer>
#include "stlvulkanwindow.h"
#include <QPlainTextEdit>
#include <QSerialPort>
#include <QSettings>
#include <QTimer>

#include "executiveengine.h"
#define DEFAULT_PATH "/home/BJ2/printjobs/"
#define DEFAULT_GCODE "/home/BJ2/printjobs/GCODE/"
#include "crc.h"

Q_DECLARE_METATYPE(QCheckBox*);


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#if defined(SL1_BUILD)

#elif defined(Q1_BUILD)

#elif defined(Q2_BUILD)

#elif defined(SG_BUILD)
#else
#error "No Printhead Selected"

#endif

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    void doServiceTimer();

private slots:
    void pollTemps();

private slots:
    void on_setZPosition_clicked();

private slots:
    void on_raiseZToTop_clicked();

private slots:
    void on_clearExecLog_clicked();

private slots:
    void on_clearPHCLog_clicked();

private slots:
    void on_clearCNCLog_clicked();

private slots:
    void on_tryPass_clicked();

private slots:
    void on_goPassAlignY_clicked();

private slots:
    void on_goPassEndX_clicked();

private slots:
    void on_goSendDirectPHC_clicked();

private slots:
    void on_tryWipeRoutine_clicked();

private slots:
    void on_txtPHCDirect_returnPressed();

private slots:
    void on_runInitializeScript_clicked();

private slots:
    void on_restartComs_clicked();

private slots:
    void on_updateLaydownINI_clicked();

private slots:
    void on_gotoCapSafeX_clicked();

private slots:
    void on_gotoCapY_clicked();

private slots:
    void on_gotoCapX_clicked();

private slots:
    void on_tryPrimeRoutine_clicked();

private slots:
    void on_saveTendingFunctions_clicked();

private slots:
    void on_makeTendingFunctions_clicked();

private slots:
    void on_tryDispensePower_clicked();

private slots:
    void on_goWipeEndX_clicked();

private slots:
    void on_pushButton_clicked();

    void on_tabWidget_currentChanged(int index);

//    void on_pushButton_2_clicked();

//    void on_pushButton_3_clicked();

    void on_btnAddLayerCommand_clicked();

    void on_pushButton_4_clicked();

    void on_btnUpdateLaydown_clicked();

    void addLayerCommand(QPlainTextEdit* cmdList);

    void on_btnAddLayerException_clicked();

    void on_sbSliceLayer_valueChanged(int value);

    void on_pushButton_5_clicked();

    void on_goStartUV_clicked();

    void on_goEndUV_clicked();

    void on_tryDryRoutine_clicked();

    void on_goDispenseStart_clicked();

    void on_goDispenseEnd_clicked();

    void on_tryDispenseTurns_clicked();

    void on_tryDispenseRoutine_clicked();

    void on_goRollerStart_clicked();

    void on_goRollerEnd_clicked();

    void on_tryRoller1RPM_clicked();

    void on_tryRoller2RPM_clicked();

    void on_tryRollerRoutine_clicked();

    void on_goPassStartX_clicked();

    void on_goToZeroA_clicked();

    void on_tryRecoatAll_clicked();

    void on_goHomeX_clicked();

    void on_goHomeY_clicked();

    void on_goHomeA_clicked();

    void on_goHomeZ_clicked();

    void on_goSendDirect_clicked();

    void on_generateBuildFiles_clicked();

    void on_btnLoadSTL_clicked();

    void on_btnMakeImages_clicked();

    void on_cmdMakeRecoatDefaults_clicked();

    void on_cmdGenerateRecoaterGCode_clicked();

    void on_cmdSaveRecoatGCodeAs_clicked();

    void on_txtCNCDirect_returnPressed();

    void on_btnRunPause_clicked();

    void on_goWipeStartX_clicked();

    void on_attachCommands_clicked();

    void on_setImageStackSource_clicked();

 //   void on_saveRecoatINI_clicked();

    void on_lockZeroZ_clicked();

    void on_chooseGCODESource_clicked();

    void on_saveScripts_clicked();

    void on_loadScript_clicked();

    void on_chkEnableCNCStream_stateChanged(int arg1);

    void on_chkEnablePHCStream_stateChanged(int arg1);

    void on_lowerZToBottom_clicked();

    void on_loadRecoatINI_clicked();

    void on_loadLaydownINI_clicked();

    void on_resetLayer_clicked();

    void on_resetJob_clicked();

    void on_RIPImageStack_clicked();

    void on_rip2ColorImageStack_clicked();

    void on_gotoPrimeX_clicked();

    void on_turnsAsOnDelay_stateChanged(int arg1);

    void on_lowerZby1_clicked();

    void on_setCurrentLayer_clicked();

    void on_gotoTempCheckX_clicked();

    void on_gotoTempCheckY_clicked();

    void on_fireSample_clicked();

    void on_calculateDropmass_clicked();

    void on_raiseFby1_clicked();

    void on_lowerFto0_clicked();

    void on_phSelect_currentIndexChanged(int index);

    void on_preheatBuildplate_clicked();

    void on_preheatOverhead_clicked();

    void on_runPHCInitScript_clicked();

    void on_preheatBinder_clicked();

    void on_preheatOven_clicked();

    void on_preheatFeedplate_clicked();

    void on_tryFan1_clicked();

    void on_tryFan2_clicked();

    void on_setFPosition_clicked();

    void on_raiseFToTop_clicked();

    void on_goRollMidpoint_clicked();

    void on_trySmooth_B2B_clicked();

    void on_enablePowerScaling_stateChanged(int arg1);

    void on_IOConfig_modeChanged(int arg1);
    void on_IOConfig_stateChanged(int arg1);


    void on_unloadAllSTL_clicked();

    void on_cancelCNCCommand_clicked();

private:
    Ui::MainWindow *ui;
    QVulkanInstance *vulkanInstance=nullptr;
    STLVulkanWindow *stl_window=nullptr;
    QWidget * holder=nullptr;
    ExecutiveEngine m_executiveEngine;
    CNCComManager m_CNCComPort;
    PHCCommManager m_PHCCommPort;
    QTimer* m_serviceTimer=nullptr;
    QTimer* m_tempTimer=nullptr;
    QString m_curGcodePath;
    QString m_curStlPath;
    QString m_curImagePath;
    QString m_curLaydownIniPath;
    QString m_curRecoatIniPath;
    MaterialINIFile m_curMaterial;
    QImage makeQuadImage();
    QImage makeNPassImage();

    RecoaterSettings recoaterSettings;
    LaydownSettings laydownSettings;
    bool readRecoatingPage();
    void writeRecoatingPage();
    bool readLaydownPage();
    void writeLaydownPage();
    bool saveGCodeFiles(QPlainTextEdit *lines);
    void refreshFileLists();
    bool ripLayer(uint32_t layer, const QString &filePath);
    QImage ripSLImage(const QImage &inImg);
    bool makeMultiThicknessStack( QString outFilePath );
    bool ripLayerImageStack(const QString &infilePath, const QString &outFilePath);
    QImage ripQ1Image(const QImage inImg);
    QImage ripQ2Image(const QImage inImg, uint32_t colorChannel);
    QImage ripSGImage( const QImage inImage );
    QImage ripSGImageFwd( const QImage inImage );
    bool ripLayerQ2(uint32_t layer, const QString &filePath);
    bool m_IOStartup=false;
};

bool SaveGCodeFile(QPlainTextEdit *lines, const QString* outFilePath);

#endif // MAINWINDOW_H
