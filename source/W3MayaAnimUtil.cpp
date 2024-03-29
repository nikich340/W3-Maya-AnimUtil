#include "W3MayaAnimUtil.h"
#include "ui_W3MayaAnimUtil.h"
#include <QScreen>
#include <QScrollBar>
#include <QMessageBox>
#include <QFileInfo>
#include <QFileDialog>
#include <math.h>

#define upn(val, start, end) for(int val = start; val <= end; ++val)
#define JRef QJsonValueRef
#define VERSION "v2.3"
#define MAU W3MayaAnimUtil

MAU::MAU(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MAU)
{
    ui->setupUi(this);
    //ui->textLog->setFontPointSize(20);
    ui->textLog->setHtml("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">"
                         "<html><head><meta name=\"qrichtext\" content=\"1\" /><meta charset=\"utf-8\" /><style type=\"text/css\">"
                         "p, li { white-space: pre-wrap; }"
                         "</style></head><body style=\" font-family:'Segoe UI'; font-size:9pt; font-weight:400; font-style:normal;\">"
                         "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Welcome to <span style=\" font-weight:700;\">W3MayaAnimUtil "
                         + QString(VERSION " (" __DATE__ ")")
                         + "</span>.<br />Made by <span style=\" font-weight:696; color:#6f00a6;\">nikich340</span> for better the Witcher 3 modding experiene.<br /><span style=\" font-style:italic;\"><br /></span>Click &quot;Load anim .json&quot; to start.</p>"
                         "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><img src=\":/icons/motivated_small.png\" /></p></body></html>");
    ui->spinSensivity->setMinimum(0.0000000001);
    ui->spinSensivity->setSingleStep(0.00001);
    ui->spinSensivity->setValue(0.00001);

    logFile.setFileName("W3AnimUtil_log.txt");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        logStream.setDevice(&logFile);
    }
    pLabelInfo = new QLabel();

    if (QSettings().value("GUIStyle", "windowsvista") == "windowsvista") {
        ui->comboGUIStyle->setCurrentText("Windows");
    } else if (QSettings().value("GUIStyle", "windowsvista") == "Windows") {
        ui->comboGUIStyle->setCurrentText("WindowsXP");
    } else {
        ui->comboGUIStyle->setCurrentText("Fusion");
    }

    connect(ui->comboGUIStyle, SIGNAL(currentTextChanged(QString)), this, SLOT(onChanged_GUIStyle(QString)));
    connect(ui->buttonLoad, SIGNAL(clicked(bool)), this, SLOT(onClicked_Load()));
    connect(ui->buttonSave, SIGNAL(clicked(bool)), this, SLOT(onClicked_Save()));
    connect(ui->buttonSaveSplit, SIGNAL(clicked(bool)), this, SLOT(onClicked_SaveSplit()));
    connect(ui->buttonApplyMotionToBone, SIGNAL(clicked(bool)), this, SLOT(onClicked_applyMotionToBone()));
    connect(ui->buttonExtractMotionFromBone, SIGNAL(clicked(bool)), this, SLOT(onClicked_extractMotionFromBone()));
    connect(ui->buttonExtractMotionFromBoneBatch, SIGNAL(clicked(bool)), this, SLOT(onClicked_extractMotionFromBoneBatch()));
    connect(ui->buttonLoadEventsSource, SIGNAL(clicked(bool)), this, SLOT(onClicked_loadAnimEventsSource()));
    connect(ui->buttonLoadMotionSource, SIGNAL(clicked(bool)), this, SLOT(onClicked_loadMotionSource()));
    connect(ui->buttonLoadTxtDump, SIGNAL(clicked(bool)), this, SLOT(onClicked_loadTxtDump()));

    connect(ui->buttonPrintInfo, SIGNAL(clicked(bool)), this, SLOT(onClicked_PrintInfo()));

    connect(ui->buttonExtractCutscene, SIGNAL(clicked(bool)), this, SLOT(onClicked_extractPartsCutscene()));
    connect(ui->buttonPatchCutscene, SIGNAL(clicked(bool)), this, SLOT(onClicked_patchPartsCutscene()));

    /* GENERAL EDIT */
    connect(ui->comboEditAnims, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged_EditCurrentAnim(int)));
    connect(ui->spinEditStart, SIGNAL(valueChanged(int)), this, SLOT(onChanged_EditStart(int)));
    connect(ui->spinEditDuration, SIGNAL(valueChanged(int)), this, SLOT(onChanged_EditDuration(int)));
    connect(ui->spinEditEnd, SIGNAL(valueChanged(int)), this, SLOT(onChanged_EditEnd(int)));
    connect(ui->buttonEditApply, SIGNAL(clicked(bool)), this, SLOT(onClicked_EditApply()));
    connect(ui->buttonEditApplyAll, SIGNAL(clicked(bool)), this, SLOT(onClicked_EditApplyAll()));
    connect(ui->groupEditCut, SIGNAL(clicked(bool)), this, SLOT(onChecked_EditCutScale(bool)));
    connect(ui->groupEditScale, SIGNAL(clicked(bool)), this, SLOT(onChecked_EditCutScale(bool)));

    /* EVENTS EDIT */
    ui->comboEventsType->addItems(m_knownEventTypes);
    ui->comboEventsVarType->addItems(m_knownVarTypes);
    ui->comboEventsVarType->addItems(m_knownEnumTypes.keys());
    ui->comboEventsAttackHitReaction->addItems(m_knownEnumTypes["EHitReactionType"]);
    ui->comboEventsAttackSwingDir->addItems(m_knownEnumTypes["EAttackSwingDirection"]);
    ui->comboEventsAttackSwingType->addItems(m_knownEnumTypes["EAttackSwingType"]);
    ui->stackEventsValue->widget(0)->setProperty("type", QString("Bool"));
    ui->stackEventsValue->widget(1)->setProperty("type", QString("Int32"));
    ui->stackEventsValue->widget(2)->setProperty("type", QString("Float"));
    ui->stackEventsValue->widget(3)->setProperty("type", QString("String"));
    ui->stackEventsValue->widget(4)->setProperty("type", QString("array:2,0,StringAnsi"));
    ui->stackEventsValue->widget(5)->setProperty("type", QString("SEnumVariant"));
    ui->stackEventsValue->widget(6)->setProperty("type", QString("CPreAttackEventData"));

    addEventsVarControl(ui->checkEventsVarEnumType, ui->lineEventsEnumType);
    addEventsVarControl(ui->checkEventsVarEnumValue, ui->spinEventsEnumValue);
    addEventsVarControl(ui->checkEventsVarAttackName, ui->lineEventsAttackName);
    addEventsVarControl(ui->checkEventsVarAttackWeaponSlot, ui->lineEventsAttackWeaponSlot);
    addEventsVarControl(ui->checkEventsVarAttackSwingType, ui->comboEventsAttackSwingType);
    addEventsVarControl(ui->checkEventsVarAttackSwingDir, ui->comboEventsAttackSwingDir);
    addEventsVarControl(ui->checkEventsVarAttackSound, ui->lineEventsAttackSound);
    addEventsVarControl(ui->checkEventsVarAttackRange, ui->lineEventsAttackRange);
    addEventsVarControl(ui->checkEventsVarAttackHitReaction, ui->comboEventsAttackHitReaction);
    addEventsVarControl(ui->checkEventsVarAttackCanParry, ui->checkEventsAttackCanParry);
    addEventsVarControl(ui->checkEventsVarAttackCanBeDodged, ui->checkEventsAttackCanBeDodged);
    addEventsVarControl(ui->checkEventsVarAttackDamageNeutral, ui->checkEventsAttackDamageNeutral);
    addEventsVarControl(ui->checkEventsVarAttackDamageFriendly, ui->checkEventsAttackDamageFriendly);
    addEventsVarControl(ui->checkEventsVarAttackHitFx, ui->lineEventsAttackHitFx);
    addEventsVarControl(ui->checkEventsVarAttackHitParriedFx, ui->lineEventsAttackHitParriedFx);
    addEventsVarControl(ui->checkEventsVarAttackHitBackFx, ui->lineEventsAttackHitBackFx);
    addEventsVarControl(ui->checkEventsVarAttackHitBackParriedFx, ui->lineEventsAttackHitBackParriedFx);

    connect(ui->listEvents, SIGNAL(currentRowChanged(int)), this, SLOT(onChanged_eventRow(int)));
    connect(ui->listEventsContent, SIGNAL(currentRowChanged(int)), this, SLOT(onChanged_eventContentRow(int)));
    connect(ui->buttonResetEvents, SIGNAL(clicked(bool)), this, SLOT(onClicked_eventsReset()));
    connect(ui->buttonApplyEvents, SIGNAL(clicked(bool)), this, SLOT(onClicked_eventsApply()));

    connect(ui->buttonEventsSort, SIGNAL(clicked(bool)), this, SLOT(onClicked_eventsSort()));
    connect(ui->buttonEventsFixAnimationName, SIGNAL(clicked(bool)), this, SLOT(onClicked_fixAnimationNames()));
    connect(ui->buttonEventsType, SIGNAL(clicked(bool)), this, SLOT(onClicked_eventsSetType()));
    connect(ui->buttonEventsAdd, SIGNAL(clicked(bool)), this, SLOT(onClicked_eventsAdd()));
    connect(ui->buttonEventsClone, SIGNAL(clicked(bool)), this, SLOT(onClicked_eventsClone()));
    connect(ui->buttonEventsRemove, SIGNAL(clicked(bool)), this, SLOT(onClicked_eventsRemove()));

    connect(ui->buttonEventsVarRename, SIGNAL(clicked(bool)), this, SLOT(onClicked_eventsVarRename()));
    connect(ui->buttonEventsVarType, SIGNAL(clicked(bool)), this, SLOT(onClicked_eventsVarSetType()));
    connect(ui->buttonEventsVarAdd, SIGNAL(clicked(bool)), this, SLOT(onClicked_eventsVarAdd()));
    connect(ui->buttonEventsVarRemove, SIGNAL(clicked(bool)), this, SLOT(onClicked_eventsVarRemove()));
    // all value changes are set via form -> onChanged_eventsVarAny()

    /* MERGE */
    connect(ui->buttonMergeLoad, SIGNAL(clicked(bool)), this, SLOT(onClicked_LoadMergeJson()));
    connect(ui->buttonMergeHelp, SIGNAL(clicked(bool)), this, SLOT(onClicked_MergePictureHelp()));
    connect(ui->buttonMergeDo, SIGNAL(clicked(bool)), this, SLOT(onClicked_MergeProcess()));
    connect(ui->spinMergeStart, SIGNAL(valueChanged(int)), this, SLOT(onChanged_MergeStart()));
    connect(ui->spinMergeDuration, SIGNAL(valueChanged(int)), this, SLOT(onChanged_MergeDuration()));
    connect(ui->comboMergeType, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged_MergeType(int)));
    connect(ui->buttonMergeLastHalfSec, SIGNAL(clicked(bool)), this, SLOT(onClicked_BlendHalfSec()));

    resize(QGuiApplication::primaryScreen()->geometry().width() * 0.75, QGuiApplication::primaryScreen()->geometry().height() * 0.8);
}

double MAU::mReductionSensitivity() {
    return ui->spinSensivity->value();
}
void MAU::addLog(QString text, LogType type) {
    qDebug() << text;
    QColor color;
    QString stype;
    if (type == logFinish) {
        color.setRgb(20, 90, 50);
        stype = " [INFO] ";
    } else if (type == logWarning) {
        color.setRgb(188, 94, 0);
        stype = " [WARNING] ";
    } else if (type == logProcess) {
        color.setRgb(0, 15, 155);
        stype = " [PROCESS] ";
    } else {
        color.setRgb(188, 0, 0);
        stype = " [ERROR] ";
    }

    if (logStream.device() != nullptr) {
        logStream << QTime::currentTime().toString() << stype << text << "\n";
        logStream.flush();
    }

    QString oldText = ui->textLog->toHtml();
    if (oldText.length() > 25000) {
        oldText = "";
    }
    text = QString("<font color=\"%1\">%2</font>").arg(color.name()).arg(text);

    ui->textLog->setHtml( oldText + text );
    ui->textLog->verticalScrollBar()->setValue( ui->textLog->verticalScrollBar()->maximum() );
}

bool MAU::loadJsonFile(QString filePath) {
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

    if ( !onlyPrint ) {
        if ( !QFile::exists(filePath + ".bak") && ui->checkBak->isChecked() ) {
            jsonFile.copy(filePath + ".bak");
        } else {
            //QFile::remove(filePath + ".bak");
            //jsonFile.copy(filePath + ".bak");
        }
    }
    jsonFile.close();

    jsonRoot = jsonDoc.object();
    if ( loadW3Data() ) {
        addLog( QString("[OK] Loaded %1, original file saved as: %1.bak").arg(QFileInfo(filePath).fileName()), logFinish );
        ui->linePath->setText(filePath);
        ui->buttonSave->setEnabled(true);
        ui->buttonSaveSplit->setEnabled(true);
        return true;
    } else {
        addLog( QString("Failed to detect data format: %1").arg(QFileInfo(filePath).fileName()), logError );
        ui->linePath->setText("NO CORRECT .JSON LOADED!");
        ui->buttonSave->setEnabled(false);
        ui->buttonSaveSplit->setEnabled(false);
        return false;
    }
}

double MAU::getEventStartTime(QJsonObject eventObj) {
    QJsonArray contentArr = eventObj.value("Content").toArray();
    upn(j, 0, contentArr.count() - 1) {
        QJsonObject contentEntry = contentArr.at(j).toObject();
        if (contentEntry.value("Name") == "startTime") {
            return contentEntry.value("val").toDouble();
        }
    }
    //qDebug() << QString("Can't find event startTime! Type: %1").arg(eventObj.value("Type").toString());
    return 0.0;
}
void MAU::setEventStartTime(QJsonObject& eventObj, double newTime) {
    QJsonArray contentArr = eventObj.value("Content").toArray();

    upn(j, 0, contentArr.count() - 1) {
        QJsonObject contentEntry = contentArr.at(j).toObject();
        if (contentEntry.value("Name") == "startTime") {
            contentEntry["val"] = newTime;
            contentArr[j] = contentEntry;
            break;
        }
    }
    eventObj["Content"] = contentArr;
}
void MAU::setEventDuration(QJsonObject& eventObj, double newDuration) {
    QJsonArray contentArr = eventObj.value("Content").toArray();

    upn(j, 0, contentArr.count() - 1) {
        QJsonObject contentEntry = contentArr.at(j).toObject();
        if (contentEntry.value("Name") == "duration") {
            contentEntry["val"] = newDuration;
            contentArr[j] = contentEntry;
            break;
        }
    }
    eventObj["Content"] = contentArr;
}
void MAU::onChanged_GUIStyle(QString newStyle) {
    if (newStyle == "Default") {
        QSettings().remove("GUIStyle");
    } else if (newStyle == "WindowsXP") {
        QSettings().setValue("GUIStyle", "Windows");
    } else if (newStyle == "Fusion") {
        QSettings().setValue("GUIStyle", "Fusion");
    }
    addLog(QString("[OK] Set new GUI Style: %1. Please restart application to apply it.").arg(newStyle), logWarning);
}
void MAU::onClicked_Load() {
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
void MAU::setCurrentAnimInfo(int bones, int events, int frames, double duration, bool rootMotion, bool motionExtraction) {
    if (frames == -1 && duration < 0) {
        ui->labelEditInfo->setText("<html><head/><body><p><span style=\"font-weight:700;\">Bones</span>: -. <span style=\"font-weight:700;\">Events</span>: -. <span style=\"font-weight:700;\">Duration</span>: - frames (- s), <span style=\"color:#ff0000;\">RootMotion</span>, <span style=\"color:#ff0000;\">motionExtraction</span><span style=\"color:#000000;\">. </span><span style=\"font-weight:700; color:#000000;\">Anim name</span><span style=\"color:#000000;\">: </span></p></body></html>");
        ui->spinEditStart->setValue(-1);
        ui->spinEditDuration->setValue(-1);
        ui->spinEditEnd->setValue(-1);
        ui->lineEditName->setText(" - ");
        //ui->labelEditStart->setText("Start (- s) frame:");
        //ui->labelEditDuration->setText("Duration (- s) frames:");
        ui->buttonEditApply->setEnabled(false);
        ui->buttonEditApplyAll->setEnabled(false);
    } else {
        ui->labelEditInfo->setText(QString("<html><head/><body><p><span style=\"font-weight:700;\">Bones</span>: %1. <span style=\"font-weight:700;\">Events</span>: %2. <span style=\"font-weight:700;\">Duration</span>: %3 frames (%4 s), <span style=\"color:%5;\">RootMotion</span>, <span style=\"color:%6;\">motionExtraction</span><span style=\"color:#000000;\">. </span><span style=\"font-weight:700; color:#000000;\">Anim name</span><span style=\"color:#000000;\">: </span></p></body></html>")
                                   .arg(bones)
                                   .arg(events)
                                   .arg(frames)
                                   .arg(duration, 0, 'f', 3)
                                   .arg(rootMotion ? "#2a7e3e" : "#ff0000")
                                   .arg(motionExtraction ? "#2a7e3e" : "#ff0000"));
        ui->lineEditName->setText(m_animNames[m_animIndex]);
        ui->spinEditStart->setValue(1);
        onChanged_EditStart(1); // hack to update label
        ui->spinEditDuration->setValue(frames);
        ui->spinEditEnd->setValue(frames);
        ui->buttonEditApply->setEnabled(true);
        ui->buttonEditApplyAll->setEnabled(true);
    }
}
void MAU::applyEdits() {
    QJsonArray animsObj = QJsonArray();
    QJsonObject animRootObj = QJsonObject();
    QJsonObject animObj = QJsonObject();
    QJsonArray eventsArray = QJsonArray();
    // load
    if (animSet) {
        animsObj = jsonRoot.value("animations").toArray();
        animRootObj = animsObj.at(m_animIndex).toObject();
        animObj = animRootObj.value("animation").toObject();
        eventsArray = animRootObj.value("entries").toArray();
    } else {
        animObj = jsonRoot.value("animation").toObject();
        eventsArray = jsonRoot.value("entries").toArray();
    }

    // edit
    addLog(QString("[EDIT] Anim: %1.").arg(ui->comboEditAnims->currentText()));
    QString animName = ui->lineEditName->text();
    if (animName != ui->comboEditAnims->currentText()) {
        ui->comboEditAnims->setItemText(m_animIndex, animName);
        m_animNames[m_animIndex] = animName;
        hasChanges = true;
    }
    if (ui->checkEditAddRootMotion->isChecked()) {
        QJsonObject animBuff = animObj.value("animBuffer").toObject();
        QJsonArray animBones = animBuff.value("bones").toArray();
        editAddEmptyBoneFrames(animBones, "RootMotion");
        animBuff["bones"] = animBones;
        animObj["animBuffer"] = animBuff;
        hasChanges = true;
    }
    if (ui->groupEditBake->isChecked()) {
        editBakeBones(animObj, ui->checkEditBakePos->isChecked(), ui->checkEditBakeRot->isChecked(), ui->checkEditBakeScale->isChecked());
        hasChanges = true;
    }
    if (ui->groupEditCut->isChecked()) {
        editCropAnim(animObj, eventsArray, ui->checkEditCutEvents->isChecked(),
                     ui->spinEditStart->value(), ui->spinEditDuration->value());
        hasChanges = true;
    }
    if (ui->groupEditScale->isChecked()) {
        editScaleAnim(animObj, eventsArray, ui->checkEditScaleEvents->isChecked(),
                      ui->spinEditScale->value());
        hasChanges = true;
    }
    if (ui->editForceDuration->isChecked()) {
        editSetCDPRDuration(animObj);
        hasChanges = true;
    }

    if (ui->groupEditOptimize->isChecked()) {
        int boneOptimized = editOptimizeBones(animObj, ui->checkEditOptimizePos->isChecked(), ui->checkEditOptimizeRot->isChecked(), ui->checkEditOptimizeScale->isChecked());
        hasChanges = hasChanges || (boneOptimized > 0);
    }

    if (ui->checkEditSortEvents->isChecked()) {
        editSortEvents(eventsArray);
        hasChanges = true;
    }

    // save
    if (animSet) {
        animRootObj["animation"] = animObj;
        animRootObj["entries"] = eventsArray;
        animsObj[m_animIndex] = animRootObj;
        jsonRoot["animations"] = animsObj;
    } else {
        jsonRoot["animation"] = animObj;
        jsonRoot["entries"] = eventsArray;
    }
    addLog(QString("[OK] All edits were applied for anim: %1.").arg(animName), logFinish);
    onChanged_EditCurrentAnim(m_animIndex);
}

void MAU::editRenameAnim(QJsonObject& animObj, QJsonArray& eventsArray, QString newName) {
    addLog(QString("\t[EDIT] Rename anim to: %1.")
           .arg(newName));
    animObj["name"] = newName;

    upn(i, 0, eventsArray.count() - 1) {
        QJsonObject eventObj = eventsArray.at(i).toObject();
        QJsonArray contentArr = eventObj.value("Content").toArray();
        upn(j, 0, contentArr.count() - 1) {
            QJsonObject contentEntry = contentArr.at(j).toObject();
            if (contentEntry.value("Name") == "animationName") {
                contentEntry["Value"] = newName;
                contentArr[j] = contentEntry;
                break;
            }
        }
        eventObj["Content"] = contentArr;
        eventsArray[i] = eventObj;
    }
}

void MAU::editSetCDPRDuration(QJsonObject& animObj) {
    QJsonObject animBuff = animObj.value("animBuffer").toObject();
    int animFrames = animBuff.value("numFrames").toInt();
    double durationCDPR = framesToSec(animFrames - 1);
    addLog(QString("\t[EDIT] Set anim and animBuffer duration = %1 s.")
           .arg( durationCDPR ));

    animBuff["duration"] = durationCDPR;
    animObj["animBuffer"] = animBuff;
    animObj["duration"] = durationCDPR;
    if ( animObj.contains("motionExtraction") ) {
        JSO motionObj = animObj["motionExtraction"].toObject();
        motionObj["duration"] = qMin(motionObj["duration"].toDouble(), durationCDPR);
        animObj["motionExtraction"] = motionObj;
    }
    m_animDurations[m_animIndex] = framesToSec(animFrames - 1);
}

void MAU::editSortEvents(QJsonArray& eventsArray) {
    addLog(QString("\t[EDIT] Sort events by startTime: %1 entries.")
           .arg(eventsArray.count()));
    QJsonArray newEventsArray = QJsonArray();
    QMultiMap<double, QJsonObject> sortedEvents;

    upn(i, 0, eventsArray.count() - 1) {
        QJsonObject eventObj = eventsArray.at(i).toObject();
        //qDebug() << QString("editSortEvents[%1].count = %2").arg(i).arg(eventObj.count());
        double eventStartTime = getEventStartTime(eventObj);
        sortedEvents.insert(eventStartTime, eventObj);
    }
    QMapIterator<double, QJsonObject> it(sortedEvents);
    while (it.hasNext()) {
        it.next();
        qDebug() << it.key() << ": " << it.value().count();
        newEventsArray.append( it.value() );
    }

    eventsArray = newEventsArray;
}

void MAU::editScaleAnim(QJsonObject &animObj, QJsonArray &eventsArray, bool scaleEvents, double speedFactor) {
    QJsonObject animBuff = animObj.value("animBuffer").toObject();
    int animFrames = animBuff.value("numFrames").toInt();
    //speedFactor += 1.0 / animFrames; // to add 1 frame for correct result
    int newFrames = qMax(2, int(animFrames * speedFactor) - 1);
    double frameLen = 1.0 / speedFactor;
    double newDuration = framesToSec(newFrames - 1);
    QJsonArray animBones = animBuff.value("bones").toArray();

    addLog(QString("\t[EDIT] Scale anim frames: [%1 -> %2].")
           .arg(animFrames)
           .arg(newFrames));

    for (int i = 0; i < animBones.count(); i += 1) {
        QString boneName = animBones[i].toObject().value("BoneName").toString();
        QJsonObject boneObj = animBones[i].toObject();

        QString attrNames[3] = { "position", "rotation", "scale" };
        upn(j, 0, 2) {
            int attrFrames = boneObj.value(QString("%1_numFrames").arg(attrNames[j])).toInt();
            QJsonArray attrArray = boneObj.value(QString("%1Frames").arg(attrNames[j])).toArray();

            if (attrFrames == 1) {
                // do nothing
            } else if (attrFrames == animFrames) {
                QJsonArray newArray = QJsonArray();


                double framePos = 0.0;
                newArray.append( attrArray.first() );
                // frame = 1 == first(), skipped
                upn(frame, 2, newFrames - 1) {
                    int f1 = int(framePos);
                    int f2 = f1 + 1;
                    double k = framePos - floor(framePos);
                    qDebug() << QString("Bone %1, %5 frame %2, framePos %3, k %4.").arg(boneName).arg(frame).arg(framePos).arg(k).arg(attrNames[j]);
                    newArray.append( interpolateObj(k, attrArray[f1].toObject(), attrArray[f2].toObject()) );
                    framePos += frameLen;
                }

                newArray.append( attrArray.last() );

                boneObj[ QString("%1_numFrames").arg(attrNames[j]) ] = newFrames;
                boneObj[ QString("%1Frames").arg(attrNames[j]) ] = newArray;
            } else {
                addLog(QString("\t[EDIT][SCALE] Bone: %1, 1 < %2_Frames = %3 < animFrames = %4. Cropping skipped.")
                       .arg(boneName).arg(attrNames[j]).arg(attrFrames).arg(animFrames), logError);
                return;
            }
        }
        animBones[i] = boneObj;
    }

    if ( scaleEvents && !eventsArray.isEmpty() ) {
        QJsonArray newEventsArray = QJsonArray();

        upn(i, 0, eventsArray.count() - 1) {
            QJsonObject eventObj = eventsArray.at(i).toObject();
            double startTime = getEventParam(eventObj, "startTime", -1.0).toDouble();
            double duration = getEventParam(eventObj, "duration", -1.0).toDouble();
            if (startTime >= 0.0) {
                setEventStartTime(eventObj, startTime * (speedFactor - 1.0 / (double)animFrames));
            }
            if (duration >= 0.0) {
                setEventDuration(eventObj, duration * (speedFactor - 1.0 / (double)animFrames));
            }
            newEventsArray.append( eventObj );
        }
        addLog( QString("\t[EDIT] Scale %1 anim events.")
                .arg(newEventsArray.count()) );
        eventsArray = newEventsArray;
    }

    animBuff["bones"] = animBones;
    animBuff["numFrames"] = newFrames;
    animBuff["duration"] = newDuration;
    animObj["animBuffer"] = animBuff;
    animObj["duration"] = newDuration;
    m_animFrames[m_animIndex] = newFrames;
    m_animDurations[m_animIndex] = newDuration;
    addLog(QString("\t[EDIT] Set anim and animBuffer duration = %1 s.")
           .arg( framesToSec(newFrames - 1) ));
}

void MAU::editCropAnim(QJsonObject& animObj, QJsonArray& eventsArray, bool cropEvents, int startFrame, int durationFrames) {
    addLog(QString("\t[EDIT] Crop anim frames to: [%1 - %2].")
           .arg(startFrame)
           .arg(durationFrames));

    QJsonObject animBuff = animObj.value("animBuffer").toObject();
    int animFrames = animBuff.value("numFrames").toInt();
    QJsonArray animBones = animBuff.value("bones").toArray();

    for (int i = 0; i < animBones.count(); i += 1) {
        QString boneName = animBones[i].toObject().value("BoneName").toString();
        QJsonObject boneObj = animBones[i].toObject();

        QString attrNames[3] = { "position", "rotation", "scale" };
        upn(j, 0, 2) {
            int attrFrames = boneObj.value(QString("%1_numFrames").arg(attrNames[j])).toInt();
            QJsonArray attrArray = boneObj.value(QString("%1Frames").arg(attrNames[j])).toArray();

            if (attrFrames == 1) {
                // do nothing
            } else if (attrFrames == animFrames) {
                QJsonArray newArray = QJsonArray();
                upn(jj, startFrame - 1, startFrame - 1 + durationFrames - 1) {
                    newArray.append(attrArray[jj]);
                }
                boneObj[ QString("%1_numFrames").arg(attrNames[j]) ] = durationFrames;
                boneObj[ QString("%1Frames").arg(attrNames[j]) ] = newArray;
            } else {
                addLog(QString("\t[EDIT][CROP] Bone: %1, 1 < %2Frames = %3 < animFrames = %4. Cropping skipped.")
                       .arg(boneName).arg(attrNames[j]).arg(attrFrames).arg(animFrames), logError);
                return;
            }
        }
        animBones[i] = boneObj;
    }

    if ( cropEvents && !eventsArray.isEmpty() ) {
        QJsonArray newEventsArray = QJsonArray();
        double cropStartTime = framesToSec(startFrame - 1);
        double cropEndTime = framesToSec(startFrame - 1 + durationFrames);

        upn(i, 0, eventsArray.count() - 1) {
            QJsonObject eventObj = eventsArray.at(i).toObject();
            double eventStartTime = getEventStartTime(eventObj);
            if (eventStartTime >= cropStartTime && eventStartTime <= cropEndTime) {
                setEventStartTime(eventObj, eventStartTime - cropStartTime);
                newEventsArray.append( eventObj );
            }
        }
        addLog( QString("\t[EDIT] Crop anim events: %1 before, %2 after.")
                .arg(eventsArray.count()).arg(newEventsArray.count()) );
        eventsArray = newEventsArray;
    }

    animBuff["bones"] = animBones;
    animBuff["numFrames"] = durationFrames;
    animBuff["duration"] = framesToSec(durationFrames - 1);
    animObj["animBuffer"] = animBuff;
    animObj["duration"] = framesToSec(durationFrames - 1);
    m_animFrames[m_animIndex] = durationFrames;
    m_animDurations[m_animIndex] = framesToSec(durationFrames - 1);
    addLog(QString("\t[EDIT] Set anim and animBuffer duration = %1 s.")
           .arg( framesToSec(durationFrames - 1) ));
}

void MAU::editAddEmptyBoneFrames(QJsonArray& bonesArr, QString boneName, int numFrames) {
    int last_index = 0;
    if (!bonesArr.isEmpty()) {
        last_index = bonesArr.last().toObject().value("index").toInt();
    }
    addLog(QString("\t[EDIT] Adding empty bone %1 with index %2").arg(boneName).arg(last_index + 1));

    QJsonObject newBoneObj = QJsonObject();
    newBoneObj.insert("BoneName", boneName);
    newBoneObj.insert("index", last_index + 1);

    double dt = 0.033333333;
    QString attrNames[3] = { "position", "rotation", "scale" };
    upn(j, 0, 2) {
        newBoneObj.insert(QString("%1_numFrames").arg(attrNames[j]), numFrames);
        QJsonArray attrArray = QJsonArray();
        upn(jj, 1, numFrames) {
            if (attrNames[j] == "rotation")
                attrArray.append(objXYZW(0,0,0,1));
            else if (attrNames[j] == "position")
                attrArray.append(objXYZ(0,0,0));
            else
                attrArray.append(objXYZ(1,1,1));
        }
        newBoneObj.insert(QString("%1Frames").arg(attrNames[j]), attrArray);
        newBoneObj.insert(QString("%1_dt").arg(attrNames[j]), dt);
    }
    bonesArr.append(newBoneObj);
}

void MAU::editBakeBones(QJsonObject& animObj, bool bakePos, bool bakeRot, bool bakeScale) {
    QJsonObject animBuff = animObj.value("animBuffer").toObject();
    int animFrames = animBuff.value("numFrames").toInt();
    QJsonArray animBones = animBuff.value("bones").toArray();

    addLog(QString("\t[EDIT] Bake %1 bones to %2 frames: pos - %3, rot - %4, scale - %5.")
           .arg(animBones.count())
           .arg(animFrames)
           .arg(bakePos ? "true" : "false")
           .arg(bakeRot ? "true" : "false")
           .arg(bakeScale ? "true" : "false"));


    for (int i = 0; i < animBones.count(); i += 1) {
        QString boneName = animBones[i].toObject().value("BoneName").toString();
        QJsonObject boneObj = animBones[i].toObject();
        int posFrames = boneObj.value("position_numFrames").toInt();
        int rotFrames = boneObj.value("rotation_numFrames").toInt();
        int scaleFrames = boneObj.value("scale_numFrames").toInt();
        if (bakePos && posFrames < animFrames) {
            QJsonArray posArr = boneObj.value("positionFrames").toArray();
            qDebug() << QString("BlendPos! bone %1, posFrames %2, animFrames %3").arg(boneName).arg(posFrames).arg(animFrames);
            blendPos(posArr, animFrames);
            boneObj["positionFrames"] = posArr;
            boneObj["position_numFrames"] = animFrames;
        }
        if (bakeRot && rotFrames < animFrames) {
            QJsonArray rotArr = boneObj.value("rotationFrames").toArray();
            qDebug() << QString("BlendRot! bone %1, rotFrames %2, animFrames %3").arg(boneName).arg(rotFrames).arg(animFrames);
            blendRot(rotArr, animFrames);
            boneObj["rotationFrames"] = rotArr;
            boneObj["rotation_numFrames"] = animFrames;
        }
        if (bakeScale && scaleFrames < animFrames) {
            QJsonArray scaleArr = boneObj.value("scaleFrames").toArray();
            qDebug() << QString("BlendScale! bone %1, scaleFrames %2, animFrames %3").arg(boneName).arg(scaleFrames).arg(animFrames);
            blendPos(scaleArr, animFrames); // the same logic
            boneObj["scaleFrames"] = scaleArr;
            boneObj["scale_numFrames"] = animFrames;
        }
        animBones[i] = boneObj;
    }

    animBuff["bones"] = animBones;
    animObj["animBuffer"] = animBuff;
}

bool MAU::editOptimizeBone(QJsonObject& boneObj, bool optimizePos, bool optimizeRot, bool optimizeScale) {
    int posNum = boneObj["position_numFrames"].toInt();
    int rotNum = boneObj["rotation_numFrames"].toInt();
    int scaleNum = boneObj["scale_numFrames"].toInt();

    optimizePos = posNum > 1;
    optimizeRot = rotNum > 1;
    optimizeScale = scaleNum > 1;
    QJsonArray posArray = boneObj["positionFrames"].toArray();
    QJsonArray rotArray = boneObj["rotationFrames"].toArray();
    QJsonArray scaleArray = boneObj["scaleFrames"].toArray();

    upn(frame, 1, posNum - 1) {
        double diffX = posArray[frame].toObject().value("x").toDouble() - posArray[frame - 1].toObject().value("x").toDouble();
        double diffY = posArray[frame].toObject().value("y").toDouble() - posArray[frame - 1].toObject().value("y").toDouble();
        double diffZ = posArray[frame].toObject().value("z").toDouble() - posArray[frame - 1].toObject().value("z").toDouble();
        double diffTotal = qAbs(diffX) + qAbs(diffY) + qAbs(diffZ);
        if (diffTotal > mReductionSensitivity()) {
            optimizePos = false;
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
            optimizeRot = false;
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
    if (optimizePos) {
        QJsonArray newPosArray;
        newPosArray.append( posArray.first() );
        boneObj["positionFrames"] = newPosArray;
        boneObj["position_numFrames"] = 1;
    }
    if (optimizeRot) {
        QJsonArray newRotArray;
        newRotArray.append( rotArray.first() );
        boneObj["rotationFrames"] = newRotArray;
        boneObj["rotation_numFrames"] = 1;
    }
    if (optimizeScale) {
        QJsonArray newScaleArray;
        newScaleArray.append( scaleArray.first() );
        boneObj["scaleFrames"] = newScaleArray;
        boneObj["scale_numFrames"] = 1;
    }
    return (optimizePos || optimizeRot || optimizeScale);
}

int MAU::editOptimizeBones(QJsonObject& animObj, bool optimizePos, bool optimizeRot, bool optimizeScale) {
    QJsonObject bufferObj = animObj["animBuffer"].toObject();
    QJsonArray bonesArray = bufferObj["bones"].toArray();
    int optimized = 0;
    int numBones = bonesArray.count();

    upn(i, 0, numBones - 1) {
        QJsonObject boneObj = bonesArray.at(i).toObject();
        bool boneOptimized = editOptimizeBone(boneObj, optimizePos, optimizeRot, optimizeScale);
        if (boneOptimized) {
            optimized += 1;
            bonesArray[i] = boneObj;
        }
    }

    if (optimized > 0) {
        bufferObj["bones"] = bonesArray;
        animObj["animBuffer"] = bufferObj;
    }
    addLog(QString("\t[EDIT] Optimized %1 bones of %2.").arg(optimized).arg(numBones));
    return optimized;
}

void MAU::onChecked_EditCutScale(bool checked) {
    if (ui->groupEditCut->isChecked() || ui->groupEditScale->isChecked()) {
        ui->groupEditBake->setChecked(true);
        ui->checkEditBakePos->setChecked(true);
        ui->checkEditBakeRot->setChecked(true);
        ui->groupEditBake->setEnabled(false);
    } else {
        ui->groupEditBake->setEnabled(true);
    }
}
void MAU::onClicked_EditGroupOptimize(bool checked) {
    if (checked) {
        ui->checkEditOptimizePos->setChecked(true);
        ui->checkEditOptimizeRot->setChecked(true);
        ui->checkEditOptimizeScale->setChecked(true);
    }
}
void MAU::onClicked_EditApply() {
    applyEdits();
}
void MAU::onClicked_EditApplyAll() {
    if (animSet) {
        ui->progressBar->setValue(0);
        for (int i = 0; i < m_animNames.count(); i += 1) {
            addLog(QString("[EDIT] Applying edits to anim #%1: %2").arg(i).arg(m_animNames[i]));
            m_animIndex = i;
            applyEdits();

            ui->progressBar->setValue((i + 1.0) * 100.0 / m_animNames.count());
            // YES it's dirty crutch
            QApplication::processEvents();
        }
        ui->progressBar->setValue(100);
    } else {
        addLog("[EDIT] Single anim loaded, applying edits to it.", logWarning);
        applyEdits();
    }
}
void MAU::onChanged_EditStart(int startFrame) {
    if (startFrame < 0) {
        ui->labelEditStart->setText("Start (- s) (frame):");
    } else {
        ui->labelEditStart->setText(QString("Start (%1 s) frame:")
                                    .arg(framesToSec(startFrame - 1), 0, 'f', 3));
        ui->spinEditDuration->setMaximum(m_framesCount - startFrame + 1);
    }
}
void MAU::onChanged_EditDuration(int durFrames) {
    if (durFrames < 0) {
        ui->labelEditDuration->setText("Duration (- s) frames:");
    } else {
        ui->labelEditDuration->setText(QString("Duration (%1 s) frames:")
                                    .arg(framesToSec(durFrames), 0, 'f', 3));
        ui->spinEditEnd->setValue(ui->spinEditStart->value() + durFrames - 1);
    }
}
void MAU::onChanged_EditEnd(int endFrame) {
    if (endFrame < 0) {
        ui->labelEditEnd->setText("End (- s) frame:");
    } else {
        ui->labelEditEnd->setText(QString("End (%1 s) frame:")
                                    .arg(framesToSec(endFrame), 0, 'f', 3));
        ui->spinEditDuration->setValue(endFrame - ui->spinEditStart->value() + 1);
    }
}
void MAU::onChanged_EditCurrentAnim(int newAnimIndex) {
    m_animEvents = QJsonArray();
    if (newAnimIndex < 0) {
        eventsLoad();
        ui->widgetEventsEdit->setEnabled(false);
        setCurrentAnimInfo(-1, -1, -1, -1, false, false);
        return;
    }
    m_animIndex = newAnimIndex;
    int animFrames = m_animFrames[m_animIndex];
    double animDuration = m_animDurations[m_animIndex];
    m_hasRootMotion = false;
    QJsonObject animObj = QJsonObject();

    if (animSet) {
        animObj = jsonRoot.value("animations").toArray().at(m_animIndex).toObject().value("animation").toObject();
        m_animEvents = jsonRoot.value("animations").toArray().at(m_animIndex).toObject().value("entries").toArray();
    } else {
        animObj = jsonRoot.value("animation").toObject();
        m_animEvents = jsonRoot.value("entries").toArray();

    }

    m_hasMotionExtraction = animObj.contains("motionExtraction")
                    && !animObj.value("motionExtraction").toObject().isEmpty();
    QJsonArray bonesArr = animObj.value("animBuffer").toObject().value("bones").toArray();
	m_numBones = bonesArr.count();
    for (auto boneVal : bonesArr) {
        QString boneName = boneVal.toObject().value("BoneName").toString();
        //qDebug() << "boneName: " << boneName;
        if (boneName == "RootMotion") {
            m_hasRootMotion = true;
            break;
        }
    }
    if (m_hasRootMotion) {
        ui->checkEditAddRootMotion->setChecked(false);
        ui->checkEditAddRootMotion->setEnabled(false);
    } else {
        ui->checkEditAddRootMotion->setEnabled(true);
    }
    ui->spinEditStart->setMaximum(animFrames);
    ui->spinEditDuration->setMaximum(animFrames);
    ui->spinEditEnd->setMaximum(animFrames);
    m_framesCount = animFrames;
    ui->widgetEventsEdit->setEnabled(true);
    eventsLoad();
    setCurrentAnimInfo(m_numBones, m_animEvents.count(), animFrames, animDuration, m_hasRootMotion, m_hasMotionExtraction);

    ui->spinMergeStart->setMaximum(m_framesCount);
    onChanged_MergeStart();
}

/* EVENTS */
JSO MAU::varToEntry(QString entryName, QVariant val, QString customType) {
    JSO entry = JSO();
    entry["Name"] = entryName;
    switch (val.type()) {
        case QMetaType::Bool:
        {
            entry["Type"] = "Bool";
            entry["val"] = val.toBool();
        }
            break;
        case QMetaType::Int:
        {
            entry["Type"] = "Int32";
            entry["val"] = val.toInt();
        }
            break;
        case QMetaType::Double:
        {
            entry["Type"] = "Float";
            entry["val"] = val.toDouble();
        }
            break;
        case QMetaType::QString:
        {
            if (customType.toUpper() == "CNAME") {
                entry["Type"] = "CName";
                entry["Value"] = val.toString();
            } else if (m_knownEnumTypes.contains(customType)) {
                entry["Type"] = customType;
                entry["Value"] = val.toString();
            } else {
                entry["Type"] = "StringAnsi";
                entry["val"] = val.toString();
            }
        }
            break;
        case QMetaType::QStringList:
        {
            entry["Type"] = "array:2,0,StringAnsi";
            JSA valArray = JSA();
            for (QString s : val.toStringList()) {
                valArray.append( s );
            }
            entry["Array"] = valArray;
        }
            break;
        default:
        {
            QVector< QPair<QString, QString> > mapKeys = QVector< QPair<QString, QString> >();
            QHash<QString, QVariant> map = val.value< QHash<QString, QVariant> >();
            JSA dataArr = JSA();

            if (customType == "SEnumVariant") {
                mapKeys = {
                    QPair<QString, QString>("enumType", "CName"),
                    QPair<QString, QString>("enumValue", "Int32")
                };
            } else if (customType == "CPreAttackEventData") {
                mapKeys = {
                    QPair<QString, QString>("attackName", "CName"),
                    QPair<QString, QString>("weaponSlot", "CName"),
                    QPair<QString, QString>("hitReactionType", "Int32"),
                    QPair<QString, QString>("rangeName", "CName"),
                    QPair<QString, QString>("Damage_Friendly", "Bool"),
                    QPair<QString, QString>("Damage_Neutral", "Bool"),
                    QPair<QString, QString>("Damage_Hostile", "Bool"),
                    QPair<QString, QString>("Can_Parry_Attack", "Bool"),
                    QPair<QString, QString>("hitFX", "CName"),
                    QPair<QString, QString>("hitBackFX", "CName"),
                    QPair<QString, QString>("hitParriedFX", "CName"),
                    QPair<QString, QString>("hitBackParriedFX", "CName"),
                    QPair<QString, QString>("swingType", "Int32"),
                    QPair<QString, QString>("swingDir", "Int32"),
                    QPair<QString, QString>("soundAttackType", "CName"),
                    QPair<QString, QString>("canBeDodged", "Bool"),
                    QPair<QString, QString>("cameraAnimOnMissedHit", "CName"),
                };
            } else {
                addLog(QString("[ERROR] varToEntry: unknown type %1 for %2").arg(entryName).arg(val.typeName()), logError);
                return JSO();
            }
            for (QPair<QString, QString> p : mapKeys) {
                if (map.contains(p.first)) {
                    dataArr.append( varToEntry(p.first, map.value(p.first), p.second) );
                }
            }
            entry["Content"] = dataArr;
            entry["Name"] = entryName;
            entry["Type"] = customType;
        }
            break;
    }
    return entry;
}

QVariant MAU::entryToVar(JSO entry) {
    if ( !entry.contains("Type") ) {
        qDebug() << "entryToVar: entry type not found! Name: " << entry.value("Name").toString();;
        return QVariant();
    }
    QString type = entry.value("Type").toString();
    if (type == "Bool") {
        return entry.value("val").toBool();
    } else if (type == "Int32") {
        return entry.value("val").toInt();
    } else if (type == "Float") {
        return entry.value("val").toDouble();
    } else if (type == "CName") {
        return entry.value("Value").toString();
    } else if (type == "StringAnsi") {
        return entry.value("val").toString();
    } else if (type == "array:2,0,StringAnsi") {
        QStringList ret = QStringList();
        JSA array = entry.value("Array").toArray();
        upn(i, 0, array.count()) {
            ret.append( array.at(i).toString() );
        }
        return ret;
    } else if (m_knownEnumTypes.contains(type)) {
        return entry.value("Value").toString();
    } else if (type == "SEnumVariant" || type == "CPreAttackEventData") {
        QHash<QString, QVariant> ret = QHash<QString, QVariant>();
        JSA array = entry.value("Content").toArray();

        upn(i, 0, array.count() - 1) {
            JSO arrObj = array.at(i).toObject();
            QString name = arrObj.value("Name").toString();

            ret.insert( name, entryToVar(arrObj) );
        }
        return ret;
    } else {
        addLog(QString("[ERROR] entryToVar: unknown type %1 for %2").arg(type).arg(entry["Name"].toString()), logError);
        return QVariant();
    }
}

void MAU::eventsLoad() {
    ui->listEvents->clear();

    if ( !m_animEvents.isEmpty() ) {
        upn(i, 0, m_animEvents.count() - 1) {
            eventsUpdateLabel(i, true);
        }
        ui->listEvents->setCurrentRow(0);
    }
}

QVariant MAU::getEventParam(QJsonObject eventObj, QString paramName, QVariant defaultValue) {
    QJsonArray contentArr = eventObj.value("Content").toArray();
    upn(j, 0, contentArr.count() - 1) {
        QJsonObject contentEntry = contentArr.at(j).toObject();
        if (contentEntry.value("Name") == paramName) {
            return contentEntry.value("val").toVariant();
        }
    }
    qDebug() << QString("Can't find event %1! Type: %2").arg(paramName).arg(eventObj.value("Type").toString());
    return defaultValue;
}

void MAU::eventsUpdateLabel(int eventIndex, bool add) {
    if (eventIndex < 0)
        return;
    if (add) {
        ui->listEvents->addItem("-");
        eventIndex = ui->listEvents->count() - 1;
    }
    if (eventIndex < ui->listEvents->count()) {
        QJsonObject eventObj = m_animEvents.at(eventIndex).toObject();
        QString type = eventObj.value("Type").toString();
        if ( !m_knownEventTypes.contains(type) ) {
            m_knownEventTypes.append(type);
            ui->comboEventsType->addItem(type);
        }
        double startTime = getEventStartTime(eventObj);
        double durationTime = getEventParam(eventObj, "duration", QVariant(-1.0)).toDouble();

        if (durationTime > 0.0)
            ui->listEvents->item(eventIndex)->setText( QString("%1 #%2 [%3 - %4 s]").arg(type).arg(eventIndex).arg(startTime, 0, 'f', 3).arg(startTime + durationTime, 0, 'f', 3) );
        else
            ui->listEvents->item(eventIndex)->setText( QString("%1 #%2 [%3 s]").arg(type).arg(eventIndex).arg(startTime, 0, 'f', 3) );
    }
}

void MAU::eventsUpdateContentLabel(int eventIndex, int index, bool add) {
    if (index < 0 || eventIndex < 0)
        return;
    if (add) {
        ui->listEventsContent->addItem("-");
        index = ui->listEventsContent->count() - 1;
    }
    //qDebug() << QString("eventsUpdateContentLabel: evIndex = %1, index = %2, add = %3").arg(eventIndex).arg(index).arg(add);
    if (index < ui->listEventsContent->count() && eventIndex < m_animEvents.count()) {
        JSO contentEntry = m_animEvents.at(eventIndex).toObject().value("Content").toArray().at(index).toObject();
        QString entryName = contentEntry.value("Name").toString();
        QString entryType = contentEntry.value("Type").toString();
        QVariant value = entryToVar(contentEntry);
        //qDebug() << QString("eventsUpdateContentLabel: entryType = %1").arg(entryType);
        if ( !value.canConvert(QMetaType::QString) ) {
            value = QString("<complex value>");
        }

        ui->listEventsContent->item(index)->setText( QString("%1 (%2) = %3").arg(entryName).arg(entryType).arg(value.toString()) );
    }
}

JSO MAU::defaultEntry(QString name, QString type) {
    JSO contentEntry = JSO();
    if (type == "Bool") {
        contentEntry = varToEntry(name, false);
    } else if (type == "Int32") {
        contentEntry = varToEntry(name, 0);
    } else if (type == "Float") {
        contentEntry = varToEntry(name, 0.0);
    } else if (type == "StringAnsi") {
        contentEntry = varToEntry(name, "");
    } else if (type == "CName") {
        contentEntry = varToEntry(name, "", "CName");
    } else if (type == "array:2,0,StringAnsi") {
        contentEntry = varToEntry( name, QStringList({""}) );
    } else if (m_knownEnumTypes.contains(type)) {
        contentEntry = varToEntry(name, m_knownEnumTypes[type].first(), type);
    } else if (type == "SEnumVariant") {
       QHash<QString, QVariant> map = QHash<QString, QVariant>();
       map["enumType"] = QString("ERotationRate");
       contentEntry = varToEntry( name, QVariant::fromValue(map), "SEnumVariant" );
    } else if (type == "CPreAttackEventData") {
        QHash<QString, QVariant> map = QHash<QString, QVariant>();
        map["attackName"] = QString("attackHeavy");
        map["rangeName"] = QString("basic_strike");
        map["soundAttackType"] = QString("monster_big_bite");
        map["hitReactionType"] = 1;
        contentEntry = varToEntry( name, QVariant::fromValue(map), "CPreAttackEventData" );
    } else {
        addLog(QString("[ERROR] defaultEntry: unknown type %1 for %2").arg(type).arg(name), logError);
    }
    return contentEntry;
}

// EVENTS slots
void MAU::onClicked_eventsSort() {
    editSortEvents(m_animEvents);
    eventsLoad();
    addLog(QString("[EVENTS] Sorted event entries: %2.")
           .arg(m_animEvents.count()));
}
void MAU::onClicked_fixAnimationNames() {
    int fixed_vars = 0;

    upn(eventIndex, 0, m_animEvents.count() - 1) {
        JSO eventObj = m_animEvents.at(eventIndex).toObject();
        JSA contentArr = eventObj.value("Content").toArray();
        upn(index, 0, contentArr.count() - 1) {
            JSO contentEntry = contentArr.at(index).toObject();
            if (contentEntry["Name"] == "animationName" && contentEntry["Value"].toString() != m_animNames[m_animIndex]) {
                contentEntry["Value"] = m_animNames[m_animIndex];
                contentArr[index] = contentEntry;
                ++fixed_vars;
            }
        }
        eventObj["Content"] = contentArr;
        m_animEvents[eventIndex] = eventObj;
    }
    eventsLoad();
    addLog(QString("[EVENTS] Fixed animationName vars: %1.")
           .arg(fixed_vars));
}
void MAU::onChecked_DynamicLabel(bool checked) {
    QCheckBox* box = qobject_cast<QCheckBox*>(sender());
    if (box != nullptr) {
        if (checked)
            box->setText("true");
        else
            box->setText("false");
    }
}
void MAU::onChecked_ToggleVar(bool checked) {
    QString controllerName = sender()->objectName();
    if ( !m_mapControlledBy.contains(controllerName) ) {
        qDebug() << QString("onChecked_ToggleVar[%1]: %2. Can't find control!").arg(checked).arg(sender()->objectName());
        return;
    }
    m_mapControlledBy[ controllerName ]->setEnabled(checked);
}
void MAU::onClicked_eventsReset() {
    m_animEvents = QJsonArray();
    if (m_animIndex >= 0) {
        if (animSet) {
            m_animEvents = jsonRoot.value("animations").toArray().at(m_animIndex).toObject().value("entries").toArray();
        } else {
            m_animEvents = jsonRoot.value("entries").toArray();
        }
    }
    eventsLoad();
}
void MAU::onClicked_eventsApply() {
    if (m_animIndex >= 0) {
        if (animSet) {
            JSA animArray = jsonRoot.value("animations").toArray();
            JSO animRootObj = animArray.at(m_animIndex).toObject();
            animRootObj["entries"] = m_animEvents;
            animArray[m_animIndex] = animRootObj;
            jsonRoot["animations"] = animArray;
        } else {
            jsonRoot["entries"] = m_animEvents;
        }
        addLog(QString("[EVENTS] Applied all changes to anim %1. Event entries: %2.")
               .arg(m_animNames[m_animIndex])
               .arg(m_animEvents.count()));
        setCurrentAnimInfo(m_numBones, m_animEvents.count(), m_animFrames[m_animIndex], m_animDurations[m_animIndex], m_hasRootMotion, m_hasMotionExtraction);
    }
}
void MAU::onClicked_eventsAdd() {
    JSO eventObj = JSO();
    JSA contentArr = JSA();

    QString type = ui->comboEventsType->currentText();
    // generic CExtAnimEvent
    contentArr.append( defaultEntry("eventName", "CName") );
    contentArr.append( defaultEntry("startTime", "Float") );
    contentArr.append( varToEntry("animationName", m_animNames[m_animIndex], "CName") );

    if (type.endsWith("DurationEvent") || type == "CEASEnumEvent" || type == "CPreAttackEvent") {
        contentArr.append( varToEntry("duration", qMax(0.01, m_animDurations[m_animIndex] - framesToSec(1))) );
        contentArr.append( defaultEntry("alwaysFiresEnd", "Bool") );
    }

    if (type.startsWith("CExtAnimEffect")) {
        contentArr.append( defaultEntry("effectName", "CName") );
    }
    if (type == "CExtAnimEffectEvent") {
        contentArr.append( defaultEntry("action", "EAnimEffectAction") );
    }
    if (type == "CExtAnimItemEvent") {
        contentArr.append( defaultEntry("category", "CName") );
        contentArr.append( defaultEntry("itemName_optional", "CName") );
        contentArr.append( defaultEntry("ignoreItemsWithTag", "CName") );
        contentArr.append( defaultEntry("action", "EItemAction") );
    }
    if (type == "CExtAnimItemEffectEvent") {
        contentArr.append( defaultEntry("effectName", "CName") );
        contentArr.append( defaultEntry("itemSlot", "CName") );
        contentArr.append( defaultEntry("action", "EItemEffectAction") );
    }

    if (type.startsWith("CExp")) {
        contentArr.append( defaultEntry("translation", "Bool") );
        contentArr.append( defaultEntry("rotation", "Bool") );
    }

    if (type == "CExtAnimAttackEvent") {
        contentArr.append( defaultEntry("soundAttackType", "CName") );
    }

    if (type == "CExtAnimSoundEvent" || type == "CExtAnimFootstepEvent") {
        contentArr.append( defaultEntry("soundEventName", "StringAnsi") );
        contentArr.append( defaultEntry("maxDistance", "Float") );
        contentArr.append( defaultEntry("bone", "CName") );
        contentArr.append( defaultEntry("switchesToUpdate", "array:2,0,StringAnsi") );
    }

    if (type == "CExtAnimFootstepEvent") {
        contentArr.append( defaultEntry("fx", "Bool") );
        contentArr.append( defaultEntry("customFxName", "CName") );
    }

    if (type == "CEASEnumEvent") {
        contentArr.append( defaultEntry("enumVariant", "SEnumVariant") );
    }

    if (type == "CPreAttackEvent") {
        contentArr.append( defaultEntry("data", "CPreAttackEventData") );
    }

    eventObj["Content"] = contentArr;
    eventObj["Name"] = type;
    eventObj["Type"] = type;
    m_animEvents.append(eventObj);

    eventsUpdateLabel(m_animEvents.count(), true);
    onChanged_eventRow(m_animEvents.count() - 1);
}
void MAU::onClicked_eventsClone() {
    int index = ui->listEvents->currentRow();
    JSO eventObj = m_animEvents.at(index).toObject();
    m_animEvents.append(eventObj);

    eventsUpdateLabel(m_animEvents.count(), true);
    onChanged_eventRow(m_animEvents.count() - 1);
}
void MAU::onClicked_eventsRemove() {
    int index = ui->listEvents->currentRow();
    m_animEvents.removeAt(index);
    eventsLoad(); // reload
}
void MAU::onChanged_eventRow(int newRow) {
    ui->listEventsContent->clear();
    //qDebug() << QString("onChanged_eventRow: %1").arg(newRow);
    if (newRow < 0) {
        ui->buttonEventsClone->setEnabled(false);
        ui->buttonEventsRemove->setEnabled(false);
        ui->buttonEventsType->setEnabled(false);
        return;
    }
    ui->buttonEventsClone->setEnabled(true);
    ui->buttonEventsRemove->setEnabled(true);
    ui->buttonEventsType->setEnabled(true);

    if (newRow < m_animEvents.count()) {
        JSO eventObj = m_animEvents.at(newRow).toObject();
        QString type = eventObj.value("Type").toString();

        JSA contentArr = eventObj.value("Content").toArray();
        if ( !contentArr.isEmpty() ) {
            upn(i, 0, contentArr.count() - 1) {
                eventsUpdateContentLabel(newRow, i, true);
            }
        }
        ui->comboEventsType->setCurrentText(type);
        ui->listEventsContent->setCurrentRow(0);
    }
}
void MAU::eventsUpdateContentData(QHash<QString, QVariant>& map, QCheckBox* pController, QString key, QString type) {
    QVariant val = QVariant();
    QWidget* pElement = m_mapControlledBy[pController->objectName()];
    Q_ASSERT(pElement);
    if (!map.contains(key)) {
        pController->setChecked(false);
        pElement->setEnabled(false);
        return;
    }
    val = map.value(key);
    map.remove(key);
    pController->setChecked(true);
    pElement->setEnabled(true);

    if (type == "Bool") {
        qobject_cast<QCheckBox*>(pElement)->setChecked( val.toBool() );
    } else if (type == "Int32") {
        qobject_cast<QSpinBox*>(pElement)->setValue( val.toInt() );
    //} else if (type == "Enum") {
    //    qobject_cast<QComboBox*>(pElement)->setCurrentIndex( val.toInt() );
    } else if (type == "Float") {
        qobject_cast<QDoubleSpinBox*>(pElement)->setValue( val.toFloat() );
    } else if (type == "StringAnsi" || type == "CName") {
        qobject_cast<QLineEdit*>(pElement)->setText( val.toString() );
    } else if (m_knownEnumTypes.contains(type)) {
        qobject_cast<QComboBox*>(pElement)->setCurrentIndex( val.toInt() );
    } else {
        addLog( QString("eventsUpdateContentData: unknown type %1 for key %2.").arg(type).arg(key), logError );
    }
}
void MAU::eventsLoadContentData(QHash<QString, QVariant>& map, QCheckBox* pController, QString key, QString type) {
    if ( !pController->isChecked() )
        return;

    QWidget* pElement = m_mapControlledBy[pController->objectName()];
    Q_ASSERT(pElement);
    if (type == "Bool") {
        map[key] = qobject_cast<QCheckBox*>(pElement)->isChecked();
    } else if (type == "Int32") {
        map[key] = qobject_cast<QSpinBox*>(pElement)->value();
    } else if (type == "Float") {
        map[key] = qobject_cast<QDoubleSpinBox*>(pElement)->value();
    } else if (type == "StringAnsi" || type == "CName") {
        map[key] = qobject_cast<QLineEdit*>(pElement)->text();
    } else if (m_knownEnumTypes.contains(type)) {
        map[key] = qobject_cast<QComboBox*>(pElement)->currentIndex();
    } else {
        qDebug() << QString("eventsLoadContentData: unknown type %1 for key %2").arg(type).arg(key);
    }
}
void MAU::onChanged_eventContentRow(int newRow) {
    if (newRow < 0) {
        ui->buttonEventsVarRemove->setEnabled(false);
        ui->buttonEventsVarRename->setEnabled(false);
        ui->buttonEventsVarType->setEnabled(false);
        return;
    }
    ui->buttonEventsVarRemove->setEnabled(true);
    ui->buttonEventsVarRename->setEnabled(true);
    ui->buttonEventsVarType->setEnabled(true);

    int eventIndex = ui->listEvents->currentRow();
    JSA contentArr = m_animEvents.at(eventIndex).toObject().value("Content").toArray();
    if (newRow < contentArr.count()) {
        JSO contentEntry = contentArr.at(newRow).toObject();
        QString entryName = contentEntry.value("Name").toString();
        QString entryType = contentEntry.value("Type").toString();
        QVariant value = entryToVar( contentEntry );

        m_eventsAcceptSignals = false;
        ui->lineEventsVarName->setText(entryName);
        ui->comboEventsVarType->setCurrentText(entryType);
        if (entryType == "Bool") {
            ui->stackEventsValue->setCurrentIndex(0);
            ui->checkEventsValueBool->setChecked( value.toBool() );
        } else if (entryType == "Int32") {
            ui->stackEventsValue->setCurrentIndex(1);
            ui->spinEventsValueInt->setValue( value.toInt() );
        } else if (entryType == "Float") {
            ui->stackEventsValue->setCurrentIndex(2);
            if (entryName == "startTime" || entryName == "duration") {
                ui->spinEventsValueFloat->setMinimum(0.0);
                ui->spinEventsValueFloat->setMaximum(m_animDurations[m_animIndex]);
                ui->spinEventsValueFloat->setSingleStep( framesToSec(1) );
            } else {
                ui->spinEventsValueFloat->setMinimum(-999999);
                ui->spinEventsValueFloat->setMaximum(999999);
                ui->spinEventsValueFloat->setSingleStep( 0.1 );
            }
            ui->spinEventsValueFloat->setValue( value.toDouble() );
        } else if (entryType == "StringAnsi" || entryType == "CName") {
            ui->stackEventsValue->setCurrentIndex(3);
            ui->lineEventsValueString->setText( value.toString() );
        } else if (entryType == "array:2,0,StringAnsi") {
            ui->stackEventsValue->setCurrentIndex(4);
            ui->editEventsValueArrayString->setPlainText( value.toStringList().join("\n") );
        } else if (m_knownEnumTypes.contains(entryType)) {
            ui->stackEventsValue->setCurrentIndex(7);
            ui->comboEventsValueEnum->clear();
            ui->comboEventsValueEnum->addItems( m_knownEnumTypes[entryType] );
            ui->comboEventsValueEnum->setCurrentText( value.toString() );
        } else if (entryType == "SEnumVariant") {
            ui->stackEventsValue->setCurrentIndex(5);
            QHash<QString, QVariant> map = value.value< QHash<QString, QVariant> >();

            eventsUpdateContentData(map, ui->checkEventsVarEnumType, "enumType", "CName");
            eventsUpdateContentData(map, ui->checkEventsVarEnumValue, "enumValue", "Int32");
            if ( !map.isEmpty() ) {
                addLog(QString("[WARNING] SEnumVariant %1, the following vars are unknown for this type: %2").arg(entryName).arg(map.keys().join("; ")), logWarning);
            }
        } else if (entryType == "CPreAttackEventData") {
            ui->stackEventsValue->setCurrentIndex(6);
            QHash<QString, QVariant> map = value.value< QHash<QString, QVariant> >();

            eventsUpdateContentData(map, ui->checkEventsVarAttackName, "attackName", "CName");
            eventsUpdateContentData(map, ui->checkEventsVarAttackRange, "rangeName", "CName");
            eventsUpdateContentData(map, ui->checkEventsVarAttackWeaponSlot, "weaponSlot", "CName");
            eventsUpdateContentData(map, ui->checkEventsVarAttackHitReaction, "hitReactionType", "EHitReactionType");
            eventsUpdateContentData(map, ui->checkEventsVarAttackDamageFriendly, "Damage_Friendly", "Bool");
            eventsUpdateContentData(map, ui->checkEventsVarAttackDamageNeutral, "Damage_Neutral", "Bool");
            eventsUpdateContentData(map, ui->checkEventsVarAttackCanParry, "Can_Parry_Attack", "Bool");
            eventsUpdateContentData(map, ui->checkEventsVarAttackHitFx, "hitFX", "CName");
            eventsUpdateContentData(map, ui->checkEventsVarAttackHitBackFx, "hitBackFX", "CName");
            eventsUpdateContentData(map, ui->checkEventsVarAttackHitParriedFx, "hitParriedFX", "CName");
            eventsUpdateContentData(map, ui->checkEventsVarAttackHitBackParriedFx, "hitBackParriedFX", "CName");
            eventsUpdateContentData(map, ui->checkEventsVarAttackSwingType, "swingType", "EAttackSwingType");
            eventsUpdateContentData(map, ui->checkEventsVarAttackSwingDir, "swingDir", "EAttackSwingDirection");
            eventsUpdateContentData(map, ui->checkEventsVarAttackSound, "soundAttackType", "CName");
            eventsUpdateContentData(map, ui->checkEventsVarAttackCanBeDodged, "canBeDodged", "Bool");
            if ( !map.isEmpty() ) {
                addLog(QString("[WARNING] CPreAttackEventData %1, the following vars are unknown for this type: %2").arg(entryName).arg(map.keys().join("; ")), logWarning);
            }
        }

        m_eventsAcceptSignals = true;
        qDebug() << QString("onChanged_eventContentRow: %1, var %2 (%3)").arg(newRow).arg(entryName).arg(entryType);
    }
}
void MAU::onClicked_eventsVarRename() {
    int index = ui->listEventsContent->currentRow();
    int eventIndex = ui->listEvents->currentRow();
    QString newName = ui->lineEventsVarName->text();

    JSO eventObj = m_animEvents.at(eventIndex).toObject();
    JSA contentArr = eventObj.value("Content").toArray();
    JSO contentEntry = contentArr.at(index).toObject();

    contentEntry["Name"] = newName;
    contentArr[index] = contentEntry;
    eventObj["Content"] = contentArr;
    m_animEvents[eventIndex] = eventObj;
    eventsUpdateContentLabel(eventIndex, index, false);
}
void MAU::onClicked_eventsSetType() {
    QString newType = ui->comboEventsType->currentText();
    int eventIndex = ui->listEvents->currentRow();

    JSO eventObj = m_animEvents.at(eventIndex).toObject();
    eventObj["Type"] = newType;
    eventObj["Name"] = newType;
    m_animEvents[eventIndex] = eventObj;
    eventsUpdateLabel(eventIndex, false);
}

void MAU::onClicked_eventsVarSetType() {
    //qDebug() << QString("onClicked_eventsVarSetType: %1").arg(sender()->objectName());
    int eventIndex = ui->listEvents->currentRow();
    int index = ui->listEventsContent->currentRow();

    if (eventIndex < 0 || eventIndex >= m_animEvents.count())
        return;
    JSO eventObj = m_animEvents.at(eventIndex).toObject();
    JSA contentArr = eventObj.value("Content").toArray();
    JSO contentEntry = contentArr[index].toObject();

    QString name = contentEntry["Name"].toString();
    QString oldType = contentEntry["Type"].toString();
    QString newType = ui->comboEventsVarType->currentText();
    if (oldType == newType)
        return;

    contentArr[index] = defaultEntry(name, newType);
    eventObj["Content"] = contentArr;
    m_animEvents[eventIndex] = eventObj;

    eventsUpdateContentLabel(eventIndex, index, false);
    onChanged_eventContentRow(index);
}
void MAU::onClicked_eventsVarAdd() {
    //qDebug() << QString("onClicked_eventsVarAdd: %1").arg(sender()->objectName());
    int eventIndex = ui->listEvents->currentRow();

    if (eventIndex < 0 || eventIndex >= m_animEvents.count())
        return;
    JSO eventObj = m_animEvents.at(eventIndex).toObject();
    JSA contentArr = eventObj.value("Content").toArray();

    QString type = ui->comboEventsVarType->currentText();
    QString name = ui->lineEventsVarName->text();
    JSO contentEntry = defaultEntry(name, type);
    contentArr.append(contentEntry);
    eventObj["Content"] = contentArr;
    m_animEvents[eventIndex] = eventObj;

    int index = contentArr.count() - 1;
    eventsUpdateContentLabel(eventIndex, index, true);
}
void MAU::onClicked_eventsVarRemove() {
    //qDebug() << QString("onClicked_eventsVarRemove: %1").arg(sender()->objectName());
    int eventIndex = ui->listEvents->currentRow();
    int index = ui->listEventsContent->currentRow();

    if (eventIndex < 0 || eventIndex >= m_animEvents.count())
        return;
    JSO eventObj = m_animEvents.at(eventIndex).toObject();
    JSA contentArr = eventObj.value("Content").toArray();
    contentArr.removeAt(index);
    eventObj["Content"] = contentArr;
    m_animEvents[eventIndex] = eventObj;
    onChanged_eventRow(eventIndex);
}
void MAU::onChanged_eventsVarAny() {
    if (!m_eventsAcceptSignals)
        return;
    //qDebug() << QString("onChanged_eventsVarAny: %1").arg(sender()->objectName());
    int eventIndex = ui->listEvents->currentRow();
    int index = ui->listEventsContent->currentRow();

    if (eventIndex < 0 || eventIndex >= m_animEvents.count())
        return;
    JSO eventObj = m_animEvents.at(eventIndex).toObject();
    JSA contentArr = eventObj.value("Content").toArray();
    JSO contentEntry = contentArr[index].toObject();
    QString entryName = contentEntry["Name"].toString();
    QString entryType = contentEntry["Type"].toString();

    if (entryType == "Bool") {
        contentEntry = varToEntry( entryName, ui->checkEventsValueBool->isChecked() );
    } else if (entryType == "Int32") {
        contentEntry = varToEntry( entryName, ui->spinEventsValueInt->value() );
    } else if (entryType == "Float") {
        contentEntry = varToEntry( entryName, ui->spinEventsValueFloat->value() );
    } else if (entryType == "StringAnsi" || entryType == "CName") {
        contentEntry = varToEntry( entryName, ui->lineEventsValueString->text(), entryType );
    } else if (entryType == "array:2,0,StringAnsi") {
        contentEntry = varToEntry( entryName, ui->editEventsValueArrayString->toPlainText().split("\n"), entryType );
    } else if (m_knownEnumTypes.contains(entryType)) {
        contentEntry = varToEntry( entryName, ui->comboEventsValueEnum->currentText(), entryType );
    } else if (entryType == "SEnumVariant") {
        ui->stackEventsValue->setCurrentIndex(5);
        QHash<QString, QVariant> map = QHash<QString, QVariant>();

        eventsLoadContentData(map, ui->checkEventsVarEnumType, "enumType", "CName");
        eventsLoadContentData(map, ui->checkEventsVarEnumValue, "enumValue", "Int32");
        contentEntry = varToEntry( entryName, QVariant::fromValue(map), entryType );
    } else if (entryType == "CPreAttackEventData") {
        ui->stackEventsValue->setCurrentIndex(6);
        QHash<QString, QVariant> map = QHash<QString, QVariant>();

        eventsLoadContentData(map, ui->checkEventsVarAttackName, "attackName", "CName");
        eventsLoadContentData(map, ui->checkEventsVarAttackRange, "rangeName", "CName");
        eventsLoadContentData(map, ui->checkEventsVarAttackWeaponSlot, "weaponSlot", "CName");
        eventsLoadContentData(map, ui->checkEventsVarAttackHitReaction, "hitReactionType", "EHitReactionType");
        eventsLoadContentData(map, ui->checkEventsVarAttackDamageFriendly, "Damage_Friendly", "Bool");
        eventsLoadContentData(map, ui->checkEventsVarAttackDamageNeutral, "Damage_Neutral", "Bool");
        eventsLoadContentData(map, ui->checkEventsVarAttackCanParry, "Can_Parry_Attack", "Bool");
        eventsLoadContentData(map, ui->checkEventsVarAttackHitFx, "hitFX", "CName");
        eventsLoadContentData(map, ui->checkEventsVarAttackHitBackFx, "hitBackFX", "CName");
        eventsLoadContentData(map, ui->checkEventsVarAttackHitParriedFx, "hitParriedFX", "CName");
        eventsLoadContentData(map, ui->checkEventsVarAttackHitBackParriedFx, "hitBackParriedFX", "CName");
        eventsLoadContentData(map, ui->checkEventsVarAttackSwingType, "swingType", "EAttackSwingType");
        eventsLoadContentData(map, ui->checkEventsVarAttackSwingDir, "swingDir", "EAttackSwingDirection");
        eventsLoadContentData(map, ui->checkEventsVarAttackSound, "soundAttackType", "CName");
        eventsLoadContentData(map, ui->checkEventsVarAttackCanBeDodged, "canBeDodged", "Bool");
        contentEntry = varToEntry( entryName, QVariant::fromValue(map), entryType );
    }

    contentArr[index] = contentEntry;
    eventObj["Content"] = contentArr;
    m_animEvents[eventIndex] = eventObj;

    eventsUpdateContentLabel(eventIndex, index, false);
    if ((entryName == "startTime" || entryName == "duration") && entryType == "Float") {
        eventsUpdateLabel(eventIndex, false);
    }
}
/* JSON loading */
bool MAU::loadW3Data() {
    // make cleanup here!
    /* extra nr */
    m_animNames.clear();
    m_animFrames.clear();
    m_animDurations.clear();
    ui->comboEditAnims->clear();
    ui->buttonApplyMotionToBone->setEnabled(false);
    ui->buttonExtractMotionFromBone->setEnabled(false);

    if ( jsonRoot.contains("animations") ) {
        animSet = true; // exported from wkit
        QJsonArray animsObj = jsonRoot.value("animations").toArray();
        ui->progressBar->setValue(0);
        int animCount = animsObj.count();
        for (int i = 0; i < animCount; i += 1) {
            QJsonObject animObj = animsObj.at(i).toObject().value("animation").toObject();
            QString name = animObj.value("name").toString();
            double duration = animObj.value("duration").toDouble();
            m_framesCount = animObj.value("animBuffer").toObject().value("numFrames").toInt();
            qDebug() << QString("Add anim #%1 (animset): %2, %3 frames").arg(i).arg(name).arg(m_framesCount);
            m_animNames.append(name);
            m_animDurations.append(duration);
            m_animFrames.append(m_framesCount);
            ui->comboEditAnims->addItem(QString("[%1] %2").arg(i + 1).arg(name));

            ui->progressBar->setValue((i + 1.0) * 100.0 / animCount);
            // YES it's dirty crutch
            QApplication::processEvents();
        }
        ui->progressBar->setValue(100);

        m_framesCount = -1;
        if (animCount > 0) {
            //onUpdate_MergeInfo();
            ui->buttonApplyMotionToBone->setEnabled(true);
            ui->buttonExtractMotionFromBone->setEnabled(true);
            ui->spinSplitStart->setMaximum( animCount );
            ui->spinSplitEnd->setMaximum( animCount );
            onChanged_EditCurrentAnim(0);
            addLog( QString("[OK] Detected array of %1 anims (probably, exported from WolvenKit)").arg(animCount) );
            if ( jsonRoot.contains("SCutsceneActorDefs") ) {
                addLog( QString("[OK] Detected cutscene definition.").arg(animCount) );
            }
            return true;
        } else {
            addLog( QString("Expected array of anims, but it doesn't exist or is empty"), logError );
            return false;
        }
    } else if ( jsonRoot.contains("animation") ) {
        animSet = false; // exported from maya
        QJsonObject animObj = jsonRoot.value("animation").toObject();
        QString name = animObj.value("name").toString();
        double duration = animObj.value("duration").toDouble();
        m_framesCount = animObj.value("animBuffer").toObject().value("numFrames").toInt();
        m_animNames.append(name);
        m_animDurations.append(duration);
        m_animFrames.append(m_framesCount);
        ui->comboEditAnims->addItem(name);

        int m_bonesCount = animObj.value("animBuffer").toObject().value("bones").toArray().count();
        if ( duration < 0.03 || m_framesCount < 1 ) {
            addLog( QString("Detected single anim with incorrect duration: %1 s, %2 frames").arg(duration).arg(m_framesCount), logError );
            return false;
        }
        if ( m_bonesCount < 1 ) {
            addLog( QString("Detected single anim with incorrect bones amount (%1 bones)").arg(m_bonesCount), logError );
            return false;
        }

        //onUpdate_MergeInfo();
        ui->buttonApplyMotionToBone->setEnabled(true);
        ui->buttonExtractMotionFromBone->setEnabled(true);
        ui->spinSplitStart->setMaximum( 1 );
        ui->spinSplitEnd->setMaximum( 1 );
        onChanged_EditCurrentAnim(0);
        addLog( QString("[OK] Detected single anim (probably, exported from Maya)") );
        addLog( QString("[Info] name: %1, duration: %2 s, bones amount: %3").arg(name).arg(duration).arg(m_bonesCount) );
        return true;
    } else {
        //onUpdate_MergeInfo();
        return false;
    }
}
void MAU::onClicked_Save() {
    if ( jsonRoot.empty() ) {
        addLog( QString("Seems that no file was loaded!"), logError );
        return;
    }
    if ( !jsonFile.open(QFile::WriteOnly | QFile::Truncate) ) {
        addLog( QString("Can't save into file: %1").arg(jsonFile.fileName()), logError );
        return;
    }

    hasChanges = false;
    jsonDoc.setObject(jsonRoot);
    QByteArray bArray = jsonDoc.toJson();
    jsonFile.write(bArray);
    jsonFile.close();
    addLog( QString("[OK] File saved: %1").arg(jsonFile.fileName()), logFinish );
}
void MAU::onClicked_SaveSplit() {
    if ( jsonRoot.contains("animations") ) {
        QJsonArray animArray = jsonRoot.value("animations").toArray();
        if (animArray.count() > 0) {
            if ( jsonRoot.contains("SCutsceneActorDefs") ) {
                addLog( QString("Detected cutscene definition! Use special w2cutscene section to handle it."), logWarning );
                return;
            }
            addLog( QString("[INFO] Splitting array of %1 anims.").arg(animArray.count()) );

            QString dirPath = QFileInfo(jsonFile.fileName()).absolutePath() + "/splitted";
            QDir splitDir(dirPath);
            if (!splitDir.exists()) {
                splitDir.mkpath(".");
            }
            upn(i, 0, animArray.count() - 1) {
                QJsonObject animObj = animArray.at(i).toObject();
                QJsonObject jsonRootSplit = QJsonObject();
                QString animName = animObj.value("animation").toObject().value("name").toString();

                jsonRootSplit["animation"] = animObj.value("animation").toObject();
                if (animObj.contains("entries")) {
                    jsonRootSplit["entries"] = animObj.value("entries").toArray();
                }
                // new json
                QFile jsonFileSplit(splitDir.path() + "/" + animName + ".w2anims.json");
                if ( !jsonFileSplit.open(QFile::WriteOnly | QFile::Truncate) ) {
                    addLog( QString("Can't save anim to file: %1").arg(jsonFileSplit.fileName()), logError );
                    continue;
                }
                QJsonDocument jsonDocSplit;
                jsonDocSplit.setObject(jsonRootSplit);
                jsonFileSplit.write(jsonDocSplit.toJson());
                jsonFileSplit.close();

                addLog( QString("[OK] Anim #%1/%2 saved to: %3").arg(i + 1).arg(animArray.count()).arg(jsonFileSplit.fileName()) );
                // YES it's dirty crutch
                QApplication::processEvents();
            }
            hasChanges = false;
            addLog( QString("[OK] Done!"), logFinish );

        } else {
            addLog( QString("Expected array of anims, but it doesn't exist or is empty!"), logError );
            return;
        }
    } else {
        addLog( QString("Animset wasn't detected, saving as usual."), logWarning );
        onClicked_Save();
    }
}

double MAU::framesToSec(int frames, int fps) {
    return double(frames) / fps;
}

int MAU::secToFrames(double sec, int fps) {
    return int(sec * fps + 0.05);
}
QJsonObject MAU::objXYZ(double X, double Y, double Z) const {
    QJsonObject ret;
    ret["x"] = QJsonValue(X);
    ret["y"] = QJsonValue(Y);
    ret["z"] = QJsonValue(Z);
    return ret;
}
void MAU::objToXYZ(QJsonObject obj, double& X, double& Y, double& Z) const {
    if (obj.isEmpty())
        qDebug() << "Empty!!";
    X = obj["x"].toDouble();
    Y = obj["y"].toDouble();
    Z = obj["z"].toDouble();
}
void MAU::objToXYZW(QJsonObject obj, double& X, double& Y, double& Z, double& W) const {
    X = obj["X"].toDouble();
    Y = obj["Y"].toDouble();
    Z = obj["Z"].toDouble();
    W = obj["W"].toDouble();
}
QJsonObject MAU::objXYZW(double X, double Y, double Z, double W) const {
    QJsonObject ret;
    ret["X"] = QJsonValue(X);
    ret["Y"] = QJsonValue(Y);
    ret["Z"] = QJsonValue(Z);
    ret["W"] = QJsonValue(W);
    return ret;
}
QJsonObject MAU::objQuanternion(double Pitch, double Yaw, double Roll) const {
    QVector4D vec4 = QQuaternion::fromEulerAngles(QVector3D(Pitch, Yaw, Roll)).toVector4D();
    return objXYZW( vec4.x(), vec4.y(), vec4.z(), vec4.w() );
}
JSO MAU::interpolateObj(double k, QJsonObject prevObj, QJsonObject nextObj) {
    double X1, Y1, Z1, W1;
    double X2, Y2, Z2, W2;
    if (prevObj.contains("W")) {
        objToXYZW(prevObj, X1, Y1, Z1, W1);
        objToXYZW(nextObj, X2, Y2, Z2, W2);
        interpolateRot(k, X1, Y1, Z1, W1, X2, Y2, Z2, W2);
        return objXYZW(X1, Y1, Z1, W1);
    } else {
        objToXYZ(prevObj, X1, Y1, Z1);
        objToXYZ(nextObj, X2, Y2, Z2);
        interpolatePos(k, X1, Y1, Z1, X2, Y2, Z2);
        return objXYZ(X1, Y1, Z1);
    }
}
void MAU::interpolatePos(double k, double& X1, double& Y1, double& Z1, double X2, double Y2, double Z2) {
    X1 = X1 * (1.0 - k) + X2 * k;
    Y1 = Y1 * (1.0 - k) + Y2 * k;
    Z1 = Z1 * (1.0 - k) + Z2 * k;
}
void MAU::interpolateRot(double k, double& X1, double& Y1, double& Z1, double& W1, double X2, double Y2, double Z2, double W2) {
    QQuaternion q1 = QQuaternion(W1, X1, Y1, Z1);
    QQuaternion q2 = QQuaternion(W2, X2, Y2, Z2);
    q1 = QQuaternion::nlerp(q1, q2, k);
    X1 = q1.x();
    Y1 = q1.y();
    Z1 = q1.z();
    W1 = q1.scalar();
}
void MAU::blendMotion(QVector<double>& motion, int animFrames, const QVector<int>& framePoints) {
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
void MAU::blendPos(QJsonArray& posArray, int targetFrames) {
    QJsonArray resArray;
    int frames = posArray.count();
    qDebug() << QString("blendPos [%1] -> [%2]").arg(frames).arg(targetFrames);
    if (frames > targetFrames) {
        addLog(QString("[ERROR] BlendPos: can't bake! posArray size (%1) is bigger than required (%2).").arg(frames).arg(targetFrames), logError);
    }
    if (frames == targetFrames)
        return;
    if (frames == 1) {
        while (resArray.count() < targetFrames) {
            resArray.append( posArray.first() );
        }
        posArray = resArray;
        return;
    }
    resArray.append( posArray.first() );

    double partSize = (targetFrames - 1.0) / (frames - 1.0);
    double currentPartSize = 0.0;
    qDebug() << QString("blendPos.partSize = %1").arg(partSize);
    int j = 0;
    double X1, Y1, Z1;
    double X2, Y2, Z2;
    for (int i = 2; i < targetFrames; i += 1) {
        currentPartSize += 1.0;
        //qDebug() << QString("blendPos.frame: %1, currentPartSize = %1").arg(currentPartSize);
        if (currentPartSize > partSize) {
            j += 1;
            currentPartSize -= partSize;
        }
        double k = currentPartSize / partSize;
        //qDebug() << QString("blendPos.frame: %1, k = %2, currentPartSize = %3").arg(i).arg(k).arg(currentPartSize);
        k = qMin(1.0, qMax(0.0, k));
        // [j..j+1]
        objToXYZ(posArray[j].toObject(),       X1, Y1, Z1);
        objToXYZ(posArray[j + 1].toObject(),   X2, Y2, Z2);
        interpolatePos(k, X1, Y1, Z1, X2, Y2, Z2);
        resArray.append( objXYZ(X1, Y1, Z1) );
    }
    qDebug() << QString("blendPos.lastK = %1 (must be around 1.0)").arg((currentPartSize + 1.0) / partSize);
    resArray.append( posArray.last() );
    posArray = resArray;
    return;
}
void MAU::blendRot(QJsonArray& rotArray, int targetFrames) {
    QJsonArray resArray;
    int frames = rotArray.count();
    qDebug() << QString("blendRot [%1] -> [%2]").arg(frames).arg(targetFrames);
    if (frames > targetFrames) {
        addLog(QString("[ERROR] BlendPos: can't bake! posArray size (%1) is bigger than required (%2).").arg(frames).arg(targetFrames), logError);
    }
    if (frames == targetFrames)
        return;
    if (frames == 1) {
        while (resArray.count() < targetFrames) {
            resArray.append( rotArray.first() );
        }
        rotArray = resArray;
        return;
    }
    resArray.append( rotArray.first() );

    double partSize = (targetFrames - 1.0) / (frames - 1.0);
    qDebug() << QString("blendRot.partSize = %1").arg(partSize);
    double currentPartSize = 0.0;
    int j = 0;
    double X1, Y1, Z1, W1;
    double X2, Y2, Z2, W2;
    for (int i = 2; i < targetFrames; i += 1) {
        currentPartSize += 1.0;
        if (currentPartSize > partSize) {
            j += 1;
            currentPartSize -= partSize;
        }
        double k = currentPartSize / partSize;
        //qDebug() << QString("blendRot.frame: %1, k = %2, currentPartSize = %3").arg(i).arg(k).arg(currentPartSize);
        k = qMin(1.0, qMax(0.0, k));
        // [j..j+1]
        objToXYZW(rotArray[j].toObject(),       X1, Y1, Z1, W1);
        objToXYZW(rotArray[j + 1].toObject(),   X2, Y2, Z2, W2);
        interpolateRot(k, X1, Y1, Z1, W1, X2, Y2, Z2, W2);
        resArray.append( objXYZW(X1, Y1, Z1, W1) );
    }
    qDebug() << QString("blendRot.lastK = %1 (must be around 1.0)").arg((currentPartSize + 1.0) / partSize);
    resArray.append( rotArray.last() );
    rotArray = resArray;
    return;
}

bool MAU::isAdditiveAnim(QJsonObject animObj) {
    QString animName = animObj["name"].toString();

    bool isAdditive = additiveNames.contains(animName);
    //qDebug() << "Check anim: " << animName << ", contains: " << isAdditive;

    if ( !isAdditive ) {
        QJsonObject bufferObj = animObj["animBuffer"].toObject();
        QJsonArray bonesArray = bufferObj["bones"].toArray();
        //int nullMotionBones = 0;
        int testAllBones = 0;
        int testAbsBones = 0;
        double avPos = 0.0;

        upn(i, 0, bonesArray.size() - 1) {
            QJsonObject tempObj = bonesArray[i].toObject();
            QString boneName = tempObj.value("BoneName").toString();
            //int posNum = tempObj["position_numFrames"].toInt();
            //int rotNum = tempObj["rotation_numFrames"].toInt();
            //int scaleNum = tempObj["scale_numFrames"].toInt();

            QJsonObject tpos = tempObj["positionFrames"].toArray().at(0).toObject();
            QJsonObject trot = tempObj["rotationFrames"].toArray().at(0).toObject();
            double sumPos = qAbs(tpos.value("x").toDouble()) + qAbs(tpos.value("y").toDouble()) + qAbs(tpos.value("z").toDouble());
            //double sumRot = qAbs(trot.value("X").toDouble()) + qAbs(trot.value("Y").toDouble()) + qAbs(trot.value("Z").toDouble());
            //double diffW = qAbs( qAbs(trot.value("W").toDouble()) - 1.0 );

            ++testAllBones;
            if (sumPos < 0.000001) {
                ++testAbsBones;
            }
            avPos += sumPos;
        }
        //qDebug() << "testAbsBones: " << testAbsBones << ", testAllBones: " << testAllBones << ", avPos: " << avPos / bonesArray.size();

        if (testAbsBones + 3 >= testAllBones) { // equal in fact
            isAdditive = true;
        }
    }
    if (isAdditive) {
        additiveNames.append(animName);
        additiveTypes.append("auto-detected");
    }
    return isAdditive;
}

bool MAU::applyMotionToBone(QJsonValueRef ref) {
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
    if (!ui->checkConvertME->isChecked()) {
        addLog("    Not asked to convert motionExtraction, skipping.");
        ref = animObj;
        return false;
    }

    //double animDuration = animObj["duration"].toDouble();
    QJsonObject bufferObj = animObj["animBuffer"].toObject();
    QJsonArray bonesArray = bufferObj["bones"].toArray();
    if ( bonesArray.isEmpty() ) {
        addLog("    Corrupted bones (or anim) buffer, skipping.", logError);
        return false;
    }

    /* TEST! Trajectory !
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
    */

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
    
    int deltaTotal = 0;
    QVector<int> framePoints;
    framePoints.append(1);

    if (motionObj["deltaTimes"].isString()) {
        QByteArray deltaTimes = QByteArray::fromBase64( motionObj["deltaTimes"].toString().toUtf8() );
        upn(i, 0, deltaTimes.size() - 1) {
            framePoints.append( framePoints.back() + static_cast<uint8_t>(deltaTimes[i]) );
            deltaTotal += static_cast<uint8_t>(deltaTimes[i]);
        }
    } else if (motionObj["deltaTimes"].isArray()) {
        JSA deltaTimes = motionObj["deltaTimes"].toArray();
        upn(i, 0, deltaTimes.size() - 1) {
            framePoints.append( framePoints.back() + deltaTimes[i].toInt() );
            deltaTotal += deltaTimes[i].toInt();
        }
    } else {
        addLog(QString("[ERROR] Unknown deltaTimes format: %1").arg(motionObj["deltaTimes"].type()), logError);
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
    if (framePoints.size() != framesSets || deltaTotal > animFrames || deltaTotal < animFrames - 1) {
        addLog(QString("    Incorrect motionExtraction [framePointsSize = %1, framesSets = %2, deltaTotal = %3, animFrames = %4], setting zero RootMotion.")
               .arg(framePoints.size()).arg(framesSets).arg(deltaTotal).arg(animFrames), logError);
        anyFlag = false;
    }
    /*if (deltaTotal == animFrames) {
        addLog(QString("    Fixing motionExtraction [deltaTotal = %3 == animFrames = %4]").arg(deltaTotal).arg(animFrames), logWarning);
        deltaTimes.back() = (int)deltaTimes.back() - 1;
    }*/

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
        } else {
            mBoneObj["position_numFrames"] = QJsonValue(1);
            mBoneObj["positionFrames"] = QJsonArray({ objXYZ(0, 0, 0) });
        }

        addLog("    [FINISH] Processed anim: " + animName, logFinish);
    }

    // END
    bonesArray[mBoneIdx] = mBoneObj;
    bufferObj["bones"] = bonesArray;
    animObj["animBuffer"] = bufferObj;
    ref = animObj;
    
    return true;
}
void MAU::onClicked_applyMotionToBone() {
    eTimer.start();
    ui->progressBar->setValue(0);

    if (animSet) {
        QJsonArray animArray = jsonRoot["animations"].toArray();
        QJsonArray animArrayRes = QJsonArray();
        upn(i, 0, animArray.size() - 1) {
            QJsonObject animObj = animArray[i].toObject();
            bool isAdditive = isAdditiveAnim(animObj["animation"].toObject());
            if ( applyMotionToBone(animObj["animation"]) ) {
                hasChanges = true;
            }

            addLog(QString("additive anim: %1").arg(isAdditive));
            if ((ui->radioLeaveOnlyNormal->isChecked() && !isAdditive)
              || (ui->radioLeaveOnlyAdditives->isChecked() && isAdditive)
              || ui->radioLeaveAll->isChecked()) {
                animArrayRes.append(animObj);
            }
            ui->progressBar->setValue((i + 1.0) * 100.0 / animArray.size());
            // YES it's dirty crutch
            QApplication::processEvents();
        }
        if (animArray.count() != animArrayRes.count()) {
            hasChanges = true;
        }
        if (ui->checkSplitSet->isChecked()) {
            QJsonArray animArrayRes2 = QJsonArray();
            for (int i = ui->spinSplitStart->value() - 1; i <= ui->spinSplitEnd->value() - 1; ++i) {
                animArrayRes2.append( animArrayRes.at(i) );
            }
            animArrayRes = animArrayRes2;
        }

        jsonRoot["animations"] = animArrayRes;
        addLog(QString("[Finished in %1s] Processed %2 animations. Anims in result: %3<br>").arg(eTimer.elapsed() / 1000.0).arg(animArray.size()).arg(animArrayRes.count()), logFinish);
    } else {
        if ( applyMotionToBone(jsonRoot["animation"]) ) {
            hasChanges = true;
        }
        addLog(QString("[Finished in %1s] Processed 1 animation.<br>").arg(eTimer.elapsed() / 1000.0), logFinish);
        ui->progressBar->setValue(100);
    }
}

bool MAU::extractMotionFromBone(QJsonValueRef ref) {
    QJsonObject animObj = ref.toObject();
    if (animObj.isEmpty()) {
        addLog("Empty anim object, skipping.", logError);
        return false;
    }
    QString animName = animObj["name"].toString();
    qDebug() << "*** ANIM: " << animName;
    addLog("[START] Processing anim: " + animName);

    if ( animObj.contains("motionExtraction") && !onlyPrint ) {
        addLog("    Overwriting existing motionExtraction.", logWarning);
        animObj.remove("motionExtraction");
    }

    double animDuration = animObj["duration"].toDouble();
    /* extra nr */
    m_animNames.push_back(animName);
    m_animDurations.push_back(animDuration);

    int bonesOptimized = 0;
    if ( ui->checkOptimizeEqual->isChecked() ) {
        bonesOptimized = editOptimizeBones(animObj, true, true, true);
    }

    QJsonObject bufferObj = animObj["animBuffer"].toObject();
    QJsonArray bonesArray = bufferObj["bones"].toArray();
    if ( bonesArray.isEmpty() ) {
        addLog("Empty bones/anim buffer, skipping.", logError);
        return false;
    }

    int animFrames = bufferObj["numFrames"].toInt();
    int mBoneIdx = -1;
    QStringList brokenBones;


    upn(i, 0, bonesArray.size() - 1) {
        QJsonObject boneObj = bonesArray[i].toObject();
        QString boneName = boneObj.value("BoneName").toString();

        if (boneName == mBoneName) {
            mBoneIdx = i;
        }
        /*int posNum = boneObj["position_numFrames"].toInt();
        int rotNum = boneObj["rotation_numFrames"].toInt();
        int scaleNum = boneObj["scale_numFrames"].toInt();

        QString error;
        if (posNum != animFrames && posNum != 1) {
            if (ui->checkAutoBakeIncomplete->isChecked()) {
                QJsonArray posArray = boneObj["positionFrames"].toArray();
                while (posArray.count() != animFrames) {
                    posArray.append( posArray.last() );
                }
                boneObj["positionFrames"] = posArray;
                boneObj["position_numFrames"] = animFrames;
            }
            error += QString(" [translation = %1];").arg(posNum);
        }

        if (rotNum != animFrames && rotNum != 1) {
            if (ui->checkAutoBakeIncomplete->isChecked()) {
                QJsonArray rotArray = boneObj["rotationFrames"].toArray();
                while (rotArray.count() != animFrames) {
                    rotArray.append( rotArray.last() );
                }
                boneObj["rotationFrames"] = rotArray;
                boneObj["rotation_numFrames"] = animFrames;
            }
            error += QString(" [rotation = %1];").arg(rotNum);
        }

        if (scaleNum != animFrames && scaleNum != 1) {
            if (ui->checkAutoBakeIncomplete->isChecked()) {
                QJsonArray scaleArray = boneObj["scaleFrames"].toArray();
                while (scaleArray.count() != animFrames) {
                    scaleArray.append( scaleArray.last() );
                }
                boneObj["scaleFrames"] = scaleArray;
                boneObj["scale_numFrames"] = animFrames;
            }
            error += QString(" [scale = %1];").arg(scaleNum);
        }

        if (!error.isEmpty()) {
            if (ui->checkAutoBakeIncomplete->isChecked()) {
                bonesArray[i] = boneObj;
                ++bonesBaked;
            }
            if (boneName.endsWith("_roll") || boneName.startsWith("IK_"))
                brokenBones.append( boneName + ":" + error );
            else
                brokenBones.append( "!!! " + boneName + ":" + error );
        }*/
    }

    bool isAdditive = isAdditiveAnim(animObj);
    if ( (additiveNames.contains(animName) && isAdditive) || (!additiveNames.contains(animName) && !isAdditive) ) {
        qDebug() << "ADD MATCH :) == " << isAdditive;
    } else {
        qDebug() << "ADD WRONG :(" << isAdditive;
    }
    if ( isAdditive && ui->checkFixAdditives->isChecked() && !onlyPrint ) {
        bonesOptimized += 1;
        QString additiveType = additiveTypes.at( additiveNames.indexOf(animName) );
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
                if (additiveType == "AT_Ref") {
                    nArray.append( objXYZW(0.0, 0.0, 0.0, -1.0) );
                } else {
                    nArray.append( objXYZW(-0.00048828125, -0.00048828125, -0.00048828125, 0.999577046) );
                }
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

    if (onlyPrint) {
        return false;
    }

    if (bonesOptimized) {
        addLog( QString("    [OPTIMIZE] Set single frame to %1 bones.").arg(bonesOptimized) );
    }

    if ( !brokenBones.isEmpty() && !shownBroken ) {
        shownBroken = true;
        if (ui->checkAutoBakeIncomplete->isChecked())
            QMessageBox::information(this, "Warning!", QString("Detected bones with incorrect numFrames in anim [%1] (numFrames = %2)\nBones were auto-baked to numFrames (may make anim wrong): %3").arg(animName).arg(animFrames).arg(brokenBones.join("\n")));
        else
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
            double tmp_deltaRotZ = motionRotZ[frame + 1] - motionRotZ[frame];
            double diff_totalDelta = qAbs(tmp_deltaX - deltaX) + qAbs(tmp_deltaY - deltaY)
                            + qAbs(tmp_deltaZ - deltaZ) + qAbs(tmp_deltaRotZ - deltaRotZ);
            //qDebug() << QString("[%1] total delta: %2,   X: %3, Y: %4, Z: %5, Rot: %6").arg(frame).arg(diff_totalDelta).arg(tmp_deltaX).arg(tmp_deltaY).arg(tmp_deltaZ).arg(tmp_deltaRotZ);

            if (diff_totalDelta < mReductionSensitivity() && frame - lastDelta < 255) {
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
                //qDebug() << QString("[%1] NEW delta: X: %3, Y: %4, Z: %5, Rot: %6").arg(frame).arg(motionX[frame]).arg(motionY[frame]).arg(motionZ[frame]).arg(motionRotZ[frame]);
            }
        }
        int frame = animFrames - 1;
        if (frame - 1 > 0)
            deltaTimes.append(frame - lastDelta);
        tmp_motionX.append(motionX[frame]);
        tmp_motionY.append(motionY[frame]);
        tmp_motionZ.append(motionZ[frame]);
        tmp_motionRotZ.append(motionRotZ[frame]);
        //qDebug() << QString("[%1] NEW delta: X: %3, Y: %4, Z: %5, Rot: %6").arg(frame).arg(motionX[frame]).arg(motionY[frame]).arg(motionZ[frame]).arg(motionRotZ[frame]);
        // ^ last frame

        motionX = tmp_motionX;
        motionY = tmp_motionY;
        motionZ = tmp_motionZ;
        motionRotZ = tmp_motionRotZ;
        rotFrames = posFrames = motionX.size();
        //qDebug() << "deltaTimes: " << deltaTimes;
        //qDebug() << "Reduce: " << animFrames << " -> " << posFrames << " frames.";
        addLog(QString("    [OPTIMIZE] Motion: Removed excess frames: %1 -> %2.").arg(animFrames).arg(posFrames));
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
            addLog(QString("    [OPTIMIZE] Motion: Remove unused X axis frames."));
        }
        if (isSingleY) {
            flags = flags ^ BYTE_Y;
            addLog(QString("    [OPTIMIZE] Motion: Remove unused Y axis frames."));
        }
        if (isSingleZ) {
            flags = flags ^ BYTE_Z;
            addLog(QString("    [OPTIMIZE] Motion: Remove unused Z axis frames."));
        }
        if (isSingleRotZ) {
            flags = flags ^ BYTE_RotZ;
            addLog(QString("    [OPTIMIZE] Motion: Remove unused Rotation Z axis frames."));
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
    addLog("    [FINISH] Processed anim: " + animName, logFinish);
    animObj["motionExtraction"] = motionExtractionObj;
    ref = animObj;
    return true;
}
void MAU::onClicked_extractMotionFromBone() {
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
                    hasChanges = true;
                    addLog( QString("    [EVENTS] Added %1 entries").arg(animEventsByName[animName].count()) );
                }
            }

            animArray[i] = animObj;
            ui->progressBar->setValue((i + 1.0) * 100.0 / animArray.size());
            // YES it's dirty crutch
            QApplication::processEvents();
        }

        jsonRoot["animations"] = animArray;
        addLog(QString("[Finished in %1s] Processed %2 animations.<br>").arg(eTimer.elapsed() / 1000.0).arg(animArray.size()), logFinish);
    } else {
        if ( extractMotionFromBone(jsonRoot["animation"]) ) {
            hasChanges = true;
        }
        if ( ui->checkLoadAnimEvents->isChecked() ) {
            QJsonValue nameVal = jsonRoot.value("animation").toObject().value("name");
            QString animName = (nameVal.isString() ? nameVal.toString() : "NONE");
            if ( animEventsByName.contains(animName) ) {
                jsonRoot["entries"] = animEventsByName[animName];
                hasChanges = true;
                addLog( QString("    [EVENTS] Added %1 entries").arg(animEventsByName[animName].count()) );
            }
        }

        addLog(QString("[Finished in %1s] Processed 1 animation.<br>").arg(eTimer.elapsed() / 1000.0), logFinish);
        if (!batchMode)
            ui->progressBar->setValue(100);
    }
}
void MAU::onClicked_extractMotionFromBoneBatch() {
    batchMode = true;
    ui->progressBar->setValue(0);
    //additiveNames.clear();
    m_animNames.clear();
    m_animDurations.clear();

    QString dirName = QFileDialog::getExistingDirectory(this, tr("Choose directory"),
                                                        QDir::currentPath());
    if (dirName.isEmpty()) {
        addLog("User canceled operation.", logWarning);
        return;
    }
    QStringList jsonList = QDir(dirName).entryList({"*.json"}, QDir::Files, QDir::Name);

    int current = 0;
    for (QString& jsonPath : jsonList) {
        ++current;
        addLog("[BATCH] Processing file: " + jsonPath);
        if (loadJsonFile(dirName + "/" + jsonPath)) {
            onClicked_extractMotionFromBone();
            // YES it's dirty crutch
            QApplication::processEvents();
            qDebug() << "hasChanges: " << hasChanges;
            if (hasChanges) {
                onClicked_Save();
            }
        }
        ui->progressBar->setValue( current * 100.0 / jsonList.count() );
        // YES it's dirty crutch
        QApplication::processEvents();
    }
    addLog(QString("[BATCH] Completed processing %1 files.").arg(jsonList.count()), logFinish);

    /* extra nr */
    QString res = QString("ADDITIVE anims: %1\n").arg(additiveNames.count());
    for (QString i : additiveNames) {
        res += "        " + i + "\n";
    }
    res += "\n||||||||||||||||||||||||||\n\n";

    res += "    function addAnims() {\n";
    upn(i, 0, m_animNames.size() - 1) {
        QString add = "false);";
        qDebug() << "i: " << i;
        if (additiveNames.contains(m_animNames[i])) {
            add = "true); //" + additiveTypes.at( additiveNames.indexOf(m_animNames[i]) );
        }
       res += QString("		addAnim('%1', %2, %3\n").arg(m_animNames[i]).arg(m_animDurations[i]).arg(add);
    }
    res += "    }\n";
    QTextEdit* textEdit = new QTextEdit();
    textEdit->setFontPointSize(11);
    textEdit->setPlainText(res);
    textEdit->showMaximized();

    ui->progressBar->setValue(100);
    batchMode = false;
}

bool MAU::loadJsonAnimEvents(QString filePath) {
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
    if ( ui->checkBak->isChecked() ) {
        if ( QFile::exists(filePath + ".bak") ) {
            QFile::remove(filePath + ".bak");
        }
        jsonFileEvents.copy(filePath + ".bak");
    }
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
            addLog( QString("Json: added events for animation [%1] %2: %3").arg(i).arg(nameVal.toString()).arg(eventsArr.count()) );
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
void MAU::onClicked_loadAnimEventsSource() {
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

    if ( loadJsonMotion(filePath) ) {
        ui->checkUseExternalMotion->setText( QString("motion: Use external file if it contains anim with the same name (currently loaded: %1)").arg(QFileInfo(filePath).fileName()) );
    } else {
        ui->checkUseExternalMotion->setText( QString("motion: Use external file if it contains anim with the same name (currently loaded: %1)").arg("NONE") );
    }
}

bool MAU::loadJsonMotion(QString filePath) {
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
void MAU::onClicked_loadMotionSource() {
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

void MAU::onClicked_loadTxtDump() {
    QString filePath = QFileDialog::getOpenFileName(this, "Open text dump from wkit", "", "TEXT Files (*.txt)");
    if (filePath.isEmpty()) {
        addLog("Loading file canceled by user", logWarning);
        return;
    }
    QFile inputFile(filePath);
    if (inputFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
       QTextStream in(&inputFile);
       while (!in.atEnd())
       {
          QString line = in.readLine();
          QStringList parts = line.split("|");
          if (parts[1] != "SAT_Normal") {
              qDebug() << "additive: " << parts[0];
              additiveNames.append(parts[0]);
              additiveTypes.append(parts[2]);
          }
       }
       inputFile.close();
    }
}

void MAU::onClicked_PrintInfo() {
    batchMode = true;
    onlyPrint = true;
    ui->progressBar->setValue(0);
    //additiveNames.clear();
    m_animNames.clear();
    m_animDurations.clear();

    QString dirName = QFileDialog::getExistingDirectory(this, tr("Choose directory"),
                                                        QDir::currentPath());
    if (dirName.isEmpty()) {
        addLog("User canceled operation.", logWarning);
        return;
    }
    QStringList jsonList = QDir(dirName).entryList({"*.json"}, QDir::Files, QDir::Name);

    int current = 0;
    for (const QString &jsonPath : jsonList) {
        ++current;
        addLog("[BATCH] Processing file: " + jsonPath);
        if (loadJsonFile(dirName + "/" + jsonPath)) {
            onClicked_extractMotionFromBone();
            // YES it's dirty crutch
            QApplication::processEvents();
        }
        ui->progressBar->setValue( current * 100.0 / jsonList.count() );
        // YES it's dirty crutch
        QApplication::processEvents();
    }
    addLog(QString("[BATCH] Completed processing %1 files.").arg(jsonList.count()), logFinish);

    /* extra nr */
    QString res = QString("ADDITIVE anims: %1\n").arg(additiveNames.count());
    for (QString i : additiveNames) {
        res += "        " + i + "\n";
    }
    res += "\n||||||||||||||||||||||||||\n\n";

    res += "    function addAnims() {\n";
    upn(i, 0, m_animNames.count() - 1) {
        QString add = "false);";
        qDebug() << "L2: " << i;
        if (additiveNames.contains(m_animNames[i])) {
            add = "true); //" + additiveTypes.at( additiveNames.indexOf(m_animNames[i]) );
        }
        res += QString("		addAnim('%1', %2, %3\n").arg(m_animNames[i]).arg(m_animDurations[i]).arg(add);
    }
    res += "    }\n";
    QTextEdit* textEdit = new QTextEdit();
    textEdit->setFontPointSize(11);
    textEdit->setPlainText(res);
    textEdit->showMaximized();

    ui->progressBar->setValue(100);
    batchMode = false;
    onlyPrint = false;
}

QJsonArray MAU::extractAnimParts(QJsonValueRef ref) {
    bool mergeParts = ui->checkExtractCutsceneMerge->isChecked();
    QJsonArray resultArray = QJsonArray();

    QJsonObject animObj = ref.toObject();
    if (animObj.isEmpty()) {
        addLog("Empty anim object, skipping.", logError);
        return resultArray;
    }
    QString animName = animObj["name"].toString();
    if (ui->lineCutsceneFilter1->text() != QString() && !animName.startsWith(ui->lineCutsceneFilter1->text())) {
        return resultArray;
    }

    qDebug() << "*** CUTSCENE ANIM: " << animName;
    addLog("[START] Processing anim parts: " + animName);

    if ( animObj.contains("motionExtraction") && animObj["motionExtraction"].isNull() ) {
        addLog("    Removing empty motionExtraction.", logWarning);
        animObj.remove("motionExtraction");
    }

    //double animDuration = animObj["duration"].toDouble();
    QJsonObject bufferObj = animObj["animBuffer"].toObject();
    //int numFrames = bufferObj["numFrames"].toInt();
    QJsonArray firstFrames = bufferObj["firstFrames"].toArray();
    QJsonArray parts = bufferObj["parts"].toArray();

    upn(i, 0, parts.count() - 1) {
        QJsonObject resObj;
        QJsonObject resAnimObj;
        QJsonObject partsObj = parts[i].toObject();
        if (mergeParts && i > 0) {
            // TODO
        } else {
            resAnimObj = animObj;
            if (!mergeParts) {

                resAnimObj["duration"] = partsObj["duration"];
                resAnimObj["name"] = QString("%1+%2").arg(animName).arg(i);
                resAnimObj["animBuffer"] = partsObj;

                resObj["animation"] = resAnimObj;
                resultArray.append(resObj);
            }
        }
    }
    return resultArray;
}
void MAU::onClicked_extractPartsCutscene() {
    eTimer.start();
    ui->progressBar->setValue(0);

    if (!animSet) {
        return;
    }

    // new json
    QFile jsonFileParts;
    QJsonDocument jsonDocParts;
    QJsonObject jsonRootParts;
    jsonRootParts["animations"] = QJsonArray();

    QJsonArray animArray = jsonRoot["animations"].toArray();
    QJsonArray animArrayParts = jsonRootParts["animations"].toArray();

    upn(i, 0, animArray.size() - 1) {
        QJsonObject animObj = animArray[i].toObject();
        QJsonArray animObjParts = extractAnimParts(animObj["animation"]);

        upn(j, 0, animObjParts.size() - 1) { // contains 1 if merged, PARTS count if not merged
            animArrayParts.append(animObjParts[j]);
        }
        ui->progressBar->setValue((i + 1.0) * 100.0 / animArray.size());
        // YES it's dirty crutch
        QApplication::processEvents();
    }
    addLog(QString("[Finished in %1s] Processed %2 animations.<br>").arg(eTimer.elapsed() / 1000.0).arg(animArray.size()), logFinish);

    // SAVE
    jsonRootParts["animations"] = animArrayParts;

    jsonFileParts.setFileName(jsonFile.fileName().replace(".w2cutscene.json", ".w2anims.json"));
    if ( !jsonFileParts.open(QFile::WriteOnly | QFile::Truncate) ) {
        addLog( QString("Can't save parts into file: %1").arg(jsonFileParts.fileName()), logError );
        return;
    }

    jsonDocParts.setObject(jsonRootParts);
    QByteArray bArray = jsonDocParts.toJson();
    jsonFileParts.write(bArray);
    jsonFileParts.close();
    addLog( QString("[OK] Anim parts extracted: %1").arg(jsonFileParts.fileName()), logFinish );
}

void MAU::patchAnimParts(QString jsonPath, QJsonValueRef animArrayRef) {
    addLog("[PATCH CS] Processing anim json: " + jsonPath);
    QFile animFile(jsonPath);
    if ( !animFile.open(QFile::ReadOnly) ) {
        addLog( QString("Can't open file: %1").arg(animFile.fileName()), logError );
        return;
    }

    QByteArray bArray = animFile.readAll();
    QJsonParseError* jsonError = new QJsonParseError;
    QJsonDocument animDoc = QJsonDocument::fromJson(bArray, jsonError);
    if ( jsonDoc.isNull() ) {
        addLog( QString("Can't load json document correctly, parse error = %1").arg(jsonError->errorString()), logError );
        return;
    } else if ( !animDoc.isObject() ) {
        addLog( "Json root is not an object, can't load info.", logError );
        return;
    }

    QJsonObject animObj = animDoc.object().value("animation").toObject();
    if (animObj.isEmpty()) {
        addLog( "Json object is empty!", logError );
        return;
    }
    QString animName = animObj.value("name").toString();
    addLog("[PATCH CS] Anim name: " + animName);
    QStringList animNameParts = animName.split("+"); // [0] - name, [1] - idx
    QJsonArray animBones = animObj.value("animBuffer").toObject().value("bones").toArray();
    double dur = animObj.value("animBuffer").toObject().value("duration").toDouble();
    int numFrames = animObj.value("animBuffer").toObject().value("numFrames").toInt();
    if (animBones.isEmpty()) {
        addLog( "Json bones object is empty / anim name is incorrect!", logError );
        return;
    }
    addLog(QString("[CS PATCH] PART info: bones = %1, len = %2 s, frames = %3").arg(animBones.count()).arg(dur).arg(numFrames));

    QJsonArray animArray = animArrayRef.toArray();
    upn(i, 0, animArray.count() - 1) {
        QJsonObject animObj0 = animArray[i].toObject();
        QJsonObject animObj1 = animObj0["animation"].toObject();
        QString animName0 = animObj1["name"].toString();
        if (animNameParts[0] == animName0) {
            QJsonObject animBuffer0 = animObj1["animBuffer"].toObject();
            QJsonArray animBufferParts0 = animBuffer0["parts"].toArray();
            if (animNameParts[1].toInt() >= animBufferParts0.count()) {
                addLog( QString("Too big part index: %1, parts in anim: %2").arg(animNameParts[1]).arg(animBufferParts0.count()), logError );
                continue;
            }
            QJsonObject partObject0 = animBufferParts0[ animNameParts[1].toInt() ].toObject();

            //addLog(QString("BBBones count: %1").arg(partObject0["bones"].toArray().count()), logInfo);
            partObject0["bones"] = animBones;
            //addLog(QString("PArts count: %1").arg(animBuffer0["parts"].toArray().count()), logInfo);
            animBufferParts0[ animNameParts[1].toInt() ] = partObject0;
            animBuffer0["parts"] = animBufferParts0;
            //addLog(QString("PPPArts count: %1").arg(animBuffer0["parts"].toArray().count()), logInfo);
            animObj1["animBuffer"] = animBuffer0;
            animObj0["animation"] = animObj1;
            animArray[i] = animObj0;
        }
    }
    animArrayRef = animArray;
}
void MAU::onClicked_patchPartsCutscene() {
    ui->progressBar->setValue(0);

    QString dirName = QFileDialog::getExistingDirectory(this, tr("Choose directory with anim jsons"),
                                                        QDir::currentPath());
    if (dirName.isEmpty()) {
        addLog("User canceled operation.", logWarning);
        return;
    }
    QStringList jsonList = QDir(dirName).entryList({"*.json"}, QDir::Files, QDir::Name);

    int current = 0;

    QJsonValueRef animArrayRef = jsonRoot["animations"];
    for (const QString jsonPath : jsonList) {
        ++current;
        patchAnimParts(dirName + "/" + jsonPath, animArrayRef);

        ui->progressBar->setValue( current * 100.0 / jsonList.count() );
        // YES it's dirty crutch
        QApplication::processEvents();
    }
}

void MAU::onChanged_MergeStart() {
    ui->spinMergeDuration->setMaximum( qMin(m_framesCount - ui->spinMergeStart->value() + 1, m_secondAnimFrames) );
    ui->labelMergeStart->setText(QString("Merge start (%1 s) frame:").arg(framesToSec(ui->spinMergeStart->value() - 1)));
}
void MAU::onChanged_MergeDuration() {
    ui->labelMergeDuration->setText(QString("Merge duration (%1 s) frames:").arg(framesToSec(ui->spinMergeDuration->value())));
}

void MAU::saveMergedJson(QJsonObject root, QString suffix) {
    QString animName = root.value("animation").toObject().value("name").toString();
    QString newFileName = QFileInfo(m_mergeFilePath).absolutePath() + "/" + animName + "_" + suffix + "_";
    int idx = 0;
    while (QFile::exists(newFileName + QString::number(idx) + ".w2anims.json"))
        ++idx;
    newFileName += QString::number(idx) + ".w2anims.json";

    QFile mergedFile( newFileName );
    if ( !mergedFile.open(QFile::WriteOnly | QFile::Truncate) ) {
        addLog( QString("Can't save merged anim to file: %1").arg(mergedFile.fileName()), logError );
        return;
    }
    QJsonDocument jsonMergedDoc;
    jsonMergedDoc.setObject(root);
    mergedFile.write(jsonMergedDoc.toJson());
    mergedFile.close();

    addLog( QString("[OK] Merged anim %1 saved to: %2").arg(animName).arg(mergedFile.fileName()), logFinish );
}
bool MAU::loadMergeJson(QString filePath) {
    m_secondAnimFrames = -1;
    jsonFileMerge.setFileName(filePath);
    if ( !jsonFileMerge.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        addLog("Can't open file in read-only text mode: " + filePath, logError);
        return false;
    }

    QByteArray bArray = jsonFileMerge.readAll();
    jsonFileMerge.close();
    QJsonParseError* jsonError = new QJsonParseError;
    jsonDocMerge = QJsonDocument::fromJson(bArray, jsonError);
    if ( jsonDocMerge.isNull() ) {
        addLog( QString("Can't load json document correctly, parse error = %1").arg(jsonError->errorString()), logError );
        return false;
    } else if ( !jsonDocMerge.isObject() ) {
        addLog( "Json root is not an object, can't load info.", logError );
        return false;
    }

    jsonRootMerge = jsonDocMerge.object();
    QJsonObject animObj = jsonRootMerge.value("animation").toObject();
    if (animObj.isEmpty()) {
        addLog("[ERROR] Can't load [animation] chunk from 2nd anim .json. Make sure it's not an animset.", logError);
    }
    m_secondAnimName = animObj.value("name").toString();
    m_secondAnimFrames = animObj.value("animBuffer").toObject().value("numFrames").toDouble();
    m_secondAnimDuration = animObj.value("duration").toDouble();
    m_secondAnimEvents = jsonRootMerge.value("entries").toArray().count();

    m_secondAnimMotionExtraction = false;
    m_secondAnimRootMotion = false;
    m_secondAnimMotionExtraction = animObj.contains("motionExtraction")
                                && !animObj.value("motionExtraction").toObject().isEmpty();
    QJsonArray bonesArr = animObj.value("animBuffer").toObject().value("bones").toArray();
    m_secondAnimBones = bonesArr.count();
    for (auto boneVal : bonesArr) {
        if (boneVal.toObject().value("BoneName").toString() == "RootMotion") {
            m_secondAnimRootMotion = true;
            break;
        }
    }

    ui->spinMergeStart->setMaximum(m_framesCount);
    onChanged_MergeStart();
    ui->spinMergeDuration->setMinimum(1);
    ui->spinMergeDuration->setValue( ui->spinMergeDuration->maximum() );
    //onChanged_MergeDuration();
    //onUpdate_MergeInfo();
    return true;
}
void MAU::onClicked_LoadMergeJson() {
    QString filePath = QFileDialog::getOpenFileName(this, "Select 2nd anim json for merge", "", "JSON Files (*.json)");
    if (filePath.isEmpty()) {
        addLog("Loading file canceled by user", logWarning);
        return;
    }
    if ( loadMergeJson(filePath) ) {
        m_mergeFilePath = filePath;
        addLog( QString("[OK] Loaded 2nd anim: %1").arg(QFileInfo(filePath).fileName()), logFinish );
    } else {
        m_secondAnimFrames = -1;
        addLog( QString("[ERROR] Can't load file: %1").arg(QFileInfo(filePath).absoluteFilePath()), logError );
    }
    onUpdate_MergeInfo();
}
void MAU::onUpdate_MergeInfo() {
    if (m_secondAnimFrames != -1) {
        ui->labelMergeSecondInfo->setText(QString("<html><head/><body><p><span style=\"font-weight:700;\">[2nd anim]</span>: %1. <span style=\"font-weight:700;\">Bones</span>: %2. <span style=\"font-weight:700;\">Events</span>: %3. <span style=\"font-weight:700;\">Duration</span>: %4 frames (%5 s), <span style=\"color:%6;\">RootMotion</span>, <span style=\"color:%7;\">motionExtraction</span>.</p></body></html>")
                                          .arg(m_secondAnimName)
                                          .arg(m_secondAnimBones)
                                          .arg(m_secondAnimEvents)
                                          .arg(m_secondAnimFrames)
                                          .arg(m_secondAnimDuration, 0, 'f', 3)
                                          .arg(m_secondAnimRootMotion ? "#2a7e3e" : "#ff0000")
                                          .arg(m_secondAnimMotionExtraction ? "#2a7e3e" : "#ff0000") );
    } else
        ui->labelMergeSecondInfo->setText(QString("<html><head/><body><p><span style=\"font-weight:700;\">[2nd anim]</span>: -</p></body></html>"));

    if (m_framesCount == -1 || m_secondAnimFrames == -1) {
        ui->buttonMergeDo->setEnabled(false);
    } else {
        onChanged_MergeStart();
        ui->buttonMergeDo->setEnabled(true);
    }
}
void MAU::onClicked_MergePictureHelp() {
    QPixmap pic(":/Merge_scheme.jpg");
    pLabelInfo->setPixmap(pic);
    pLabelInfo->show();
}
void MAU::onClicked_BlendHalfSec() {
    ui->spinMergeStart->setValue( qMax(1, m_animFrames[m_animIndex] - 15) );
    ui->spinMergeDuration->setValue( qMin(ui->spinMergeDuration->maximum(), 15) );
}
void MAU::onChanged_MergeType(int index) {
    if (index == 0) {
        ui->checkMergeSumInversePos->setEnabled(false);
        ui->checkMergeSumInverseRot->setEnabled(false);
        ui->spinMergeStart->setEnabled(true);
        ui->spinMergeDuration->setEnabled(true);
        ui->buttonMergeLastHalfSec->setEnabled(true);
        ui->labelMergeNote->setText("<html><head/><body><p><span style=\" font-weight:700; color:#55007f;\">Note</span><span style=\" color:#55007f;\">: Merged = 1st + blend(1st..2nd) + 2nd.<br/></span><span style=\" font-style:italic; color:#000000;\">Will be saved in a separate file.</span></p></body></html>");
    } else {
        ui->checkMergeSumInversePos->setEnabled(true);
        ui->checkMergeSumInverseRot->setEnabled(true);
        ui->spinMergeStart->setEnabled(false);
        ui->spinMergeDuration->setEnabled(false);
        ui->buttonMergeLastHalfSec->setEnabled(false);
    }
    if (index == 1) {
        ui->checkMergeRemoveOrigin->setEnabled(true);
        ui->checkMergeCropToSecond->setEnabled(true);
        ui->labelMergeNote->setText("<html><head/><body><p><span style=\" font-weight:700; color:#55007f;\">Note</span><span style=\" color:#55007f;\">: Merged =  1st (pose)+ 2nd (additive).<br/></span><span style=\" font-style:italic; color:#000000;\">Will be saved in a separate file.</span></p></body></html>");
    } else {
        ui->checkMergeRemoveOrigin->setEnabled(false);
        ui->checkMergeCropToSecond->setEnabled(false);
    }
    if (index == 2) {
        ui->checkMergeEventsSort->setEnabled(false);
        ui->labelMergeNote->setText("<html><head/><body><p><span style=\" font-weight:700; color:#590086;\">Note</span><span style=\" color:#590086;\">: Additive</span><span style=\" color:#590086; vertical-align:sub;\">edited</span><span style=\" color:#590086;\"> = 2nd (merged</span><span style=\" color:#590086; vertical-align:sub;\">edited</span><span style=\" color:#590086;\">) - 1st (pose).</span></p><p><span style=\" font-style:italic;\">Will be saved in a separate file.</span></p></body></html>");
        ui->comboMergeEvents->setEnabled(false);
    } else {
        ui->checkMergeEventsSort->setEnabled(true);
        ui->comboMergeEvents->setEnabled(true);
    }
}
void MAU::onClicked_MergeProcess() {
    int framesF = m_framesCount;
    int framesS = m_secondAnimFrames;
    int mergeStart = ui->spinMergeStart->value();
    int mergeDuration = ui->spinMergeDuration->value();

    bool isBlend = ui->comboMergeType->currentIndex() == 0;
    bool isSum = ui->comboMergeType->currentIndex() == 1;
    bool isSubtract = ui->comboMergeType->currentIndex() == 2;
    int framesTotal = 0;
    if (isSum) {
        if (framesF < framesS) {
            addLog(QString("[ERROR] Can't Sum: 1st anim (base pose) has less frames (%1) than 2nd (%2).").arg(framesF).arg(framesS), logError);
            return;
        }
        framesTotal = framesF;
        if (ui->checkMergeCropToSecond->isChecked()) {
            framesTotal = framesS;
        }
    } else if (isSubtract) {
        if (framesF < framesS) {
            addLog(QString("[ERROR] Can't Subtract: 1st anim (base pose) has less frames (%1) than 2nd (%2).").arg(framesF).arg(framesS), logError);
            return;
        }
        framesTotal = framesS;
    } else {
        framesTotal = qMax(mergeStart + framesS - 1, framesF);
    }

    //int currentFrame = ui->spinMergeStart->value();
    qDebug() << QString("Start merge: [%1] + [%2] -> [%3] (merge start at %4, duration %5)").arg(framesF).arg(framesS).arg(framesTotal).arg(mergeStart).arg(mergeDuration);

    QJsonObject animObjF = QJsonObject();
    QJsonArray eventsArrayF = QJsonArray();
    if (animSet) {
        QJsonArray animsObj = jsonRoot.value("animations").toArray();
        QJsonObject animRootObjF = animsObj.at(m_animIndex).toObject();
        animObjF = animRootObjF.value("animation").toObject();
        eventsArrayF = animRootObjF.value("entries").toArray();
    } else {
        animObjF = jsonRoot.value("animation").toObject();
        eventsArrayF = jsonRoot.value("entries").toArray();
    }
    editBakeBones(animObjF, true, true, false);
    QJsonObject animBuffF = animObjF.value("animBuffer").toObject();
    QJsonArray animBonesF = animBuffF.value("bones").toArray();

    QJsonObject animObjS = jsonRootMerge.value("animation").toObject();
    QJsonArray eventsArrayS = jsonRootMerge.value("entries").toArray();
    editBakeBones(animObjS, true, true, false);
    QJsonObject animBuffS = animObjS.value("animBuffer").toObject();
    QJsonArray animBonesS = animBuffS.value("bones").toArray();
    QMap<QString, QJsonObject> boneByNameF, boneByNameS;

    for (int i = 0; i < animBonesS.count(); i += 1) {
        QString boneNameS = animBonesS[i].toObject().value("BoneName").toString();
        boneByNameS[boneNameS] = animBonesS[i].toObject();
    }
    for (int i = 0; i < animBonesF.count(); i += 1) {
        QString boneNameF = animBonesF[i].toObject().value("BoneName").toString();
        boneByNameF[boneNameF] = animBonesF[i].toObject();
    }
    if (!boneByNameF.contains("RootMotion")) {
        editAddEmptyBoneFrames(animBonesF, "RootMotion", framesF);
        boneByNameF["RootMotion"] = animBonesF.last().toObject();
    }
    if (!boneByNameS.contains("RootMotion")) {
        editAddEmptyBoneFrames(animBonesS, "RootMotion", framesS);
        boneByNameS["RootMotion"] = animBonesS.last().toObject();
    }

    for (int i = 0; i < animBonesF.count(); i += 1) {
        QString boneNameF = animBonesF[i].toObject().value("BoneName").toString();
        if ( !boneByNameS.contains(boneNameF) ) {
            addLog(QString("Can't find bone %1 in source anim!").arg(boneNameF), logError);
            continue;
        }
    }

    // for every bone
    for (int j = 0; j < animBonesF.count(); j += 1) {
        QString boneNameF = animBonesF[j].toObject().value("BoneName").toString();

        QJsonObject boneF = animBonesF[j].toObject();
        QJsonObject boneS = boneByNameS[boneNameF];

        QJsonArray posArrF = boneF.value("positionFrames").toArray();
        QJsonArray posArrS = boneS.value("positionFrames").toArray();
        QJsonArray rotArrF = boneF.value("rotationFrames").toArray();
        QJsonArray rotArrS = boneS.value("rotationFrames").toArray();
        double pX1, pY1, pZ1, pW1;
        double pX2, pY2, pZ2, pW2;
        double pXa, pYa, pZa, pWa;

        if (ui->comboMergeType->currentIndex() == 0 /* BLEND */) {
            // do not change [1; mergeStart - 1]
            // interpolate [mergeStart; mergeStart + mergeDuration - 1]
            double K = 0;
            double incrK = 1.0 / mergeDuration;
            for (int frame = mergeStart; frame < mergeStart + mergeDuration - 1; frame += 1) {
                K += incrK;
                qDebug() << QString("bone %1, frame %2, interpol: K = %3").arg(boneNameF).arg(frame).arg(K);
                /* pos */
                objToXYZ(posArrF.at(frame - 1).toObject(), pX1, pY1, pZ1);
                objToXYZ(posArrS.at(frame - mergeStart).toObject(), pX2, pY2, pZ2);
                //qDebug() << QString("[pos] X1: %1, X2: %2, Y1: %3, Y2: %4, Z1: %5, Z2: %6").arg(pX1).arg(pX2).arg(pY1).arg(pY2).arg(pZ1).arg(pZ2);
                interpolatePos(K, pX1, pY1, pZ1, pX2, pY2, pZ2);
                posArrF[frame - 1] = objXYZ(pX1, pY1, pZ1);

                /* rot */
                objToXYZW(rotArrF.at(frame - 1).toObject(), pX1, pY1, pZ1, pW1);
                objToXYZW(rotArrS.at(frame - mergeStart).toObject(), pX2, pY2, pZ2, pW2);
                interpolateRot(K, pX1, pY1, pZ1, pW1, pX2, pY2, pZ2, pW2);
                rotArrF[frame - 1] = objXYZW(pX1, pY1, pZ1, pW1);
            }
            // the last one (frame == mergeStart + mergeDuration - 1): K = 1.0 vvv
            // add remaining SecondAnim frames (if they exist)
            for (int frame = mergeStart + mergeDuration - 1; frame < mergeStart + posArrS.count(); frame += 1) {
                qDebug() << QString("bone %1, frame %2, remaining (K = 1.0)").arg(boneNameF).arg(frame);
                if (posArrF.count() < frame)
                    posArrF.append( posArrS[frame - mergeStart] );
                else
                    posArrF[frame - 1] = posArrS[frame - mergeStart];

                if (rotArrF.count() < frame)
                    rotArrF.append( rotArrS[frame - mergeStart] );
               else
                    rotArrF[frame - 1] = rotArrS[frame - mergeStart];
            }
        } else if (ui->comboMergeType->currentIndex() == 1 /* SUM */) {
            // SUM [1; framesS] doing stuff in F
            for (int frame = 1; frame <= framesS; frame += 1) {
                qDebug() << QString("SUM: bone %1, frame %2").arg(boneNameF).arg(frame);
                /* pos */
                objToXYZ(posArrF.at(frame - 1).toObject(), pX1, pY1, pZ1);
                objToXYZ(posArrS.at(frame - 1).toObject(), pX2, pY2, pZ2);
                objToXYZ(posArrS.at(0).toObject(), pXa, pYa, pZa);
                //qDebug() << QString("[pos] X1: %1, X2: %2, Y1: %3, Y2: %4, Z1: %5, Z2: %6").arg(pX1).arg(pX2).arg(pY1).arg(pY2).arg(pZ1).arg(pZ2);
                if (ui->checkMergeRemoveOrigin->isChecked()) {
                    pX2 -= pXa;
                    pY2 -= pYa;
                    pZ2 -= pZa;
                }

                if (ui->checkMergeSumInversePos->isChecked()) {
                    pX1 -= pX2;
                    pY1 -= pY2;
                    pZ1 -= pZ2;
                } else {
                    pX1 += pX2;
                    pY1 += pY2;
                    pZ1 += pZ2;
                }
                posArrF[frame - 1] = objXYZ(pX1, pY1, pZ1);

                /* rot */
                objToXYZW(rotArrF.at(frame - 1).toObject(), pX1, pY1, pZ1, pW1);
                objToXYZW(rotArrS.at(frame - 1).toObject(), pX2, pY2, pZ2, pW2);

                QVector3D eulerPose = QQuaternion(pW1, pX1, pY1, pZ1).toEulerAngles();
                QVector3D eulerAdd = QQuaternion(pW2, pX2, pY2, pZ2).toEulerAngles();

                /*if (frame == mergeStart + mergeDuration - 2) {
                    qDebug() << QString("bone %1: newRot = %2, oldRot = %3, addRot = %4, originRot = %5")
                                .arg(boneNameF)
                                .arg(vec3ToString(eulerOld + eulerAdd - eulerAddOrigin))
                                .arg(vec3ToString(eulerOld))
                                .arg(vec3ToString(euler2))
                                .arg(vec3ToString(eulerA))
                                ;
                }*/
                //QQuaternion add = QQuaternion(pWa, pXa, pYa, pZa) * QQuaternion(pW2, pX2, pY2, pZ2).inverted();
                //QQuaternion sumQ = QQuaternion(pW1, pX1, pY1, pZ1) * QQuaternion(pW2, pX2, pY2, pZ2);

                if (ui->checkMergeRemoveOrigin->isChecked()) {
                    objToXYZW(rotArrS.at(0).toObject(), pXa, pYa, pZa, pWa);
                    QVector3D eulerAddOrigin = QQuaternion(pWa, pXa, pYa, pZa).toEulerAngles();
                    if (ui->checkMergeSumInverseRot->isChecked())
                        eulerAdd += eulerAddOrigin;
                    else
                        eulerAdd -= eulerAddOrigin;
                }
                QQuaternion sumQ;
                if (ui->checkMergeSumInverseRot->isChecked())
                    sumQ = QQuaternion::fromEulerAngles(eulerPose - eulerAdd);
                else
                    sumQ = QQuaternion::fromEulerAngles(eulerPose + eulerAdd);
                sumQ.normalize();
                rotArrF[frame - 1] = objXYZW(sumQ.x(), sumQ.y(), sumQ.z(), sumQ.scalar());
            }
            if (ui->checkMergeCropToSecond->isChecked()) {
                addLog(QString("\t[MERGE] Cropping bone %1 from %2 to %3 frames.").arg(boneNameF).arg(posArrF.count()).arg(framesS));
                while (posArrF.count() > framesS) {
                    posArrF.pop_back();
                }
                while (rotArrF.count() > framesS) {
                    rotArrF.pop_back();
                }
                qDebug() << "pop to: " << framesS;
            } else {
                addLog(QString("\t[MERGE] Cropping option is disabled, leaving %1 frames.").arg(posArrF.count()));
            }
        } else if (ui->comboMergeType->currentIndex() == 2 /* SUBTRACT */) {
            // SUBTRACT [1; framesS], doing stuff in S
            for (int frame = 1; frame <= framesS; frame += 1) {
                qDebug() << QString("SUBTRACT: bone %1, frame %2").arg(boneNameF).arg(frame);
                /* pos */
                objToXYZ(posArrF.at(frame - 1).toObject(), pX1, pY1, pZ1);
                objToXYZ(posArrS.at(frame - 1).toObject(), pX2, pY2, pZ2);

                if (ui->checkMergeSumInversePos->isChecked()) {
                    pX2 += pX1;
                    pY2 += pY1;
                    pZ2 += pZ1;
                } else {
                    pX2 -= pX1;
                    pY2 -= pY1;
                    pZ2 -= pZ1;
                }

                posArrS[frame - 1] = objXYZ(pX2, pY2, pZ2);

                /* rot */
                objToXYZW(rotArrF.at(frame - 1).toObject(), pX1, pY1, pZ1, pW1);
                objToXYZW(rotArrS.at(frame - 1).toObject(), pX2, pY2, pZ2, pW2);

                QVector3D eulerMerged = QQuaternion(pW2, pX2, pY2, pZ2).toEulerAngles();
                QVector3D eulerPose = QQuaternion(pW1, pX1, pY1, pZ1).toEulerAngles();

                QQuaternion subQ;
                if (ui->checkMergeSumInverseRot->isChecked())
                    subQ = QQuaternion::fromEulerAngles(eulerMerged + eulerPose);
                else
                    subQ = QQuaternion::fromEulerAngles(eulerMerged - eulerPose);
                subQ.normalize();
                rotArrS[frame - 1] = objXYZW(subQ.x(), subQ.y(), subQ.z(), subQ.scalar());
            }
            posArrF = posArrS;
            rotArrF = rotArrS;
        }
        // else do nothing

        if (framesTotal != posArrF.count() || framesTotal != rotArrF.count()) {
            addLog(QString("!!! Inconsistence! bone = %1, framesTotal = %2, posArr = %3, rotArr = %4").arg(boneNameF).arg(framesTotal).arg(posArrF.count()).arg(rotArrF.count()), logError);
        }
        boneF["position_numFrames"] = framesTotal;
        boneF["rotation_numFrames"] = framesTotal;
        boneF["positionFrames"] = posArrF;
        boneF["rotationFrames"] = rotArrF;
        animBonesF[j] = boneF;
    }

    /* events */
    if (isBlend) {
        double mergeStartTime = framesToSec(mergeStart - 1);
        if (ui->comboMergeEvents->currentIndex() == 0) {
            // erase
            eventsArrayF = QJsonArray();
        } else if (ui->comboMergeEvents->currentIndex() == 1) {
            // leave 1st
        } else if (ui->comboMergeEvents->currentIndex() == 2) {
            // leave 2nd
            eventsArrayF = eventsArrayS;
            upn(j, 0, eventsArrayF.count() - 1) {
                QJsonObject eventObj = eventsArrayF[j].toObject();
                double startTime = getEventStartTime(eventObj);
                setEventStartTime(eventObj, startTime + mergeStartTime);
                eventsArrayF[j] = eventObj;
            }
        } else if (ui->comboMergeEvents->currentIndex() == 3)  {
            // append 2nd
            upn(j, 0, eventsArrayS.count() - 1) {
                QJsonObject eventObj = eventsArrayF[j].toObject();
                double startTime = getEventStartTime(eventObj);
                setEventStartTime(eventObj, startTime + mergeStartTime);
                eventsArrayF.append( eventObj );
            }
        }
    }

    if (ui->checkMergeEventsSort->isChecked()) {
        if (isSubtract) {
            editSortEvents(eventsArrayS);
            eventsArrayF = eventsArrayS;
        } else {
            editSortEvents(eventsArrayF);
        }
    }

    // save
    animBuffF["bones"] = animBonesF;
    animBuffF["numFrames"] = framesTotal;
    animBuffF["duration"] = framesToSec(framesTotal - 1);
    if (!isBlend)
        animObjF["name"] = m_secondAnimName;
    animObjF["duration"] = framesToSec(framesTotal - 1);
    animObjF["animBuffer"] = animBuffF;

    addLog(QString("\t[MERGE] Resulting anim: numFrames = %1, duration = %2 s, name = %3").arg(framesTotal).arg(framesToSec(framesTotal - 1)).arg(animObjF["name"].toString()));

    /* optimize */
    if (ui->checkMergeOptimize->isChecked()) {
        editOptimizeBones(animObjF, true, true, true);
    }

    QJsonObject mergedJsonRoot = QJsonObject();
    mergedJsonRoot.insert("animation", animObjF);
    mergedJsonRoot.insert("entries", eventsArrayF);

    if (isBlend)
        saveMergedJson(mergedJsonRoot, "BLEND");
    else if (isSum)
        saveMergedJson(mergedJsonRoot, "MERGE");
    else
        saveMergedJson(mergedJsonRoot, "SUBTRACT");
    addLog("[OK] Merge done.", logFinish);
}

MAU::~MAU()
{
    delete ui;
}
