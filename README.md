# ObjectSerializerQt

Simple serialization for arbitrary C++ classes derived from **QObject** to/from **XML** or **QVariantMap**.

* Serializes both object properties AND child objects (recursively).
* Allows dynamic *QObject* creation at runtime during deserialization via an object creation factory.
* !!! Class members NOT registered as properties or NOT in the *QObject's* list of children are NOT serialized. Also, only both readable AND writable properties are serialized (optional inclusion of read only properties).
* XML properties can be child nodes OR node attributes.

**Author**: Marcel Goldschen-Ohm  
**Email**:  <marcel.goldschen@gmail.com>  
**License**: MIT  
Copyright (c) 2015 Marcel Goldschen-Ohm  

## Alternatives

For JSON serialization, check out [qjson](https://github.com/flavio/qjson).

## INSTALL

Header only. Just put these in your include path:

* `ObjectSerializerQt.h`
* `XmlObjectSerializerQt.h`

### Requires:

* [Qt](http://www.qt.io) (tested with version 5, but probably will work with version 4 as well - let me know if it doesn't).

## Example XML serialization.

Under the hood, *XmlObjectSerializerQt* uses *ObjectSerializerQt* to serialize the *QObject* tree to a *QVariantMap*, then generates XML from the *QVariantMap*.

```cpp
#include <assert.h>
#include "XmlObjectSerializerQt.h"
#include "Person.h" // See below examples.

// Used for runtime deserialization of
// dynamic person object trees.
QObject* createPerson() { return new Person(); }

int main(int argc, char **argv) {
    // Jane is the root of our family tree.
    Person jane("Jane");
    jane.heightInCm = 170;
    jane.dateOfBirth = QDate(1969, 7, 20);

    // Jane's child John.
    Person *john = new Person("John");
    john->setParent(&jane);
    john->heightInCm = 190;
    john->dateOfBirth = QDate(1995, 5, 20);

    // Jane's child John.
    Person *josephine = new Person("Josephine");
    josephine->setParent(&jane);
    josephine->heightInCm = 50;
    josephine->dateOfBirth = QDate(2000, 12, 25);

    // Save Jane's family tree to XML ==> See jane.xml below.
    XmlObjectSerializerQt::saveXml(&jane, "jane.xml");

    // Save to XML with properties as attributes ==> See jane_attr.xml below.
    QStringList attributes;
    attributes << "heightInCm" << "dateOfBirth";
    XmlObjectSerializerQt::saveXml(&jane, "jane_attr.xml", attributes);

    // Load Jane's family tree from XML.
    XmlObjectSerializerQt::loadXml(&jane, "jane.xml");
    assert(jane.heightInCm == 170);
    assert(jane.dateOfBirth == QDate(1969, 7, 20));
    assert(john->heightInCm == 190);
    assert(john->dateOfBirth == QDate(1995, 5, 20));
    assert(josephine->heightInCm == 50);
    assert(josephine->dateOfBirth == QDate(2000, 12, 25));

    // Runtime creation of new family tree from XML.
    // This requires a factory that creates "Jane", "John",
    // and "Josephine" Person objects.
    ObjectSerializerQt::ObjectFactory personFactory;
    personFactory.registerCreator("Jane", createPerson);
    personFactory.registerCreator("John", createPerson);
    personFactory.registerCreator("Josephine", createPerson);
    QObject *familyTree = XmlObjectSerializerQt::loadXml("jane.xml", personFactory);
    assert(familyTree);
    Person *janeCopy = (Person*) familyTree;
    assert(janeCopy);
    Person *johnCopy = janeCopy->findChild<Person*>("John");
    Person *josephineCopy = janeCopy->findChild<Person*>("Josephine");
    assert(johnCopy);
    assert(josephineCopy);
    assert(janeCopy->heightInCm == jane.heightInCm);
    assert(janeCopy->dateOfBirth == jane.dateOfBirth);
    assert(johnCopy->heightInCm == john->heightInCm);
    assert(johnCopy->dateOfBirth == john->dateOfBirth);
    assert(josephineCopy->heightInCm == josephine->heightInCm);
    assert(josephineCopy->dateOfBirth == josephine->dateOfBirth);
    delete familyTree; // Cleanup runtime allocated object tree.

    return 0;
}
```

**jane.xml**:

```xml
<Jane>
 <John>
  <dateOfBirth>1995-05-20</dateOfBirth>
  <heightInCm>190</heightInCm>
 </John>
 <Josephine>
  <dateOfBirth>2000-12-25</dateOfBirth>
  <heightInCm>50</heightInCm>
 </Josephine>
 <dateOfBirth>1969-07-20</dateOfBirth>
 <heightInCm>170</heightInCm>
</Jane>
```

**jane_attr.xml**:

```xml
<Jane dateOfBirth="1969-07-20" heightInCm="170">
 <John dateOfBirth="1995-05-20" heightInCm="190"/>
 <Josephine dateOfBirth="2000-12-25" heightInCm="50"/>
</Jane>
```

## Example QVariantMap serialization.

```cpp
#include <assert.h>
#include "ObjectSerializerQt.h"
#include "Person.h" // See below examples.

// Used for runtime deserialization of
// dynamic person object trees.
QObject* createPerson() { return new Person(); }

int main(int argc, char **argv) {
    // Jane is the root of our family tree.
    Person jane("Jane");
    jane.heightInCm = 170;
    jane.dateOfBirth = QDate(1969, 7, 20);

    // Jane's child John.
    Person *john = new Person("John");
    john->setParent(&jane);
    john->heightInCm = 190;
    john->dateOfBirth = QDate(1995, 5, 20);

    // Serialize data for Jane's family tree.
    QVariantMap janeData = ObjectSerializerQt::serialize(&jane);
    QVariantMap johnData = janeData["John"].toMap();
    assert(janeData["heightInCm"].toInt() == 170);
    assert(janeData["dateOfBirth"].toDate() == QDate(1969, 7, 20));
    assert(johnData["heightInCm"].toInt() == 190);
    assert(johnData["dateOfBirth"].toDate() == QDate(1995, 5, 20));
    // Note that class members that are NOT properties
    // and NOT children are NOT serialized.
    assert(!janeData.contains("nickName"));
    assert(!johnData.contains("nickName"));
    // Also, ONLY properties that are BOTH readable
    // AND writable are serilized. (READ ONLY properties
    // can be serialized if the includeReadOnlyProperties
    // flag is set to true - default is false).
    assert(!janeData.contains("readOnlyName"));
    assert(!johnData.contains("readOnlyName"));

    // John grew a bit taller.
    johnData["heightInCm"] = 200;
    ObjectSerializerQt::deserialize(john, johnData);
    assert(john->heightInCm == 200);

    // John grew even taller.
    johnData["heightInCm"] = 210;
    janeData["John"] = johnData; // Need to reinsert updated data.
    ObjectSerializerQt::deserialize(&jane, janeData);
    assert(john->heightInCm == 210);

    // Jane had another child Josephine.
    QVariantMap josephineData;
    josephineData["heightInCm"] = 50;
    josephineData["dateOfBirth"] = QDate(2000, 12, 25);
    janeData["Josephine"] = josephineData;
    // Deserialization of janeData into Jane's object tree
    // now requires a factory that can create a Josephine
    // object at runtime.
    ObjectSerializerQt::ObjectFactory personFactory;
    personFactory.registerCreator("Josephine", createPerson);
    ObjectSerializerQt::deserialize(&jane, janeData, personFactory);
    Person *josephine = jane.findChild<Person*>("Josephine");
    assert(josephine);
    assert(josephine->heightInCm == 50);
    assert(josephine->dateOfBirth == QDate(2000, 12, 25));

    return 0;
}
```

## Person.h - used in examples above.

```cpp
#include <QDate>
#include <QObject>
#include <QString>

class Person : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString readOnlyName READ objectName)
    Q_PROPERTY(int heightInCm MEMBER heightInCm)
    Q_PROPERTY(QDate dateOfBirth MEMBER dateOfBirth)

public:
    QString nickName; // NOT a property OR a child object.
    int heightInCm;
    QDate dateOfBirth;

    Person(const QString &name="") {
        setObjectName(name);
        nickName = "NICK NAME";
        heightInCm = 100 + 100 * int(float(rand()) / RAND_MAX);
        dateOfBirth = QDate::currentDate();
    }
};
```
