// Author: Marcel Paz Goldschen-Ohm
// Email: marcel.goldschen@gmail.com

#include <assert.h>
#include "ObjectSerializerQt.h"
#include "Person.h"

// Used for runtime deserialization of
// dynamic person object trees.
QObject* createPerson() {
    return new Person();
}

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

    // Serialize Jane's family tree data.
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
    // flag is set to true (default is false).
    assert(!janeData.contains("readOnlyName"));
    assert(!johnData.contains("readOnlyName"));

    // John grew a bit taller.
    johnData["heightInCm"] = 200;
    ObjectSerializerQt::deserialize(john, johnData);
    assert(john->heightInCm == 200);

    // John grew even taller.
    johnData["heightInCm"] = 210;
    janeData["John"] = johnData;
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
