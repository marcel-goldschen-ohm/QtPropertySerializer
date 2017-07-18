/* --------------------------------------------------------------------------------
 * Example tests for QtObjectPropertySerializer.
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#include "QtPropertySerializerTest.h"

#include <assert.h>
#include <iostream>

#include "QtPropertySerializer.h"

int main(int, char **)
{
    std::cout << "Running tests for QtPropertySerializer..." << std::endl;
    
    // Jane is the root of our family tree.
    Person jane("Jane");
    jane.heightInCm = 170;
    jane.dateOfBirth = QDate(1969, 7, 20);
    
    // Jane's child John.
    Person *john = new Person("John");
    john->setParent(&jane);
    john->heightInCm = 190;
    john->dateOfBirth = QDate(1995, 5, 20);
    
    // Jane's child Josephine.
    Person *josephine = new Person("Josephine");
    josephine->setParent(&jane);
    josephine->heightInCm = 50;
    josephine->dateOfBirth = QDate(2000, 12, 25);
    
    // Josephine's pet dog Spot.
    Pet *spot = new Pet("Spot");
    spot->setParent(josephine);
    spot->species = "dog";
    spot->setProperty("vaccinated", true); // Dynamic property.
    
    //-------------------------
    // QObject --> QVariantMap
    //-------------------------
    
    std::cout << "Checking serialization from QObject to QVariantMap... ";
    
    // Get Jane's property tree.
    QVariantMap janeData = QtPropertySerializer::serialize(&jane);
    
    // Map keys for properties are the property names.
    // Map keys for child objects are the child object class names.
    assert(janeData["objectName"].toString() == jane.objectName());
    assert(janeData["height"].toInt() == jane.heightInCm);
    assert(janeData["dob"].toDate() == jane.dateOfBirth);
    // janeData["Person"] is a QVariantList containing QVariantMaps for John and Josephine.
    // If Jane had only one child, then janeData["Person"] would instead
    // be a QVariantMap for the only child (not a QVariantList of QVariantMaps)
    // analogous to josephineData["Pet"] below.
    QVariantList janePersonList = janeData["Person"].toList();
    QVariantMap johnData = janePersonList[0].toMap();
    QVariantMap josephineData = janePersonList[1].toMap();
    assert(johnData["objectName"].toString() == john->objectName());
    assert(johnData["height"].toInt() == john->heightInCm);
    assert(johnData["dob"].toDate() == john->dateOfBirth);
    assert(josephineData["objectName"].toString() == josephine->objectName());
    assert(josephineData["height"].toInt() == josephine->heightInCm);
    assert(josephineData["dob"].toDate() == josephine->dateOfBirth);
    // josephineData["Pet"] is a QVariantMap for Spot.
    // If Josephine had two or more pets, then josephineData["Pet"] would instead
    // be a QVariantList of QVariantMaps for each pet analogous to janeData["Person"].
    QVariantMap spotData = josephineData["Pet"].toMap();
    assert(spotData["objectName"].toString() == spot->objectName());
    assert(spotData["species"].toString() == spot->species);
    assert(spotData["vaccinated"].toBool() == spot->property("vaccinated").toBool());
    
    std::cout << "OK" << std::endl;
    
    //-------------------------
    // QVarinatMap --> QObject
    //-------------------------
    
    std::cout << "Checking deserialization from QVariantMap into QObject with preallocated tree... ";
    
    // Alter Jane's property tree and then reload it from janeData.
    jane.heightInCm = 0;
    john->heightInCm = 0;
    josephine->heightInCm = 0;
    spot->species = "cat";
    
    QtPropertySerializer::deserialize(&jane, janeData);
    
    assert(jane.heightInCm == janeData["height"]);
    assert(john->heightInCm == johnData["height"]);
    assert(josephine->heightInCm == josephineData["height"]);
    assert(spot->species == spotData["species"]);
    
    std::cout << "OK" << std::endl;
    
    std::cout << "Checking deserialization from QVariantMap into QObject without preallocated tree... ";
    
    // Try and load Jane's property tree into a new object without preexisting children.
    // This will fail to deserialize the children.
    Person bizarroJane;
    QtPropertySerializer::deserialize(&bizarroJane, janeData);
    
    // Bizarro Jane should have Jane's properties, but NO children.
    assert(bizarroJane.objectName() == jane.objectName());
    assert(bizarroJane.heightInCm == jane.heightInCm);
    assert(bizarroJane.dateOfBirth == jane.dateOfBirth);
    assert(bizarroJane.children().size() == 0);
    
    std::cout << "OK" << std::endl;
    
    std::cout << "Checking deserialization from QVariantMap into QObject without preallocated tree... ";
    
    // Use a factory for dynamic creation of Person objects
    // and try again to deserialize janeData into bizzaro Jane.
    QtPropertySerializer::ObjectFactory factory;
    factory.registerCreator("Person", factory.defaultCreator<Person>);
    factory.registerCreator("Pet", factory.defaultCreator<Pet>);
    QtPropertySerializer::deserialize(&bizarroJane, janeData, &factory);
    
    // Bizzaro Jane should now be identical to Jane.
    assert(bizarroJane.children().size() == 2);
    Person *bizarroJohn = qobject_cast<Person*>(bizarroJane.findChild<Person*>("John"));
    Person *bizarroJosephine = qobject_cast<Person*>(bizarroJane.findChild<Person*>("Josephine"));
    Pet *bizarroSpot = qobject_cast<Pet*>(bizarroJosephine->findChild<Pet*>("Spot"));
    assert(bizarroJane.objectName() == jane.objectName());
    assert(bizarroJane.heightInCm == jane.heightInCm);
    assert(bizarroJane.dateOfBirth == jane.dateOfBirth);
    assert(bizarroJohn->objectName() == john->objectName());
    assert(bizarroJohn->heightInCm == john->heightInCm);
    assert(bizarroJohn->dateOfBirth == john->dateOfBirth);
    assert(bizarroJosephine->objectName() == josephine->objectName());
    assert(bizarroJosephine->heightInCm == josephine->heightInCm);
    assert(bizarroJosephine->dateOfBirth == josephine->dateOfBirth);
    assert(bizarroSpot->objectName() == spot->objectName());
    assert(bizarroSpot->species == spot->species);
    assert(bizarroSpot->property("vaccinated").toBool() == spot->property("vaccinated").toBool());
    
    std::cout << "OK" << std::endl;
    
    //------------------------
    // QObject <--> JSON file
    //------------------------
    
    std::cout << "Checking serialization/deserialization to/from JSON file... ";
    
    // These convert to/from QVariantMap under the hood.
    QtPropertySerializer::writeJson(&jane, "jane.json");
    QtPropertySerializer::readJson(&jane, "jane.json", &factory);
    
    std::cout << "OK" << std::endl;
    
    return 0;
}
