#ifndef QRESIZABLESTACKEDWIDGET_H
#define QRESIZABLESTACKEDWIDGET_H

#include <QStackedWidget>

class QResizableStackedWidget : public QStackedWidget
{
    Q_OBJECT
private:
    int m_oldIndex = -1;
public:
    explicit QResizableStackedWidget(QWidget *parent = nullptr);


    void addWidget(QWidget* pWidget);
signals:

public slots:
    void onCurrentChanged(int index);
};

#endif // QRESIZABLESTACKEDWIDGET_H
