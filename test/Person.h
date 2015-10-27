// Author: Marcel Paz Goldschen-Ohm
// Email: marcel.goldschen@gmail.com

#ifndef __Person_H__
#define __Person_H__

#include <QDate>
#include <QObject>
#include <QString>

class Person : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString readOnlyName READ objectName)
    Q_PROPERTY(int heightInCm MEMBER heightInCm)
    Q_PROPERTY(QDate dateOfBirth MEMBER dateOfBirth)

public:
    QString nickName;
    int heightInCm;
    QDate dateOfBirth;

    Person(const QString &name="") {
        setObjectName(name);
        nickName = "NICK NAME";
        heightInCm = 100 + 100 * int(float(rand()) / RAND_MAX);
        dateOfBirth = QDate::currentDate();
    }
};

#endif
