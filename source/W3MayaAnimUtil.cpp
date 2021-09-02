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

    connect(ui->buttonLoad, SIGNAL(clicked(bool)), this, SLOT(onClicked_Load()));
    connect(ui->buttonSave, SIGNAL(clicked(bool)), this, SLOT(onClicked_Save()));
    connect(ui->buttonApplyMotionToBone, SIGNAL(clicked(bool)), this, SLOT(onClicked_applyMotionToBone()));
    connect(ui->buttonExtractMotionFromBone, SIGNAL(clicked(bool)), this, SLOT(onClicked_extractMotionFromBone()));
    connect(ui->buttonExtractMotionFromBoneBatch, SIGNAL(clicked(bool)), this, SLOT(onClicked_extractMotionFromBoneBatch()));
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
        int bonesCount = animObj.value("animBuffer").toObject().value("bones").toArray().count();
        if ( duration < 0.03 ) {
            addLog( QString("Detected single anim with incorrect duration: %1 s").arg(duration), logError );
            return false;
        }
        if ( bonesCount < 1 ) {
            addLog( QString("Detected single anim with incorrect bones amount (%1 bones)").arg(bonesCount), logError );
            return false;
        }

        addLog( QString("[OK] Detected single anim (probably, exported from Maya)") );
        addLog( QString("[Info] name: %1, duration: %2 s, bones amount: %3").arg(name).arg(duration).arg(bonesCount) );
        return true;
    } else {
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
            if (ui->checkAddInverted->isChecked()) {
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
            }
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
    QJsonObject bufferObj = animObj["animBuffer"].toObject();
    QJsonArray bonesArray = bufferObj["bones"].toArray();
    if ( bonesArray.isEmpty() ) {
        addLog("Empty bones/anim buffer, skipping.", logError);
        return false;
    }

    int animFrames = bufferObj["numFrames"].toInt();
    int mBoneIdx = -1;
    upn(i, 0, bonesArray.size() - 1) {
        if (bonesArray[i].toObject().value("BoneName").toString() == mBoneName) {
            mBoneIdx = i;
            break;
        }
    }
    if (mBoneIdx == -1) {
        if (ui->checkIgnoreEmptyRootMotion->isChecked()) {
            addLog(QString("    Anim doesn't contain [%2] bone, ignoring.").arg(mBoneName), logError );
            return false;
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
            double tmp_totalDelta = qAbs(tmp_deltaX - deltaX) + qAbs(tmp_deltaY - deltaY) + qAbs(tmp_deltaZ - deltaZ) + qAbs(tmp_deltaRotZ - deltaRotZ);
            //qDebug() << QString("[%1] total delta: %2,   X: %3, Y: %4, Z: %5, Rot: %6").arg(frame).arg(tmp_totalDelta).arg(tmp_deltaX).arg(tmp_deltaY).arg(tmp_deltaZ).arg(tmp_deltaRotZ);

            if (tmp_totalDelta < mReductionSensitivity && frame - lastDelta < 255) {
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
            if (qAbs(motionX[frame + 1] - motionX[frame]) > mReductionSensitivity)
                isSingleX = false;
            if (qAbs(motionY[frame + 1] - motionY[frame]) > mReductionSensitivity)
                isSingleY = false;
            if (qAbs(motionZ[frame + 1] - motionZ[frame]) > mReductionSensitivity)
                isSingleZ = false;
            if (qAbs(motionRotZ[frame + 1] - motionRotZ[frame]) > mReductionSensitivity)
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
    ui->progressBar->setValue(0);

    if (animSet) {
        QJsonArray animArray = jsonRoot["animations"].toArray();
        upn(i, 0, animArray.size() - 1) {
            QJsonObject animObj = animArray[i].toObject();
            if ( extractMotionFromBone(animObj["animation"]) ) {
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
        if ( extractMotionFromBone(jsonRoot["animation"]) ) {
            hasChanges = true;
        }
        addLog(QString("[Finished in %1s] Processed 1 animation.<br>").arg(eTimer.elapsed() / 1000.0));
        ui->progressBar->setValue(100);
    }
}
void W3MayaAnimUtil::onClicked_extractMotionFromBoneBatch() {
    QString dirName = QFileDialog::getExistingDirectory(this, tr("Choose directory"),
                                                        QDir::currentPath());
    if (dirName.isEmpty()) {
        addLog("User canceled operation.", logWarning);
        return;
    }
    QStringList jsonList = QDir(dirName).entryList({"*.json"}, QDir::Files, QDir::Name);
    for (const QString jsonPath : jsonList) {
        addLog("[BATCH] Processing file: " + jsonPath);
        if (loadJsonFile(dirName + "/" + jsonPath)) {
            onClicked_extractMotionFromBone();
            // YES it's dirty crutch
            QApplication::processEvents();
            if (hasChanges) {
                onClicked_Save();
            }
        }
        // YES it's dirty crutch
        QApplication::processEvents();
    }
    addLog(QString("[BATCH] Completed processing %1 files.").arg(jsonList.size()));
}

W3MayaAnimUtil::~W3MayaAnimUtil()
{
    delete ui;
}

