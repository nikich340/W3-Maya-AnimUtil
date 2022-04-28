#ifndef W3MAYAANIMUTIL_H
#define W3MAYAANIMUTIL_H

#include <QMainWindow>
#include <QLabel>
#include <QElapsedTimer>
#include <QStringList>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonParseError>
#include <QQuaternion>

QT_BEGIN_NAMESPACE
namespace Ui { class W3MayaAnimUtil; }
QT_END_NAMESPACE

class W3MayaAnimUtil : public QMainWindow
{
    Q_OBJECT

public:
    W3MayaAnimUtil(QWidget *parent = nullptr);
    ~W3MayaAnimUtil();

private:
    QLabel* pLabel;
    QElapsedTimer eTimer;
    enum LogType { logInfo, logWarning, logError };
    Ui::W3MayaAnimUtil *ui;

    /* LOG */
    void addLog(QString text, LogType type = logInfo);
    QFile logFile;
    QTextStream logStream;

    const uint8_t BYTE_X = 1, BYTE_Y = 2, BYTE_Z = 4, BYTE_RotZ = 8;
    QSet<int> logStarts;
    int currentStart = 0;
    int totalLen = 0;

    /* JSON */
    const QString mBoneName = "RootMotion";
    const double mFps = 30.0;
    const int mBoneIndex = 94;
    double mReductionSensitivity(); // 10^-4
    const double mW3AngleKoefficient = - 8.07; // picked up experimentally, real value is in [8.05 - 8.10]
    bool animSet = false;
    int framesCount = -1;
    bool hasChanges = false;
    bool batchMode = false;
    bool onlyPrint = false;
    bool shownBroken = false;
    QFile jsonFile;
    QJsonDocument jsonDoc;
    QJsonObject jsonRoot;
    void objToXYZ(QJsonObject obj, double& X, double& Y, double& Z) const;
    void objToXYZW(QJsonObject obj, double& X, double& Y, double& Z, double& W) const;
    void interpolatePos(double t, double& X1, double& Y1, double& Z1, double X2, double Y2, double Z2);
    void interpolateRot(double t, double& X1, double& Y1, double& Z1, double& W1, double X2, double Y2, double Z2, double W2);
    QJsonObject objXYZ(double X, double Y, double Z) const;
    QJsonObject objXYZW(double X, double Y, double Z, double W = 1) const;
    QJsonObject objQuanternion(double Pitch, double Yaw, double Roll) const;
    bool loadW3Data();
    bool loadJsonFile(QString filePath);
    void blendMotion(QVector<double>& motion, int animFrames, const QVector<int>& framePoints);
    void reduceMotionRotZ(QVector<double>& motion, int animFrames, int& motionFrames);
    void reduceMotionPos(QVector<double>& motionX, QVector<double>& motionY, QVector<double>& motionZ, int animFrames, int& motionFrames);
    bool isAdditiveAnim(QJsonObject animObj);
    bool applyMotionToBone(QJsonValueRef ref);
    bool extractMotionFromBone(QJsonValueRef ref);

    /* extra nr */
    QVector<QString> animNames;
    QStringList additiveNames;
    QStringList additiveTypes;
    QVector<double> animDurations;

    /* EXTERNAL MOTION */
    bool loadJsonMotion(QString filePath);
    QFile jsonFileMotion;
    QJsonDocument jsonDocMotion;
    QJsonObject jsonRootMotion;
    QMap<QString, QJsonObject> motionByName;

    /* ANIM EVENTS */
    bool loadJsonAnimEvents(QString filePath);
    QFile jsonFileEvents;
    QJsonDocument jsonDocEvents;
    QJsonObject jsonRootEvents;
    QMap<QString, QJsonArray> animEventsByName;

    /* BLEND */
    bool loadBlendJson(QString filePath);
    QFile jsonFileBlend;
    QJsonDocument jsonDocBlend;
    QJsonObject jsonRootBlend;
    int blendSourceFramesCount = -1;

    /* CUTSCENE */
    QJsonArray extractAnimParts(QJsonValueRef ref);
    void patchAnimParts(QString jsonPath, QJsonValueRef animArrayRef);

private slots:
    void onClicked_Load();
    void onClicked_Save();
    void onClicked_SaveSplit();
    void onClicked_applyMotionToBone();
    void onClicked_extractMotionFromBone();
    void onClicked_extractMotionFromBoneBatch();
    void onClicked_loadAnimEventsSource();
    void onClicked_loadMotionSource();
    void onClicked_loadTxtDump();
    void onClicked_PrintInfo();

    void onClicked_extractPartsCutscene();
    void onClicked_patchPartsCutscene();

    void onClicked_LoadBlendJson();
    void onClicked_Blend();
    void onChanged_BlendParams();
    void onChanged_BlendFrames();
};
#endif // W3MAYAANIMUTIL_H
