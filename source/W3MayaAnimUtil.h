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
    QLabel* pLabelInfo;
    QElapsedTimer eTimer;
    enum LogType { logFinish, logWarning, logError, logProcess };
    Ui::W3MayaAnimUtil *ui;

    /* LOG */
    void addLog(QString text, LogType type = logProcess);
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
    int m_framesCount = -1;
    bool hasChanges = false;
    bool batchMode = false;
    bool onlyPrint = false;
    bool shownBroken = false;
    QFile jsonFile;
    QJsonDocument jsonDoc;
    QJsonObject jsonRoot;
    QString vec3ToString(QVector3D vec) {
        return QString("[%1, %2, %3]").arg(vec.x()).arg(vec.y()).arg(vec.z());
    }
    void objToXYZ(QJsonObject obj, double& X, double& Y, double& Z) const;
    void objToXYZW(QJsonObject obj, double& X, double& Y, double& Z, double& W) const;
    void interpolatePos(double k, double& X1, double& Y1, double& Z1, double X2, double Y2, double Z2);
    void interpolateRot(double k, double& X1, double& Y1, double& Z1, double& W1, double X2, double Y2, double Z2, double W2);
    void blendPos(QJsonArray& posArray, int targetFrames);
    void blendRot(QJsonArray& rotArray, int targetFrames);
    double framesToSec(int frames, int fps = 30);
    int secToFrames(double sec, int fps = 30);
    QJsonObject objXYZ(double X, double Y, double Z) const;
    QJsonObject objXYZW(double X, double Y, double Z, double W = 1) const;
    QJsonObject objQuanternion(double Pitch, double Yaw, double Roll) const;
    bool loadW3Data();
    bool loadJsonFile(QString filePath);
    void blendMotion(QVector<double>& motion, int targetFrames, const QVector<int>& framePoints);
    void reduceMotionRotZ(QVector<double>& motion, int animFrames, int& motionFrames);
    void reduceMotionPos(QVector<double>& motionX, QVector<double>& motionY, QVector<double>& motionZ, int animFrames, int& motionFrames);
    bool isAdditiveAnim(QJsonObject animObj);
    bool applyMotionToBone(QJsonValueRef ref);
    bool extractMotionFromBone(QJsonValueRef bufferRef);

    /* GENERAL EDIT */
    double getEventStartTime(QJsonObject eventObj);
    void setEventStartTime(QJsonObject& eventObj, double newTime);
    void setCurrentAnimInfo(int bones, int events, int frames, double duration, bool rootMotion, bool motionExtraction);
    void editBakeBones(QJsonObject& animObj, bool bakePos = true, bool bakeRot = true, bool bakeScale = false);
    void editCropAnim(QJsonObject& animObj, QJsonArray& eventsArray, bool cropEvents, int startFrame, int durationFrames);
    void editSetCDPRDuration(QJsonObject& animObj);
    void editSortEvents(QJsonArray& eventsArray);
    void editRenameAnim(QJsonObject& animObj, QJsonArray& eventsArray, QString newName);
    void editAddEmptyBone(QJsonArray& bonesArr, QString boneName);
    int m_animIndex;
    void applyEdits();

    /* extra nr */
    QVector<QString> m_animNames;
    QVector<int> m_animFrames;
    QVector<double> m_animDurations;
    QStringList additiveNames;
    QStringList additiveTypes;

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

    /* MERGE */
    bool loadMergeJson(QString filePath);
    void saveMergedJson(QJsonObject root, QString suffix);
    QFile jsonFileMerge;
    QString m_mergeFilePath;
    QJsonDocument jsonDocMerge;
    QJsonObject jsonRootMerge;
    QString m_secondAnimName;
    int m_secondAnimFrames = -1;
    int m_secondAnimBones = -1;
    int m_secondAnimEvents = -1;
    bool m_secondAnimMotionExtraction = false;
    bool m_secondAnimRootMotion = false;
    double m_secondAnimDuration = -1;

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

    /* GENERAL EDIT */
    void onChecked_EditCut(bool checked);
    void onChanged_EditCurrentAnim(int newAnimIndex);
    void onChanged_EditStart(int startFrame);
    void onChanged_EditDuration(int durFrames);
    void onChanged_EditEnd(int endFrame);
    void onClicked_EditApply();
    void onClicked_EditApplyAll();

    /* MERGE */
    void onClicked_LoadMergeJson();
    void onClicked_MergeProcess();
    void onClicked_MergePictureHelp();
    void onUpdate_MergeInfo();
    void onChanged_MergeStart();
    void onChanged_MergeDuration();
    void onChanged_MergeType(int index);
    void onClicked_BlendHalfSec();
};
#endif // W3MAYAANIMUTIL_H
