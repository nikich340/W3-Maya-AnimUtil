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
    void addLog(QString text, LogType type = logInfo);

    const uint8_t BYTE_X = 1, BYTE_Y = 2, BYTE_Z = 4, BYTE_RotZ = 8;
    /* JSON */
    const QString mBoneName = "RootMotion";
    const double mFps = 30.0;
    const int mBoneIndex = 94;
    const double mReductionSensitivity = 1e-4; // 10^-4
    const double mW3AngleKoefficient = - 8.07; // picked up experimentally, real value is in [8.05 - 8.10]
    bool animSet;
    bool hasChanges = false;
    QFile jsonFile;
    QJsonDocument jsonDoc;
    QJsonObject jsonRoot;
    QJsonObject objXYZ(double X, double Y, double Z) const;
    QJsonObject objXYZW(double X, double Y, double Z, double W = 1) const;
    QJsonObject objQuanternion(double Pitch, double Yaw, double Roll) const;
    bool loadW3Data();
    void blendMotion(QVector<double>& motion, int animFrames, const QVector<int>& framePoints);
    void reduceMotionRotZ(QVector<double>& motion, int animFrames, int& motionFrames);
    void reduceMotionPos(QVector<double>& motionX, QVector<double>& motionY, QVector<double>& motionZ, int animFrames, int& motionFrames);
    bool applyMotionToBone(QJsonValueRef ref);
    bool extractMotionFromBone(QJsonValueRef ref);

private slots:
    void onClicked_Load();
    void onClicked_Save();
    void onClicked_applyMotionToBone();
    void onClicked_extractMotionFromBone();
};
#endif // W3MAYAANIMUTIL_H
