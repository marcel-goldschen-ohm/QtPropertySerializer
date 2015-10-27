// Author: Marcel Paz Goldschen-Ohm
// Email: marcel.goldschen@gmail.com

#include <assert.h>
#include "XmlObjectSerializerQt.h"
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
