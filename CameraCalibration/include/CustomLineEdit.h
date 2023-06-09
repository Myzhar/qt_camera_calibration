#pragma once

#include <QtWidgets>
#include <QString>

class CustomLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    CustomLineEdit(QWidget *parent = nullptr) : QLineEdit(parent){}
protected:
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
signals:
    void validationError();
private:
    QString m_originalValue;
};
