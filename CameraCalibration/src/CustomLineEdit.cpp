#include "CustomLineEdit.h"

void CustomLineEdit::focusInEvent(QFocusEvent *event)
{
    m_originalValue = text();
    QLineEdit::focusOutEvent(event);
}

void CustomLineEdit::focusOutEvent(QFocusEvent *event)
{
    QLineEdit::focusOutEvent(event);
    if (!hasAcceptableInput())
    {
        setText(m_originalValue);
        setFocus();
        emit validationError();
    }
}


