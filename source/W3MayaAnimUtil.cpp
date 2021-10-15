#include "W3MayaAnimUtil.h"
#include "ui_W3MayaAnimUtil.h"
#include <QScrollBar>
#include <QMessageBox>
#include <QFileInfo>
#include <QFileDialog>

#define upn(val, start, end) for(int val = start; val <= end; ++val)
#define JRef QJsonValueRef

W3MayaAnimUtil::W3MayaAnimUtil(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::W3MayaAnimUtil)
{
    ui->setupUi(this);
    //ui->textLog->setFontPointSize(20);
    ui->textLog->setHtml("Welcome :)<br>Click \"Load anim .json\" to start");
    ui->spinSensivity->setMinimum(0.0000000001);
    ui->spinSensivity->setSingleStep(0.00001);
    ui->spinSensivity->setValue(0.00001);

    connect(ui->buttonLoad, SIGNAL(clicked(bool)), this, SLOT(onClicked_Load()));
    connect(ui->buttonSave, SIGNAL(clicked(bool)), this, SLOT(onClicked_Save()));
    connect(ui->buttonApplyMotionToBone, SIGNAL(clicked(bool)), this, SLOT(onClicked_applyMotionToBone()));
    connect(ui->buttonExtractMotionFromBone, SIGNAL(clicked(bool)), this, SLOT(onClicked_extractMotionFromBone()));
    connect(ui->buttonExtractMotionFromBoneBatch, SIGNAL(clicked(bool)), this, SLOT(onClicked_extractMotionFromBoneBatch()));
    connect(ui->buttonLoadEventsSource, SIGNAL(clicked(bool)), this, SLOT(onClicked_loadAnimEventsSource()));
    connect(ui->buttonLoadMotionSource, SIGNAL(clicked(bool)), this, SLOT(onClicked_loadMotionSource()));

    connect(ui->buttonBlendLoad, SIGNAL(clicked(bool)), this, SLOT(onClicked_LoadBlendJson()));
    connect(ui->buttonBlendDo, SIGNAL(clicked(bool)), this, SLOT(onClicked_Blend()));
    connect(ui->spinBlendSource, SIGNAL(valueChanged(int)), this, SLOT(onChanged_BlendFrames()));
    connect(ui->spinBlendTarget, SIGNAL(valueChanged(int)), this, SLOT(onChanged_BlendFrames()));
}

double W3MayaAnimUtil::mReductionSensitivity() {
    return ui->spinSensivity->value();
}
void W3MayaAnimUtil::addLog(QString text, LogType type) {
    QColor color;
    if (type == logInfo) {
        color.setRgb(0, 96, 8);
    } else if (type == logWarning) {
        color.setRgb(188, 94, 0);
    } else {
        color.setRgb(188, 0, 0);
    }
    ui->textLog->setText( ui->textLog->toHtml() + QString("<font color=\"%1\">%2</font>").arg(color.name()).arg(text) );
    ui->textLog->verticalScrollBar()->setValue( ui->textLog->verticalScrollBar()->maximum() );
}

bool W3MayaAnimUtil::loadJsonFile(QString filePath) {
    jsonFile.setFileName(filePath);
    if ( !jsonFile.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        addLog("Can't open file in read-only text mode: " + filePath, logError);
        return false;
    }

    QByteArray bArray = jsonFile.readAll();
    QJsonParseError* jsonError = new QJsonParseError;
    jsonDoc = QJsonDocument::fromJson(bArray, jsonError);
    if ( jsonDoc.isNull() ) {
        addLog( QString("Can't load json document correctly, parse error = %1").arg(jsonError->errorString()), logError );
        return false;
    } else if ( !jsonDoc.isObject() ) {
        addLog( "Json root is not an object, can't load info.", logError );
        return false;
    }

    if ( QFile::exists(filePath + ".bak") ) {
        QFile::remove(filePath + ".bak");
    }
    jsonFile.copy(filePath + ".bak");
    jsonFile.close();

    jsonRoot = jsonDoc.object();
    if ( loadW3Data() ) {
        addLog( QString("[OK] Loaded %1, original file saved as: %1.bak").arg(QFileInfo(filePath).fileName()) );
        ui->linePath->setText(filePath);
        return true;
    } else {
        addLog( QString("Failed to detect data format: %1").arg(QFileInfo(filePath).fileName()), logError );
        ui->linePath->setText("NO CORRECT .JSON LOADED!");
        return false;
    }
}

void W3MayaAnimUtil::onClicked_Load() {
    if (hasChanges && QMessageBox::Yes != QMessageBox::question(this, "Attention!", "Currently loaded .json has some unsaved changes. Do you want to discard them and load new file?")) {
        return;
    }
    hasChanges = false;

    QString filePath = QFileDialog::getOpenFileName(this, "Open animation json", "", "JSON Files (*.json)");
    if (filePath.isEmpty()) {
        addLog("Loading file canceled by user", logWarning);
        return;
    }
    loadJsonFile(filePath);
}
bool W3MayaAnimUtil::loadW3Data() {
    if ( jsonRoot.contains("animations") ) {
        animSet = true; // exported from wkit
        int animCount = jsonRoot.value("animations").toArray().count();
        framesCount = -1;
        onChanged_BlendParams();
        if (animCount > 0) {
            addLog( QString("[OK] Detected array of %1 anims (probably, exported from WolvenKit)").arg(animCount) );
            return true;

        } else {
            addLog( QString("Expected array of anims, but it doesn't exist or is empty"), logError );
            return false;
        }

        /*upn(i, 0, animArray.size() - 1) {
            animList.append( animArray.at(i).toObject().value("animation").toObject().value("name").toString() );
            addLog( "Add anim: " + animList.back() );
        }*/


    } else if ( jsonRoot.contains("animation") ) {
        animSet = false; // exported from maya
        QJsonObject animObj = jsonRoot.value("animation").toObject();
        QString name = animObj.value("name").toString();
        double duration = animObj.value("duration").toDouble();
        framesCount = animObj.value("animBuffer").toObject().value("numFrames").toInt();
        int bonesCount = animObj.value("animBuffer").toObject().value("bones").toArray().count();
        if ( duration < 0.03 || framesCount < 1 ) {
            addLog( QString("Detected single anim with incorrect duration: %1 s, %2 frames").arg(duration).arg(framesCount), logError );
            return false;
        }
        if ( bonesCount < 1 ) {
            addLog( QString("Detected single anim with incorrect bones amount (%1 bones)").arg(bonesCount), logError );
            return false;
        }

        addLog( QString("[OK] Detected single anim (probably, exported from Maya)") );
        addLog( QString("[Info] name: %1, duration: %2 s, bones amount: %3").arg(name).arg(duration).arg(bonesCount) );
        onChanged_BlendParams();
        return true;
    } else {
        onChanged_BlendParams();
        return false;
    }
}
void W3MayaAnimUtil::onClicked_Save() {
    if ( !jsonFile.open(QFile::WriteOnly | QFile::Truncate) ) {
        addLog( QString("Can't save into file: %1").arg(jsonFile.fileName()), logError );
        return;
    }

    hasChanges = false;
    jsonDoc.setObject(jsonRoot);
    QByteArray bArray = jsonDoc.toJson();
    jsonFile.write(bArray);
    jsonFile.close();
    addLog( QString("[OK] File saved: %1").arg(jsonFile.fileName()) );
}

QJsonObject W3MayaAnimUtil::objXYZ(double X, double Y, double Z) const {
    QJsonObject ret;
    ret["x"] = QJsonValue(X);
    ret["y"] = QJsonValue(Y);
    ret["z"] = QJsonValue(Z);
    return ret;
}
void W3MayaAnimUtil::objToXYZ(QJsonObject obj, double& X, double& Y, double& Z) const {
    if (obj.isEmpty())
        qDebug() << "Empty!!";
    X = obj["x"].toDouble();
    Y = obj["y"].toDouble();
    Z = obj["z"].toDouble();
}
void W3MayaAnimUtil::objToXYZW(QJsonObject obj, double& X, double& Y, double& Z, double& W) const {
    X = obj["X"].toDouble();
    Y = obj["Y"].toDouble();
    Z = obj["Z"].toDouble();
    W = obj["W"].toDouble();
}
QJsonObject W3MayaAnimUtil::objXYZW(double X, double Y, double Z, double W) const {
    QJsonObject ret;
    ret["X"] = QJsonValue(X);
    ret["Y"] = QJsonValue(Y);
    ret["Z"] = QJsonValue(Z);
    ret["W"] = QJsonValue(W);
    return ret;
}
QJsonObject W3MayaAnimUtil::objQuanternion(double Pitch, double Yaw, double Roll) const {
    QVector4D vec4 = QQuaternion::fromEulerAngles(QVector3D(Pitch, Yaw, Roll)).toVector4D();
    return objXYZW( vec4.x(), vec4.y(), vec4.z(), vec4.w() );
}
void W3MayaAnimUtil::interpolatePos(double t, double& X1, double& Y1, double& Z1, double X2, double Y2, double Z2) {
    X1 = X1 + (X2 - X1) * t;
    Y1 = Y1 + (Y2 - Y1) * t;
    Z1 = Z1 + (Z2 - Z1) * t;
}
void W3MayaAnimUtil::interpolateRot(double t, double& X1, double& Y1, double& Z1, double& W1, double X2, double Y2, double Z2, double W2) {
    QQuaternion q1 = QQuaternion(W1, X1, Y1, Z1);
    QQuaternion q2 = QQuaternion(W2, X2, Y2, Z2);
    q1 = QQuaternion::nlerp(q1, q2, t);
    X1 = q1.x();
    Y1 = q1.y();
    Z1 = q1.z();
    W1 = q1.scalar();
}
void W3MayaAnimUtil::blendMotion(QVector<double>& motion, int animFrames, const QVector<int>& framePoints) {
    if (motion.isEmpty()) {
        motion.fill(0, animFrames + 1);
        return;
    }

    QVector<double> res;
    //qDebug() << "+++ motion: " << motion;

    int pointIdx = 0;
    double delta = 0;
    res.append( motion[0] );

    upn(frame, 1, animFrames) {
        res.append( res.back() + delta );
        //QString dbg = "";

        if (frame == framePoints[pointIdx]) {
            res.back() = motion[pointIdx];
            if (frame < animFrames) {
                delta = (motion[pointIdx + 1] - motion[pointIdx]) / (framePoints[pointIdx + 1] - framePoints[pointIdx]);
                ++pointIdx;
            }

            //dbg += "! ";
        }
        //dbg += QString("[%1] = %2").arg(frame).arg(res[frame]);
        //qDebug() << dbg;
    }
    motion = res;
}
bool W3MayaAnimUtil::applyMotionToBone(QJsonValueRef ref) {
    QJsonObject animObj = ref.toObject();
    if (animObj.isEmpty()) {
        addLog("Empty anim object, skipping.", logError);
        return false;
    }
    QString animName = animObj["name"].toString();
    qDebug() << "*** ANIM: " << animName;
    addLog("[START] Processing anim: " + animName);

    QJsonObject motionObj = animObj["motionExtraction"].toObject();
    if (ui->checkRemoveMotionChunk) {
        animObj.remove("motionExtraction");
    }
    if (motionObj.isEmpty()) {
        addLog("    Empty motionExtraction, skipping.", logWarning);
        ref = animObj;
        return false;
    }

    double animDuration = animObj["duration"].toDouble();
    QJsonObject bufferObj = animObj["animBuffer"].toObject();
    QJsonArray bonesArray = bufferObj["bones"].toArray();
    if ( bonesArray.isEmpty() ) {
        addLog("    Corrupted bones (or anim) buffer, skipping.", logError);
        return false;
    }

    /* TEST! Trajectory ! */
    if (ui->checkAddInvertedTrajectory->isChecked()) {
        int trajectoryIdx = -1, pelvisIdx = -1;
        upn(i, 0, bonesArray.size() - 1) {
            if (bonesArray[i].toObject().value("BoneName").toString() == "Trajectory") {
                trajectoryIdx = i;
            } else if (bonesArray[i].toObject().value("BoneName").toString() == "pelvis") {
                pelvisIdx = i;
            }
        }
        if (trajectoryIdx != -1 && pelvisIdx != -1) {
            QJsonObject trajectoryObj = bonesArray[trajectoryIdx].toObject();
            QJsonObject pelvisObj = bonesArray[pelvisIdx].toObject();

            int posFramesT = trajectoryObj["position_numFrames"].toInt();
            int posFramesP = pelvisObj["position_numFrames"].toInt();
            int rotFramesT = trajectoryObj["rotation_numFrames"].toInt();
            int rotFramesP = pelvisObj["rotation_numFrames"].toInt();
            QJsonArray posArrayT = trajectoryObj["positionFrames"].toArray();
            QJsonArray posArrayP = pelvisObj["positionFrames"].toArray();
            QJsonArray rotArrayT = trajectoryObj["rotationFrames"].toArray();
            QJsonArray rotArrayP = pelvisObj["rotationFrames"].toArray();

            if (posFramesT > 0 && posFramesP > 0) {
                upn(j, 0, qMin(posFramesP, posFramesT) - 1) {
                    QJsonObject objT = posArrayT[j].toObject();
                    QJsonObject objP = posArrayP[j].toObject();
                    objP["x"] = objP["x"].toDouble() - objT["x"].toDouble();
                    objP["y"] = objP["y"].toDouble() - objT["y"].toDouble();
                    objP["z"] = objP["z"].toDouble() - objT["z"].toDouble();
                    posArrayP[j] = objP;
                    posArrayT[j] = objXYZ(0, 0, 0);
                }
                pelvisObj["positionFrames"] = posArrayP;
                trajectoryObj["positionFrames"] = posArrayT;
            }
            if (rotFramesT > 0 && rotFramesP > 0) {
                upn(j, 0, qMin(rotFramesP, rotFramesT) - 1) {
                    QJsonObject objT = rotArrayT[j].toObject();
                    QJsonObject objP = rotArrayP[j].toObject();
                    QQuaternion quanT(objT["W"].toDouble(), objT["X"].toDouble(), objT["Y"].toDouble(), objT["Z"].toDouble());
                    QQuaternion quanP(objP["W"].toDouble(), objP["X"].toDouble(), objP["Y"].toDouble(), objP["Z"].toDouble());

                    QVector3D quanVecP = quanP.toEulerAngles();
                    QVector3D quanVecT = quanT.toEulerAngles();
                    QQuaternion quanRes = QQuaternion::fromEulerAngles( quanVecP.x() - quanVecT.x(), quanVecP.y() - quanVecT.y(), quanVecP.z() - quanVecT.z() );
                    rotArrayP[j] = objXYZW( quanRes.x(), quanRes.y(), quanRes.z(), quanRes.scalar() );
                    rotArrayT[j] = objXYZW(0, 0, 0);
                }
                pelvisObj["rotationFrames"] = rotArrayP;
                trajectoryObj["rotationFrames"] = rotArrayT;
            }
            bonesArray[pelvisIdx] = pelvisObj;
            bonesArray[trajectoryIdx] = trajectoryObj;
        }
    }

    int animFrames = bufferObj["numFrames"].toInt();
    int mBoneIdx = -1;
    upn(i, 0, bonesArray.size() - 1) {
    	if (bonesArray[i].toObject().value("BoneName").toString() == mBoneName) {
    		mBoneIdx = i;
            break;
    	}
    }
    if (mBoneIdx >= 0) {
    	int posFrames = bonesArray[mBoneIdx].toObject().value("position_numFrames").toInt();
    	int rotFrames = bonesArray[mBoneIdx].toObject().value("rotation_numFrames").toInt();
    	
        if ( posFrames + rotFrames > 2 && QMessageBox::Yes != QMessageBox::question(this, "Attention!", QString("Anim %1 already contains RootMotion bone with %2 position and %3 rotation frames.<br>Do you want to overwrite them with values from motionExtraction?").arg(animName).arg(posFrames).arg(rotFrames)) )
        {
            addLog(QString("    Anim already contains %1 bone with %2 position and %3 rotation frames. Skipping.").arg(mBoneName).arg(posFrames).arg(rotFrames));
            ref = animObj;
            return false;
        } else {
            addLog(QString("    Anim already contains %1 bone with %2 position and %3 rotation frames. Overwriting values from motionExtraction.").arg(mBoneName).arg(posFrames).arg(rotFrames));
        }
    } else {
        bonesArray.append(QJsonValue::Null);
        mBoneIdx = bonesArray.size() - 1;
    }
    QJsonObject mBoneObj;
    mBoneObj["BoneName"] = mBoneName;
    mBoneObj["index"] = mBoneIdx;
    mBoneObj["position_dt"] = QJsonValue(1.0 / mFps);
    mBoneObj["rotation_dt"] = QJsonValue(1.0 / mFps);
    mBoneObj["scale_dt"] = QJsonValue(1.0 / mFps);
    mBoneObj["scale_numFrames"] = QJsonValue(1);
    mBoneObj["scaleFrames"] = QJsonArray({ objXYZ(1, 1, 1) });
    
    QByteArray deltaTimes = QByteArray::fromBase64( motionObj["deltaTimes"].toString().toUtf8() );
    QVector<int> framePoints;
    framePoints.append(1);
    upn(i, 0, deltaTimes.size() - 1) {
        framePoints.append( framePoints.back() + static_cast<uint8_t>(deltaTimes[i]) );
    }

    QVector<double> motionX, motionY, motionZ, motionRotZ;
    QJsonArray framesObj = motionObj["frames"].toArray();
    int flags = motionObj["flags"].toInt();
    bool anyFlag = (flags & 15); // 1 | 2 | 4 | 8
    int framesSets = 0;

    if (anyFlag) {
        for (int i = 0; i < framesObj.size();) {
            ++framesSets;
            if (flags & BYTE_X) {
                if (i >= framesObj.size()) {
                    addLog( QString("Incomplete frames array in motionExtraction, anim: %1, setting zero RootMotion.").arg(animObj.value("name").toString()), logError);
                    anyFlag = false;
                    break;
                }
                motionX.append( framesObj[i].toDouble() );
                ++i;
            }
            if (flags & BYTE_Y) {
                if (i >= framesObj.size()) {
                    addLog( QString("Incomplete frames array in motionExtraction, anim: %1, setting zero RootMotion.").arg(animObj.value("name").toString()), logError);
                    anyFlag = false;
                    break;
                }
                motionY.append( framesObj[i].toDouble() );
                ++i;
            }
            if (flags & BYTE_Z) {
                if (i >= framesObj.size()) {
                    addLog( QString("Incomplete frames array in motionExtraction, anim: %1, setting zero RootMotion.").arg(animObj.value("name").toString()), logError);
                    anyFlag = false;
                    break;
                }
                motionZ.append( framesObj[i].toDouble() );
                ++i;
            }
            if (flags & BYTE_RotZ) {
                if (i >= framesObj.size()) {
                    addLog( QString("Incomplete frames array in motionExtraction, anim: %1, setting zero RootMotion.").arg(animObj.value("name").toString()), logError);
                    anyFlag = false;
                    break;
                }
                motionRotZ.append( framesObj[i].toDouble() * 360.0 / mW3AngleKoefficient );
                ++i;
            }
        }
    } else {
        addLog( QString("   Flags = 0 in motionExtraction, setting null motion.").arg(animName), logWarning);
    }

    int deltaTotal = 0;
    upn(i, 0, deltaTimes.size() - 1) {
        deltaTotal += static_cast<uint8_t>(deltaTimes[i]);
    }
    //qDebug() << "framesSets: " << framesSets;
    //qDebug() << "deltaTotal: " << deltaTotal;
    //qDebug() << "framePoints: " << framePoints;
    //qDebug() << "flags: " << flags;
    //qDebug() << "motX: " << motionX;
    //qDebug() << "motY: " << motionY;
    //qDebug() << "motZ: " << motionZ;
    //qDebug() << "motRotZ: " << motionRotZ;

    /* OK: deltaTimes.size() = framesSets - 1 */
    /* OK: deltaTotal = animFrames - 1        */
    if (deltaTimes.size() != framesSets - 1 || deltaTotal > animFrames || deltaTotal < animFrames - 1) {
        addLog(QString("    Incorrect motionExtraction [deltaCnt = %1, framesSets = %2, deltaTotal = %3, animFrames = %4], setting zero RootMotion.").arg(deltaTimes.size()).arg(framesSets).arg(deltaTotal).arg(animFrames), logError);
        anyFlag = false;
    }
    if (deltaTotal == animFrames) {
        addLog(QString("    Fixing motionExtraction [deltaTotal = %3 == animFrames = %4]").arg(deltaTotal).arg(animFrames), logWarning);
        deltaTimes.back() = (int)deltaTimes.back() - 1;
    }

    if (!anyFlag) {
        mBoneObj["position_numFrames"] = QJsonValue(1);
        mBoneObj["positionFrames"] = QJsonArray({ objXYZ(0, 0, 0) });
        mBoneObj["rotation_numFrames"] = QJsonValue(1);
        mBoneObj["rotationFrames"] = QJsonArray({ objXYZW(0, 0, 0) });
    } else {
        if (flags & BYTE_RotZ) {
            mBoneObj["rotation_numFrames"] = animFrames;
            QJsonArray rotationFrames;
            blendMotion(motionRotZ, animFrames, framePoints);
            upn(frame, 1, animFrames) {
                if (ui->checkUseYRot->isChecked())
                    rotationFrames.append( objQuanternion(0, motionRotZ[frame], 0) );
                else
                    rotationFrames.append( objQuanternion(0, 0, motionRotZ[frame]) );
            }
            mBoneObj["rotationFrames"] = rotationFrames;
        } else {
            mBoneObj["rotation_numFrames"] = QJsonValue(1);
            mBoneObj["rotationFrames"] = QJsonArray({ objXYZW(0, 0, 0, 1) });
        }

        if (flags & (BYTE_X | BYTE_Y | BYTE_Z)) {
            mBoneObj["position_numFrames"] = animFrames;

            QJsonArray positionFrames;
            blendMotion(motionX, animFrames, framePoints);
            blendMotion(motionY, animFrames, framePoints);
            blendMotion(motionZ, animFrames, framePoints);

            upn(frame, 1, animFrames) {
                if (ui->checkSwapYZpos->isChecked())
                    positionFrames.append( objXYZ(motionX[frame], motionZ[frame], motionY[frame]) );
                else
                    positionFrames.append( objXYZ(motionX[frame], motionY[frame], motionZ[frame]) );
            }
            mBoneObj["positionFrames"] = positionFrames;

            // ADD INVERTED PELVIS TO ROOT, if root has no motion
            /*if (ui->checkAddInverted->isChecked()) {
                int rootIndex = -1, pelvisIndex = -1;
                upn(i, 0, bonesArray.size() - 1) {
                    if (bonesArray[i].toObject().value("BoneName").toString() == "Root") {
                        rootIndex = i;
                    }
                    if (bonesArray[i].toObject().value("BoneName").toString() == "pelvis") {
                        pelvisIndex = i;
                    }
                }
                QJsonObject pelvisBone = bonesArray[pelvisIndex].toObject();
                QJsonObject rootBone = bonesArray[rootIndex].toObject();
                int pelvisFrames = pelvisBone.value("position_numFrames").toInt();
                int rootFrames = rootBone.value("position_numFrames").toInt();
                if (pelvisFrames > 1 && rootFrames == 1) {
                    rootBone["position_numFrames"] = pelvisFrames;
                    QJsonArray pelvisPositionArray = pelvisBone.value("positionFrames").toArray();
                    QJsonObject rootOriginalKeys = rootBone.value("positionFrames").toArray().at(0).toObject();
                    upn(j, 0, pelvisPositionArray.size() - 1) {
                        QJsonObject frameKeys = pelvisPositionArray[j].toObject();
                        if (flags & BYTE_X) {
                            frameKeys["x"] = - frameKeys.value("x").toDouble();
                        } else {
                            frameKeys["x"] = rootOriginalKeys.value("x");
                        }
                        if (flags & BYTE_Y) {
                            frameKeys["y"] = - frameKeys.value("y").toDouble();
                        } else {
                            frameKeys["y"] = rootOriginalKeys.value("y");
                        }
                        if (flags & BYTE_Z) {
                            frameKeys["z"] = - frameKeys.value("z").toDouble();
                        } else {
                            frameKeys["z"] = rootOriginalKeys.value("z");
                        }
                        pelvisPositionArray[j] = frameKeys;
                    }

                    rootBone["positionFrames"] = pelvisPositionArray;
                    bonesArray[rootIndex] = rootBone;
                    addLog("    Added inverted pelvis motion to Root!");
                }
            }*/
        } else {
            mBoneObj["position_numFrames"] = QJsonValue(1);
            mBoneObj["positionFrames"] = QJsonArray({ objXYZ(0, 0, 0) });
        }

        addLog("    [FINISH] Processed anim: " + animName);
    }

    // END
    bonesArray[mBoneIdx] = mBoneObj;
    bufferObj["bones"] = bonesArray;
    animObj["animBuffer"] = bufferObj;
    ref = animObj;
    
    return true;
}
void W3MayaAnimUtil::onClicked_applyMotionToBone() {
    eTimer.start();
    ui->progressBar->setValue(0);

    if (animSet) {
        QJsonArray animArray = jsonRoot["animations"].toArray();
        upn(i, 0, animArray.size() - 1) {
            QJsonObject animObj = animArray[i].toObject();
            if ( applyMotionToBone(animObj["animation"]) ) {
                hasChanges = true;
            }

            animArray[i] = animObj;
            ui->progressBar->setValue((i + 1.0) * 100.0 / animArray.size());
            // YES it's dirty crutch
            QApplication::processEvents();
        }

        jsonRoot["animations"] = animArray;
        addLog(QString("[Finished in %1s] Processed %2 animations.<br>").arg(eTimer.elapsed() / 1000.0).arg(animArray.size()));
    } else {
        if ( applyMotionToBone(jsonRoot["animation"]) ) {
            hasChanges = true;
        }
        addLog(QString("[Finished in %1s] Processed 1 animation.<br>").arg(eTimer.elapsed() / 1000.0));
        ui->progressBar->setValue(100);
    }
}

bool W3MayaAnimUtil::extractMotionFromBone(QJsonValueRef ref) {
    QJsonObject animObj = ref.toObject();
    if (animObj.isEmpty()) {
        addLog("Empty anim object, skipping.", logError);
        return false;
    }
    QString animName = animObj["name"].toString();
    qDebug() << "*** ANIM: " << animName;
    addLog("[START] Processing anim: " + animName);

    if ( animObj.contains("motionExtraction") ) {
        addLog("    Overwriting existing motionExtraction.", logWarning);
        animObj.remove("motionExtraction");
    }

    double animDuration = animObj["duration"].toDouble();
    /* extra nr */
    animNames.push_back(animName);
    animDurations.push_back(animDuration);

    QJsonObject bufferObj = animObj["animBuffer"].toObject();
    QJsonArray bonesArray = bufferObj["bones"].toArray();
    if ( bonesArray.isEmpty() ) {
        addLog("Empty bones/anim buffer, skipping.", logError);
        return false;
    }

    int animFrames = bufferObj["numFrames"].toInt();
    int mBoneIdx = -1;
    QStringList brokenBones;
    int bonesOptimized = 0;

    upn(i, 0, bonesArray.size() - 1) {
        QJsonObject tempObj = bonesArray[i].toObject();
        QString boneName = tempObj.value("BoneName").toString();

        if (boneName == mBoneName) {
            mBoneIdx = i;
        }
        int posNum = tempObj["position_numFrames"].toInt();
        int rotNum = tempObj["rotation_numFrames"].toInt();
        int scaleNum = tempObj["scale_numFrames"].toInt();

        if ( ui->checkOptimizeEqual->isChecked() ) {
            bool optimizeTranslation = posNum > 1;
            bool optimizeRotation = rotNum > 1;
            bool optimizeScale = scaleNum > 1;
            QJsonArray posArray = tempObj["positionFrames"].toArray();
            QJsonArray rotArray = tempObj["rotationFrames"].toArray();
            QJsonArray scaleArray = tempObj["scaleFrames"].toArray();

            upn(frame, 1, posNum - 1) {
                double diffX = posArray[frame].toObject().value("x").toDouble() - posArray[frame - 1].toObject().value("x").toDouble();
                double diffY = posArray[frame].toObject().value("y").toDouble() - posArray[frame - 1].toObject().value("y").toDouble();
                double diffZ = posArray[frame].toObject().value("z").toDouble() - posArray[frame - 1].toObject().value("z").toDouble();
                double diffTotal = qAbs(diffX) + qAbs(diffY) + qAbs(diffZ);
                if (diffTotal > mReductionSensitivity()) {
                    optimizeTranslation = false;
                    break;
                }
            }
            upn(frame, 1, rotNum - 1) {
                double diffX = rotArray[frame].toObject().value("X").toDouble() - rotArray[frame - 1].toObject().value("X").toDouble();
                double diffY = rotArray[frame].toObject().value("Y").toDouble() - rotArray[frame - 1].toObject().value("Y").toDouble();
                double diffZ = rotArray[frame].toObject().value("Z").toDouble() - rotArray[frame - 1].toObject().value("Z").toDouble();
                double diffW = rotArray[frame].toObject().value("W").toDouble() - rotArray[frame - 1].toObject().value("W").toDouble();
                double diffTotal = qAbs(diffX) + qAbs(diffY) + qAbs(diffZ) + qAbs(diffW);
                if (diffTotal > mReductionSensitivity()) {
                    optimizeRotation = false;
                    break;
                }
            }
            upn(frame, 1, scaleNum - 1) {
                double diffX = scaleArray[frame].toObject().value("x").toDouble() - scaleArray[frame - 1].toObject().value("x").toDouble();
                double diffY = scaleArray[frame].toObject().value("y").toDouble() - scaleArray[frame - 1].toObject().value("y").toDouble();
                double diffZ = scaleArray[frame].toObject().value("z").toDouble() - scaleArray[frame - 1].toObject().value("z").toDouble();
                double diffTotal = qAbs(diffX) + qAbs(diffY) + qAbs(diffZ);
                if (diffTotal > mReductionSensitivity()) {
                    optimizeScale = false;
                    break;
                }
            }
            if (optimizeTranslation) {
                QJsonArray nposArray;
                nposArray.append( posArray.first() );
                tempObj["positionFrames"] = nposArray;
                tempObj["position_numFrames"] = 1;
                posNum = 1;
            }
            if (optimizeRotation) {
                QJsonArray nrotArray;
                nrotArray.append( rotArray.first() );
                tempObj["rotationFrames"] = nrotArray;
                tempObj["rotation_numFrames"] = 1;
                rotNum = 1;
            }
            if (optimizeScale) {
                QJsonArray nscaleArray;
                nscaleArray.append( scaleArray.first() );
                tempObj["scaleFrames"] = nscaleArray;
                tempObj["scale_numFrames"] = 1;
                scaleNum = 1;
            }
            if (optimizeTranslation || optimizeRotation || optimizeScale) {
                bonesArray[i] = tempObj;
                bonesOptimized += 1;
            }
        }

        QString error;
        if (posNum != animFrames && posNum != 1)
            error += QString(" [translation = %1];").arg(posNum);
        if (rotNum != animFrames && rotNum != 1)
            error += QString(" [rotation = %1];").arg(rotNum);
        if (scaleNum != animFrames && scaleNum != 1)
            error += QString(" [scale = %1];").arg(scaleNum);

        if (!error.isEmpty()) {
            brokenBones.append( boneName + ":" + error );
        }
    }

    bool isAdditive = animName.contains("additive");

    if ( ui->checkDetectAdditives->isChecked() ) {
        if (!isAdditive) {
            int nullMotionBones = 0;
            QStringList nullBones;
            // count bones which seems to be "null"-animated
            // having some of such bones is typical for additive anim

            upn(i, 0, bonesArray.size() - 1) {
                QJsonObject tempObj = bonesArray[i].toObject();
                QString boneName = tempObj.value("BoneName").toString();
                int posNum = tempObj["position_numFrames"].toInt();
                int rotNum = tempObj["rotation_numFrames"].toInt();
                int scaleNum = tempObj["scale_numFrames"].toInt();
                if (posNum == 1 && rotNum == 1) {
                    QJsonObject tpos = tempObj["positionFrames"].toArray().at(0).toObject();
                    QJsonObject trot = tempObj["rotationFrames"].toArray().at(0).toObject();
                    double sumPos = qAbs(tpos.value("x").toDouble()) + qAbs(tpos.value("y").toDouble()) + qAbs(tpos.value("z").toDouble());
                    double sumRot = qAbs(trot.value("X").toDouble()) + qAbs(trot.value("Y").toDouble()) + qAbs(trot.value("Z").toDouble());
                    double diffW = qAbs( qAbs(trot.value("W").toDouble()) - 1.0 );
                    //qDebug() << QString("Bone %1, sumPos = %2, sumRot = %3, diffW = %4").arg(boneName).arg(sumPos).arg(sumRot).arg(diffW);
                    if (sumPos < mReductionSensitivity() && sumRot < 0.006 && diffW < mReductionSensitivity()) {
                        ++nullMotionBones;
                        nullBones.append(boneName);
                    }
                }
            }

            if (nullMotionBones > 6 && (nullBones.contains("r_weapon") || nullBones.contains("l_weapon"))) {
                //qDebug() << "Additive anim detected! Bones: " << nullBones.join(", ");
                isAdditive = true;
            }
        }

        if (isAdditive) {
            bonesOptimized += 1;
            additiveNames.append(animName);
            addLog("    [ADDITIVE] Detected! Fixing bones.");

            upn(i, 0, bonesArray.size() - 1) {
                QJsonObject tempObj = bonesArray[i].toObject();
                QString boneName = tempObj.value("BoneName").toString();
                int posNum = tempObj["position_numFrames"].toInt();
                int rotNum = tempObj["rotation_numFrames"].toInt();
                int scaleNum = tempObj["scale_numFrames"].toInt();
                if (posNum == 1) {
                    QJsonArray nArray;
                    nArray.append( objXYZ(0, 0, 0) );
                    tempObj["positionFrames"] = nArray;
                }
                if (rotNum == 1) {
                    QJsonArray nArray;
                    nArray.append( objXYZW(-0.00048828125, -0.00048828125, -0.00048828125, 1.0) );
                    tempObj["rotationFrames"] = nArray;
                }
                if (scaleNum == 1) {
                    QJsonArray nArray;
                    nArray.append( objXYZ(1.0, 1.0, 1.0) );
                    tempObj["scaleFrames"] = nArray;
                }
                bonesArray[i] = tempObj;
            }
        }
    }

    if (bonesOptimized) {
        addLog( QString("    [OPTIMIZE] Set single frame to %1 bones.").arg(bonesOptimized) );
    }

    if ( !brokenBones.isEmpty() ) {
        QMessageBox::information(this, "Warning!", QString("Detected bones with incorrect numFrames in anim [%1] (numFrames = %2)\nIt may break the game!\nBones: %3").arg(animName).arg(animFrames).arg(brokenBones.join("\n")));
    }
    if (mBoneIdx == -1) {
        if (bonesOptimized) {
            bufferObj["bones"] = bonesArray;
            animObj["animBuffer"] = bufferObj;
        }
        if (ui->checkUseExternalMotion->isChecked() && motionByName.contains(animName)) {
            addLog(QString("    Anim doesn't contain [%2] bone, using external motionExtraction!").arg(mBoneName) );
            animObj["motionExtraction"] = motionByName[animName];
            ref = animObj;
            return true;
        } else if (ui->checkIgnoreEmptyRootMotion->isChecked()) {
            addLog(QString("    Anim doesn't contain [%2] bone, ignoring.").arg(mBoneName), logError );
            return bonesOptimized > 0;
        } else {
            addLog(QString("    Anim doesn't contain [%2] bone, removing motionExtraction!").arg(mBoneName), logError );
            ref = animObj;
            return true;
        }
    }

    QJsonObject motionObj = bonesArray[mBoneIdx].toObject();
    bonesArray.removeAt(mBoneIdx);
    bufferObj["bones"] = bonesArray;
    animObj["animBuffer"] = bufferObj;

    int posFrames = motionObj["position_numFrames"].toInt();
    int rotFrames = motionObj["rotation_numFrames"].toInt();
    QJsonArray posArray = motionObj["positionFrames"].toArray();
    QJsonArray rotArray = motionObj["rotationFrames"].toArray();

    if ( posFrames + rotFrames < 2 || posArray.size() != posFrames || rotArray.size() != rotFrames
         || (posFrames > 1 && posFrames != animFrames) || (rotFrames > 1 && rotFrames != animFrames) )
    {
        addLog(QString("    Bone [%2] defition is incomplete, removing motionExtraction!").arg(mBoneName), logError );
        // END
        ref = animObj;
        return true;
    } else if ( posFrames + rotFrames == 2 )
    {
        addLog(QString("    Bone [%2] has only 1 frame for pos and rot, removing motionExtraction!").arg(mBoneName), logWarning );
        // END
        ref = animObj;
        return true;
    }

    QVector<double> motionX, motionY, motionZ, motionRotZ;
    int flags = BYTE_X | BYTE_Y | BYTE_Z | BYTE_RotZ; // 15
    upn(frame, 0, posFrames - 1) {
        motionX.append( posArray[frame].toObject().value("x").toDouble() );
        if (ui->checkSwapYZpos_2->isChecked()) {
            motionY.append( posArray[frame].toObject().value("z").toDouble());
            motionZ.append( posArray[frame].toObject().value("y").toDouble() );
        } else {
            motionY.append( posArray[frame].toObject().value("y").toDouble());
            motionZ.append( posArray[frame].toObject().value("z").toDouble() );
        }
    }
    upn(frame, 0, rotFrames - 1) {
        QJsonObject rotFrameObj = rotArray[frame].toObject();
        QQuaternion quat = QQuaternion(rotFrameObj["W"].toDouble(), rotFrameObj["X"].toDouble(), rotFrameObj["Y"].toDouble(), rotFrameObj["Z"].toDouble());
        if (ui->checkUseYRot2->isChecked())
            motionRotZ.append( quat.toEulerAngles().y() * mW3AngleKoefficient / 360.0 );
        else
            motionRotZ.append( quat.toEulerAngles().z() * mW3AngleKoefficient / 360.0 );
    }

    if (rotFrames == 1) {
        motionRotZ.fill( motionRotZ.first(), animFrames );
        rotFrames = animFrames;
    }
    if (posFrames == 1) {
        motionX.fill( motionX.first(), animFrames );
        motionY.fill( motionY.first(), animFrames );
        motionZ.fill( motionZ.first(), animFrames );
        posFrames = animFrames;
    }
    QVector<int> deltaTimes;

    if (ui->checkRemoveLinear->isChecked()) {
        int lastDelta = 0;
        double deltaX = 1e9, deltaY = 1e9, deltaZ = 1e9, deltaRotZ = 1e9;
        QVector<double> tmp_motionX, tmp_motionY, tmp_motionZ, tmp_motionRotZ;

        upn(frame, 0, animFrames - 1 - 1) {
            double tmp_deltaX = motionX[frame + 1] - motionX[frame];
            double tmp_deltaY = motionY[frame + 1] - motionY[frame];
            double tmp_deltaZ = motionZ[frame + 1] - motionZ[frame];
            double tmp_deltaRotZ = motionRotZ[frame] - motionRotZ[frame];
            double tmp_totalDelta = qAbs(tmp_deltaX - deltaX) + qAbs(tmp_deltaY - deltaY)
                            + qAbs(tmp_deltaZ - deltaZ) + qAbs(tmp_deltaRotZ - deltaRotZ);
            //qDebug() << QString("[%1] total delta: %2,   X: %3, Y: %4, Z: %5, Rot: %6").arg(frame).arg(tmp_totalDelta).arg(tmp_deltaX).arg(tmp_deltaY).arg(tmp_deltaZ).arg(tmp_deltaRotZ);

            if (tmp_totalDelta < mReductionSensitivity() && frame - lastDelta < 255) {
                // current->next transition: still linear!
                // or value is 255 (maximum for uint8)
            } else {
                // current is new control point!
                if (frame > 0)
                    deltaTimes.append(frame - lastDelta);
                lastDelta = frame;
                tmp_motionX.append(motionX[frame]);
                tmp_motionY.append(motionY[frame]);
                tmp_motionZ.append(motionZ[frame]);
                tmp_motionRotZ.append(motionRotZ[frame]);

                deltaX = tmp_deltaX;
                deltaY = tmp_deltaY;
                deltaZ = tmp_deltaZ;
                deltaRotZ = tmp_deltaRotZ;
            }
        }
        int frame = animFrames - 1;
        if (frame - 1 > 0)
            deltaTimes.append(frame - lastDelta);
        tmp_motionX.append(motionX[frame]);
        tmp_motionY.append(motionY[frame]);
        tmp_motionZ.append(motionZ[frame]);
        tmp_motionRotZ.append(motionRotZ[frame]);
        // ^ last frame

        motionX = tmp_motionX;
        motionY = tmp_motionY;
        motionZ = tmp_motionZ;
        motionRotZ = tmp_motionRotZ;
        rotFrames = posFrames = motionX.size();
        //qDebug() << "deltaTimes: " << deltaTimes;
        //qDebug() << "Reduce: " << animFrames << " -> " << posFrames << " frames.";
        addLog(QString("    [OPTIMIZE] Removed excess frames: %1 -> %2.").arg(animFrames).arg(posFrames));
    } else {
        deltaTimes.fill(1, animFrames - 1);
    }

    if (ui->checkRemoveAxises->isChecked()) {
        bool isSingleX = true;
        bool isSingleY = true;
        bool isSingleZ = true;
        bool isSingleRotZ = true;
        upn(frame, 0, posFrames - 1 - 1) {
            if (qAbs(motionX[frame + 1] - motionX[frame]) > mReductionSensitivity())
                isSingleX = false;
            if (qAbs(motionY[frame + 1] - motionY[frame]) > mReductionSensitivity())
                isSingleY = false;
            if (qAbs(motionZ[frame + 1] - motionZ[frame]) > mReductionSensitivity())
                isSingleZ = false;
            if (qAbs(motionRotZ[frame + 1] - motionRotZ[frame]) > mReductionSensitivity())
                isSingleRotZ = false;
        }

        if (isSingleX) {
            flags = flags ^ BYTE_X;
            addLog(QString("    [OPTIMIZE] Remove unused X axis frames."));
        }
        if (isSingleY) {
            flags = flags ^ BYTE_Y;
            addLog(QString("    [OPTIMIZE] Remove unused Y axis frames."));
        }
        if (isSingleZ) {
            flags = flags ^ BYTE_Z;
            addLog(QString("    [OPTIMIZE] Remove unused Z axis frames."));
        }
        if (isSingleRotZ) {
            flags = flags ^ BYTE_RotZ;
            addLog(QString("    [OPTIMIZE] Remove unused Rotation Z axis frames."));
        }
    }
    if (flags == 0) {
        addLog(QString("    [OK] After optimizing frames it came out that bone [%2] doesn't contain any motion. Applying null motionExtraction").arg(mBoneName), logWarning );
        // END
        ref = animObj;
        return true;
    }

    QJsonObject motionExtractionObj;
    motionExtractionObj["duration"] = animDuration;
    motionExtractionObj["flags"] = flags;

    QJsonArray framesArray;
    upn(frame, 0, posFrames - 1) {
        if (flags & BYTE_X) {
            framesArray.append( motionX[frame] );
        }
        if (flags & BYTE_Y) {
            framesArray.append( motionY[frame] );
        }
        if (flags & BYTE_Z) {
            framesArray.append( motionZ[frame] );
        }
        if (flags & BYTE_RotZ) {
            framesArray.append( motionRotZ[frame] );
        }
    }
    motionExtractionObj["frames"] = framesArray;

    if (ui->checkBase64->isChecked()) {
        QByteArray deltaBase64;
        upn(frame, 0, deltaTimes.size() - 1) {
            deltaBase64.append( static_cast<char>(deltaTimes[frame]) );
        }
        motionExtractionObj["deltaTimes"] = QString( deltaBase64.toBase64() );
    } else {
        QJsonArray deltaArray;
        upn(frame, 0, deltaTimes.size() - 1) {
            deltaArray.append( deltaTimes[frame] );
        }
        motionExtractionObj["deltaTimes"] = deltaArray;
    }


    // END
    addLog("    [FINISH] Processed anim: " + animName);
    animObj["motionExtraction"] = motionExtractionObj;
    ref = animObj;
    return true;
}
void W3MayaAnimUtil::onClicked_extractMotionFromBone() {
    eTimer.start();
    if (!batchMode)
        ui->progressBar->setValue(0);

    if (animSet) {
        QJsonArray animArray = jsonRoot["animations"].toArray();
        upn(i, 0, animArray.size() - 1) {
            QJsonObject animObj = animArray[i].toObject();
            if ( extractMotionFromBone(animObj["animation"]) ) {
                hasChanges = true;
            }

            if ( ui->checkLoadAnimEvents->isChecked() ) {
                QJsonValue nameVal = animObj.value("animation").toObject().value("name");
                QString animName = (nameVal.isString() ? nameVal.toString() : "NONE");
                if ( animEventsByName.contains(animName) ) {
                    animObj["entries"] = animEventsByName[animName];
                    addLog( QString("    [EVENTS] Added %1 entries").arg(animEventsByName[animName].count()) );
                }
            }

            animArray[i] = animObj;
            ui->progressBar->setValue((i + 1.0) * 100.0 / animArray.size());
            // YES it's dirty crutch
            QApplication::processEvents();
        }

        jsonRoot["animations"] = animArray;
        addLog(QString("[Finished in %1s] Processed %2 animations.<br>").arg(eTimer.elapsed() / 1000.0).arg(animArray.size()));
    } else {
        if ( extractMotionFromBone(jsonRoot["animation"]) ) {
            hasChanges = true;
        }
        if ( ui->checkLoadAnimEvents->isChecked() ) {
            QJsonValue nameVal = jsonRoot.value("animation").toObject().value("name");
            QString animName = (nameVal.isString() ? nameVal.toString() : "NONE");
            if ( animEventsByName.contains(animName) ) {
                jsonRoot["entries"] = animEventsByName[animName];
                addLog( QString("    [EVENTS] Added %1 entries").arg(animEventsByName[animName].count()) );
            }
        }

        addLog(QString("[Finished in %1s] Processed 1 animation.<br>").arg(eTimer.elapsed() / 1000.0));
        if (!batchMode)
            ui->progressBar->setValue(100);
    }
}
void W3MayaAnimUtil::onClicked_extractMotionFromBoneBatch() {
    batchMode = true;
    ui->progressBar->setValue(0);
    additiveNames.clear();
    animNames.clear();
    animDurations.clear();

    QString dirName = QFileDialog::getExistingDirectory(this, tr("Choose directory"),
                                                        QDir::currentPath());
    if (dirName.isEmpty()) {
        addLog("User canceled operation.", logWarning);
        return;
    }
    QStringList jsonList = QDir(dirName).entryList({"*.json"}, QDir::Files, QDir::Name);

    int current = 0;
    for (const QString jsonPath : jsonList) {
        ++current;
        addLog("[BATCH] Processing file: " + jsonPath);
        if (loadJsonFile(dirName + "/" + jsonPath)) {
            onClicked_extractMotionFromBone();
            // YES it's dirty crutch
            QApplication::processEvents();
            if (hasChanges) {
                onClicked_Save();
            }
        }
        ui->progressBar->setValue( current * 100.0 / jsonList.count() );
        // YES it's dirty crutch
        QApplication::processEvents();
    }
    addLog(QString("[BATCH] Completed processing %1 files.").arg(jsonList.size()));

    /* extra nr */
    QString res = QString("ADDITIVE anims: %1\n").arg(additiveNames.count());
    for (QString i : additiveNames) {
        res += "        " + i + "\n";
    }
    res += "\n||||||||||||||||||||||||||\n\n";

    res += "    function addAnims() {\n";
    upn(i, 0, animNames.size() - 1) {
        res += QString("		addAnim('%1', %2);\n").arg(animNames[i]).arg(animDurations[i]);
    }
    res += "    }\n";
    QTextEdit* textEdit = new QTextEdit();
    textEdit->setFontPointSize(11);
    textEdit->setPlainText(res);
    textEdit->showMaximized();

    ui->progressBar->setValue(100);
    batchMode = false;
}

bool W3MayaAnimUtil::loadJsonAnimEvents(QString filePath) {
    jsonFileEvents.setFileName(filePath);
    if ( !jsonFileEvents.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        addLog("Can't open file in read-only text mode: " + filePath, logError);
        return false;
    }

    QByteArray bArray = jsonFileEvents.readAll();
    QJsonParseError* jsonError = new QJsonParseError;
    jsonDocEvents = QJsonDocument::fromJson(bArray, jsonError);
    if ( jsonDocEvents.isNull() ) {
        addLog( QString("Can't load json document correctly, parse error = %1").arg(jsonError->errorString()), logError );
        return false;
    } else if ( !jsonDocEvents.isObject() ) {
        addLog( "Json root is not an object, can't load info.", logError );
        return false;
    }

    /*if ( QFile::exists(filePath + ".bak") ) {
        QFile::remove(filePath + ".bak");
    }
    jsonFileEvents.copy(filePath + ".bak");*/
    jsonFileEvents.close();
    jsonRootEvents = jsonDocEvents.object();

    QJsonArray animArray = jsonRootEvents.value("animations").toArray();// "animations"].toArray();
    if (animArray.isEmpty()) {
        addLog( "Json file doesn't contain animations object.", logError );
        return false;
    }

    animEventsByName.clear();
    bool cleanFile = false;
    bool cleanAsked = false;
    upn(i, 0, animArray.size() - 1) {
        QJsonObject animObj = animArray[i].toObject().value("animation").toObject();

        // debug!
        QJsonObject animObj2 = animArray[i].toObject();
        if (animObj.contains("animBuffer") && (cleanFile || !cleanAsked)) {
            if (!cleanAsked) {
                if (QMessageBox::Yes == QMessageBox::question(this, "REMOVE buffer?", "Do you want to remove excess animBuffers from events .json file?", QMessageBox::Yes, QMessageBox::No)) {
                    cleanFile = true;
                }
                cleanAsked = true;
            }
            if (cleanFile) {
                animObj.remove("animBuffer");
                animObj2["animation"] = animObj;
                animArray[i] = animObj2;
            }
        }
        // debug!

        QJsonValue nameVal = animObj.value("name");
        if (!nameVal.isString()) {
            addLog( QString("Json: animation [%1]: can't parse name.").arg(i), logError );
            continue;
        }
        QJsonArray eventsArr = animArray[i].toObject().value("entries").toArray();
        if (!eventsArr.isEmpty()) {
            animEventsByName[nameVal.toString()] = eventsArr;
            //addLog( QString("Json: add events for animation [%1]: %2.").arg(i).arg(nameVal.toString()) );
        } else {
            //addLog( QString("Json: animation [%1]: can't parse entry arr.").arg(i), logError );
        }
    }

    // debug!
    if (cleanFile) {
        jsonRootEvents["animations"] = animArray;
        if ( !jsonFileEvents.open(QFile::WriteOnly | QFile::Truncate) ) {
            addLog( QString("Can't save into file: %1").arg(jsonFileEvents.fileName()), logError );
            return false;
        }
        jsonDocEvents.setObject(jsonRootEvents);
        bArray = jsonDocEvents.toJson();
        jsonFileEvents.write(bArray);
        jsonFileEvents.close();
    }

    return true;
}
void W3MayaAnimUtil::onClicked_loadAnimEventsSource() {
    QString filePath = QFileDialog::getOpenFileName(this, "Open animEvents json", "", "JSON Files (*.json)");
    if (filePath.isEmpty()) {
        addLog("Loading file canceled by user", logWarning);
        return;
    }
    if ( loadJsonAnimEvents(filePath) ) {
        ui->checkLoadAnimEvents->setText( QString("entries: Add anim events from source file (currently loaded: %1)").arg(QFileInfo(filePath).fileName()) );
    } else {
        ui->checkLoadAnimEvents->setText( QString("entries: Add anim events from source file (currently loaded: %1)").arg("NONE") );
    }
}

bool W3MayaAnimUtil::loadJsonMotion(QString filePath) {
    jsonFileMotion.setFileName(filePath);
    if ( !jsonFileMotion.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        addLog("Can't open file in read-only text mode: " + filePath, logError);
        return false;
    }

    QByteArray bArray = jsonFileMotion.readAll();
    QJsonParseError* jsonError = new QJsonParseError;
    jsonDocMotion = QJsonDocument::fromJson(bArray, jsonError);
    if ( jsonDocMotion.isNull() ) {
        addLog( QString("Can't load json document correctly, parse error = %1").arg(jsonError->errorString()), logError );
        return false;
    } else if ( !jsonDocMotion.isObject() ) {
        addLog( "Json root is not an object, can't load info.", logError );
        return false;
    }

    /*if ( QFile::exists(filePath + ".bak") ) {
        QFile::remove(filePath + ".bak");
    }
    jsonFileEvents.copy(filePath + ".bak");*/
    jsonFileMotion.close();
    jsonRootMotion = jsonDocMotion.object();

    QJsonArray animArray = jsonRootMotion.value("animations").toArray();// "animations"].toArray();
    if (animArray.isEmpty()) {
        addLog( "Json file doesn't contain animations object.", logError );
        return false;
    }

    motionByName.clear();
    bool cleanFile = false;
    bool cleanAsked = false;
    upn(i, 0, animArray.size() - 1) {
        QJsonObject animObj = animArray[i].toObject().value("animation").toObject();

        // debug!
        QJsonObject animObj2 = animArray[i].toObject();
        if (animObj.contains("animBuffer") && (cleanFile || !cleanAsked)) {
            if (!cleanAsked) {
                if (QMessageBox::Yes == QMessageBox::question(this, "REMOVE buffer?", "Do you want to remove excess animBuffers from motion .json file?", QMessageBox::Yes, QMessageBox::No)) {
                    cleanFile = true;
                }
                cleanAsked = true;
            }
            if (cleanFile) {
                animObj.remove("animBuffer");
                animObj2["animation"] = animObj;
                animArray[i] = animObj2;
            }
        }
        // debug!

        QJsonValue nameVal = animObj.value("name");
        if (!nameVal.isString()) {
            addLog( QString("Json: animation [%1]: can't parse name.").arg(i), logError );
            continue;
        }
        QJsonObject motionObj = animArray[i].toObject().value("animation").toObject().value("motionExtraction").toObject();
        if ( !motionObj.isEmpty() ) {
            motionByName[nameVal.toString()] = motionObj;
            //addLog( QString("Json: add motion for animation [%1]: %2.").arg(i).arg(nameVal.toString()) );
        } else {
            //addLog( QString("Json: animation [%1]: %2: can't parse motion obj.").arg(i).arg(nameVal.toString()), logError );
        }
    }

    // debug!
    if (cleanFile) {
        jsonRootMotion["animations"] = animArray;
        if ( !jsonFileMotion.open(QFile::WriteOnly | QFile::Truncate) ) {
            addLog( QString("Can't save into file: %1").arg(jsonFileMotion.fileName()), logError );
            return false;
        }
        jsonDocMotion.setObject(jsonRootMotion);
        bArray = jsonDocMotion.toJson();
        jsonFileMotion.write(bArray);
        jsonFileMotion.close();
    }

    return true;
}
void W3MayaAnimUtil::onClicked_loadMotionSource() {
    QString filePath = QFileDialog::getOpenFileName(this, "Open external motion json", "", "JSON Files (*.json)");
    if (filePath.isEmpty()) {
        addLog("Loading file canceled by user", logWarning);
        return;
    }
    if ( loadJsonMotion(filePath) ) {
        ui->checkUseExternalMotion->setText( QString("motion: Use external file if it contains anim with the same name (currently loaded: %1)").arg(QFileInfo(filePath).fileName()) );
    } else {
        ui->checkUseExternalMotion->setText( QString("motion: Use external file if it contains anim with the same name (currently loaded: %1)").arg("NONE") );
    }
}


void W3MayaAnimUtil::onChanged_BlendParams() {
    if (framesCount != -1) {
        ui->labelBlendTarget->setText(QString("Target anim duration: %1 frames").arg(framesCount));
        ui->spinBlendTarget->setMaximum(framesCount);
    } else
        ui->labelBlendTarget->setText(QString("Target anim duration: NONE"));

    if (blendSourceFramesCount != -1) {
        ui->labelBlendSource->setText(QString("Source anim duration: %1 frames").arg(blendSourceFramesCount));
        ui->spinBlendSource->setMaximum(blendSourceFramesCount);
    } else
        ui->labelBlendSource->setText(QString("Source anim duration: NONE"));

    if (animSet == true || framesCount == -1 || blendSourceFramesCount == -1) {
        ui->buttonBlendDo->setEnabled(false);
        return;
    }
    if ( ui->spinBlendSource->value() + ui->spinBlendDuration->value() - 1 > framesCount ||
         ui->spinBlendTarget->value() + ui->spinBlendDuration->value() - 1 > blendSourceFramesCount ) {
        ui->buttonBlendDo->setEnabled(false);
        return;
    }

    ui->buttonBlendDo->setEnabled(true);
    onChanged_BlendFrames();
}

void W3MayaAnimUtil::onChanged_BlendFrames() {
    int frames1 = framesCount > 0 ? (framesCount - ui->spinBlendTarget->value() + 1) : 100000;
    int frames2 = blendSourceFramesCount > 0 ? (blendSourceFramesCount - ui->spinBlendSource->value() + 1) : 100000;
    qDebug() << "frames1 = " << frames1 << ", frames2 = " << frames2;
    ui->spinBlendDuration->setMaximum( qMax(0, qMin(frames1, frames2)) );
}
bool W3MayaAnimUtil::loadBlendJson(QString filePath) {
    blendSourceFramesCount = -1;
    jsonFileBlend.setFileName(filePath);
    if ( !jsonFileBlend.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        addLog("Can't open file in read-only text mode: " + filePath, logError);
        return false;
    }

    QByteArray bArray = jsonFileBlend.readAll();
    QJsonParseError* jsonError = new QJsonParseError;
    jsonDocBlend = QJsonDocument::fromJson(bArray, jsonError);
    if ( jsonDocBlend.isNull() ) {
        addLog( QString("Can't load json document correctly, parse error = %1").arg(jsonError->errorString()), logError );
        return false;
    } else if ( !jsonDocBlend.isObject() ) {
        addLog( "Json root is not an object, can't load info.", logError );
        return false;
    }
    jsonFileBlend.close();
    jsonRootBlend = jsonDocBlend.object();

    QJsonObject animObject = jsonRootBlend.value("animation").toObject();
    QJsonValue numFramesVal = animObject.value("animBuffer").toObject().value("numFrames");
    if (numFramesVal.isNull()) {
        addLog("Error loading blend json: can't find numFrames!", logError);
        return false;
    }
    blendSourceFramesCount = numFramesVal.toDouble();
    qDebug() << "numFrames = " << blendSourceFramesCount;

    return true;
}
void W3MayaAnimUtil::onClicked_LoadBlendJson() {
    QString filePath = QFileDialog::getOpenFileName(this, "Open blend source json", "", "JSON Files (*.json)");
    if (filePath.isEmpty()) {
        addLog("Loading file canceled by user", logWarning);
        return;
    }
    if ( loadBlendJson(filePath) ) {
        ui->lineBlendPath->setText( QString("%1").arg(QFileInfo(filePath).fileName()) );
    } else {
        ui->lineBlendPath->setText( QString("ERROR Loading file: %1").arg(QFileInfo(filePath).fileName()) );
    }
    onChanged_BlendParams();
}
void W3MayaAnimUtil::onClicked_Blend() {
    int targetFrame = ui->spinBlendTarget->value();
    int sourceFrame = ui->spinBlendSource->value();
    int done = 0;
    int total = ui->spinBlendDuration->value();

    QJsonObject animObjT = jsonRoot.value("animation").toObject();
    QJsonObject animBuffT = animObjT.value("animBuffer").toObject();
    QJsonArray animBonesT = animBuffT.value("bones").toArray();

    QJsonObject animObjS = jsonRootBlend.value("animation").toObject();
    QJsonObject animBuffS = animObjS.value("animBuffer").toObject();
    QJsonArray animBonesS = animBuffS.value("bones").toArray();
    QMap<QString, QJsonObject> boneByNameS;
    double t = 0;
    double maxT = ui->spinBlendKoef->value();

    for (int i = 0; i < animBonesS.size(); i += 1) {
        QString boneNameS = animBonesS[i].toObject().value("BoneName").toString();
        boneByNameS[boneNameS] = animBonesS[i].toObject();
    }

    for (int i = 0; i < animBonesT.size(); i += 1) {
        QString boneNameT = animBonesT[i].toObject().value("BoneName").toString();
        if ( !boneByNameS.contains(boneNameT) ) {
            addLog(QString("Can't find bone %1 in source anim!").arg(boneNameT), logError);
            continue;
        }

        QJsonObject boneT = animBonesT[i].toObject();
        int posNumT = boneT.value("positionFrames").toInt();
        int rotNumT = boneT.value("rotationFrames").toInt();
        if (posNumT < framesCount) {
            QJsonArray posArr = boneT.value("positionFrames").toArray();
            while (posNumT < framesCount) {
                posArr.append( posArr.last() );
                ++posNumT;
            }
            boneT["positionFrames"] = posArr;
        }
        if (rotNumT < framesCount) {
            QJsonArray rotArr = boneT.value("rotationFrames").toArray();
            while (rotNumT < framesCount) {
                rotArr.append( rotArr.last() );
                ++rotNumT;
            }
            boneT["rotationFrames"] = rotArr;
        }
        animBonesT[i] = boneT;

        QJsonObject boneS = boneByNameS[boneNameT];
        int posNumS = boneS.value("positionFrames").toInt();
        int rotNumS = boneS.value("rotationFrames").toInt();
        if (posNumS < blendSourceFramesCount) {
            QJsonArray posArr = boneS.value("positionFrames").toArray();
            while (posNumS < blendSourceFramesCount) {
                posArr.append( posArr.last() );
                ++posNumS;
            }
            boneS["positionFrames"] = posArr;
        }
        if (rotNumS < blendSourceFramesCount) {
            QJsonArray rotArr = boneS.value("rotationFrames").toArray();
            while (rotNumS < blendSourceFramesCount) {
                rotArr.append( rotArr.last() );
                ++rotNumS;
            }
            boneS["rotationFrames"] = rotArr;
        }
        boneByNameS[boneNameT] = boneS;
    }

    while (done < total) {
        if (total == 1)
            t = maxT;
        else
            t = maxT * done / (total - 1); // (0; 1]

        addLog(QString("Process: done %1 of %2, t = %3").arg(done).arg(total).arg(t));
        for (int i = 0; i < animBonesT.size(); i += 1) {
            QString boneNameT = animBonesT[i].toObject().value("BoneName").toString();

            QJsonObject boneT = animBonesT[i].toObject();
            QJsonObject boneS = boneByNameS[boneNameT];

            QJsonArray posArr = boneT.value("positionFrames").toArray();
            QJsonArray rotArr = boneT.value("rotationFrames").toArray();
            double pX1, pY1, pZ1;
            double pX2, pY2, pZ2;
            objToXYZ(posArr.at(targetFrame).toObject(), pX1, pY1, pZ1);
            objToXYZ(boneS.value("positionFrames").toArray().at(sourceFrame).toObject(), pX2, pY2, pZ2);
            //qDebug() << QString("X1: %1, X2: %2, Y1: %3, Y2: %4, Z1: %5, Z2: %6").arg(pX1).arg(pX2).arg(pY1).arg(pY2).arg(pZ1).arg(pZ2);
            interpolatePos(t, pX1, pY1, pZ1, pX2, pY2, pZ2);
            posArr[targetFrame] = objXYZ(pX1, pY1, pZ1);
            boneT["positionFrames"] = posArr;

            double rX1, rY1, rZ1, rW1;
            double rX2, rY2, rZ2, rW2;
            objToXYZW(rotArr.at(targetFrame).toObject(), rX1, rY1, rZ1, rW1);
            objToXYZW(boneS.value("rotationFrames").toArray().at(sourceFrame).toObject(), rX2, rY2, rZ2, rW2);
            interpolateRot(t, rX1, rY1, rZ1, rW1, rX2, rY2, rZ2, rW2);
            rotArr[targetFrame] = objXYZW(rX1, rY1, rZ1, rW1);
            boneT["rotationFrames"] = rotArr;

            animBonesT[i] = boneT;
        }

        ++done;
        ++sourceFrame;
        ++targetFrame;
    }
    animBuffT["bones"] = animBonesT;
    animObjT["animBuffer"] = animBuffT;
    jsonRoot["animation"] = animObjT;
    addLog("Done!");
}

W3MayaAnimUtil::~W3MayaAnimUtil()
{
    delete ui;
}

