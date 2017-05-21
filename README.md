# QtObjectPropertySerializer

Simple C++ serialization for a QObject property tree.

    QObject <==> QVariantMap <==> {Json, XML, ...}

* Recursively serializes child objects.
* ONLY serializes properties (ignores other class members).
* Dynamic runtime creation of child objects during deserialization (requires factory).

**Author**: Marcel Goldschen-Ohm  
**Email**:  <marcel.goldschen@gmail.com>  
**License**: MIT  
Copyright (c) 2015 Marcel Goldschen-Ohm  

## Alternatives

For JSON serialization, also check out [qjson](https://github.com/flavio/qjson).

## INSTALL

Everything is in `QtObjectPropertySerializer.h`. Just include it.

### Requires:

* [Qt](http://www.qt.io)

## QObject <==> QVariantMap

QVariantMap (key, value) pairs are either:

1. (property name, property value)
2. (child object class name, child QVariantMap)
3. (child object class name, QVariantList of QVariantMaps for multiple children of the same type)

## Quick start code snippets

See `test.h/cpp` for complete example code.

```cpp
#include "QtObjectPropertySerializer.h"

// QObject tree
class Person : public QObject { ... };
class Pet : public QObject { ... };
Person jane;
Person *john = new Person;
Person *josephine = new Person;
Pet *spot = new Pet;
john->setParent(&jane);
josephine->setParent(&jane);
spot->setParent(josephine);
// jane
// |-- john
// |-- josephine
//     |-- spot

// Serialize to QVariantMap.
QVariantMap janePropertyTree = QtObjectPropertySerializer::serialize(&jane);

// Deserialize from QVariantMap.
QtObjectPropertySerializer::deserialize(&jane, janePropertyTree);

// Deserialize (requires runtime dynamic object creation).
QtObjectPropertySerializer::ObjectFactory factory;
factory.registerCreator("Person", factory.defaultCreator<Person>);
factory.registerCreator("Pet", factory.defaultCreator<Pet>);
Person bizarroJane;
// Prior to deserialization, bizarroJane has no children.
QtObjectPropertySerializer::deserialize(&bizarroJane, janePropertyTree, &factory);
// After deserialization, bizarroJane is identical to Jane
// with children John and Josephine, and Josephine's pet Spot.

// Serialize to/from JSON file.
QtObjectPropertySerializer::writeJson(&jane, "jane.json");
QtObjectPropertySerializer::readJson(&jane, "jane.json");

// Deserialize from JSON file (dynamically create object tree).
Person juniper;
QtObjectPropertySerializer::readJson(&juniper, "jane.json", &factory);

// Access child QVariantMaps in parent QVariantMap by class name.
// Multiple children of the same type are placed in a QVariantList.
QVariantList janePersonList = janePropertyTree["Person"].toList();
QVariantMap johnPropertyTree = janePersonList[0].toMap();
QVariantMap josephinePropertyTree = janePersonList[1].toMap();
QVariantMap spotPropertyTree = josephinePropertyTree["Pet"].toMap();

// Access properties in QVariantMap by property name.
qDebug() << janePropertyTree["objectName"];
qDebug() << johnPropertyTree["objectName"];
qDebug() << josephinePropertyTree["objectName"];
qDebug() << spotPropertyTree["objectName"];
```
