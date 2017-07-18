/* --------------------------------------------------------------------------------
 * Example tests for QtPropertySerializer.
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#ifndef __test_QtPropertySerializer_H__
#define __test_QtPropertySerializer_H__

#include <QDate>
#include <QObject>
#include <QString>

class Person : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int height MEMBER heightInCm)
    Q_PROPERTY(QDate dob MEMBER dateOfBirth)
    
public:
    int heightInCm;
    QDate dateOfBirth;
    
    // Members that are NOT properties NOR children are NOT serialized.
    QString nickName; // NOT a property OR a child object.
    QObject something; // Non-child member object.
    
    Person(const QString &name = "") { setObjectName(name); }
};

class Pet : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString species MEMBER species)
    
public:
    QString species;
    
    Pet(const QString &name = "") { setObjectName(name); }
};

#endif // __test_QtPropertySerializer_H__
