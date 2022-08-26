#include "QResizableStackedWidget.h"

QResizableStackedWidget::QResizableStackedWidget(QWidget *parent)
    : QStackedWidget(parent)
{
    connect(this, SIGNAL(currentChanged(int)), SLOT(onCurrentChanged(int)));
}

void QResizableStackedWidget::addWidget(QWidget* pWidget)
{
   pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
   QStackedWidget::addWidget(pWidget);
}

void QResizableStackedWidget::onCurrentChanged(int index)
{
   if (m_oldIndex != -1) {
       QWidget* pOldWidget = widget(m_oldIndex);
       Q_ASSERT(pOldWidget);
       pOldWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
   }
   m_oldIndex = index;
   QWidget* pWidget = widget(index);
   Q_ASSERT(pWidget);
   pWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
   pWidget->adjustSize();
   adjustSize();
}
