/* --------------------------------------------------------------------------------
 * ObjectSerializerQt
 *
 * Simple serialization for arbitrary objects derived from QObject to/from a QVariantMap.
 * Serializes both object properties AND child objects (recursively).
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#ifndef __ObjectSerializerQt_H__
#define __ObjectSerializerQt_H__

#include <QMap>
#include <QMetaProperty>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#ifdef DEBUG
#include <iostream>
#include <QDebug>
#endif


/* --------------------------------------------------------------------------------
 * Simple serialization of objects derived from QObject to/from a QVariantMap.
 * Serializes both object properties AND child objects (recursively).
 *
 * e.g.
 *   QObject *object = ...
 *   QObject *child = ...
 *   child->setParent(object);
 *   child->setObjectName("my child");
 *   QVariantMap objectData = ObjectSerializerQt::serialize(object);
 *   ...
 *   QVariant value = ...
 *   QVariant anotherValue = ...
 *   objectData["some property name"] = value
 *   objectData["my child"]["a property name"] = anotherValue
 *   ...
 *   ObjectSerializerQt::deserialize(object, data);
 *   assert(object->property("some property name") == value);
 *   assert(child->property("a property name") == anotherValue);
 *
 * Map key/value pairs are either:
 * 1. Property name/QVariant pairs.
 * 2. Child objectName()/QVariantMap pairs.
 *    If objectName() does not exist, the child's meta className() will be used instead.
 * 3. name/QVariantList pairs, where list may contain both child object QVariantMaps
 *    and property QVariants associated with name.
 *
 * !!! To create dynamic objects at runtime during deserialization, we require an object factory
 *     that can create objects derived from QObject using a string ID that corresponds
 *     to the map keys for child objects.
 *     e.g.
 *       QObject *object = ...
 *       QVariantMap data = ...
 *       ObjectFactory factory = ObjectFactory();
 *       factory.registerCreator(...);
 *       deserialize(object, data, factory);
 * -------------------------------------------------------------------------------- */
class ObjectSerializerQt {
public:

//    class VariantMap : public QVariantMap {
//    public:
//        QVariantMap& operator[](const QString &key) { return QVariantMap::operator[](key).toMap(); }
//        const QVariantMap operator[](const QString &key) const { return QVariantMap::operator[](key).toMap(); }

//        QVariantList& operator[](const QString &key) { return QVariantMap::operator[](key).toList(); }
//        const QVariantList operator[](const QString &key) const { return QVariantMap::operator[](key).toList(); }
//    };

    /* --------------------------------------------------------------------------------
     * Factory for objects derived from QObject.
     *
     * Used during deserialization for dynamically creating new objects at runtime.
     *
     * !!! Currently, objects require a default constructor without arguments.
     *
     * e.g.
     *   class MyObject : public QObject { ... }
     *   QObject* myObjectCreator() { return new MyObject(); }
     *   ObjectFactory myFactory;
     *   myFactory.registerCreator("myId", myObjectCreator);
     *   QObject *obj = myFactory.create("myId");
     * -------------------------------------------------------------------------------- */
    class ObjectFactory {
    public:
        typedef QObject* (*ObjectCreatorFuncPtr)();
        typedef QMap<QString, ObjectCreatorFuncPtr> ObjectCreatorMap;

    public:
        inline bool hasCreator(const QString &id) const {
            return _objectCreatorMap.contains(id);
        }

        inline void registerCreator(const QString &id, ObjectCreatorFuncPtr creator) {
            _objectCreatorMap[id] = creator;
        }

        QObject* create(const QString &id) const {
            if(_objectCreatorMap.contains(id))
                return (*_objectCreatorMap[id])();
            return 0;
        }

    private:
        ObjectCreatorMap _objectCreatorMap;
    };


    /* --------------------------------------------------------------------------------
     * Adds name/value pair to map.
     * If map already contains name, converts existing named value to a list
     *   and appends input value to the list.
     *
     * Convenience function used during conversion of QObject or other data to a QVariantMap.
     * -------------------------------------------------------------------------------- */
    static void addMappedData(QVariantMap &data, const QString &name, const QVariant &value) {
        if(data.contains(name)) {
            QVariant &existingData = data[name];
            if(existingData.type() == QVariant::List) {
                QVariantList listOfValuesWithTheSameName = existingData.toList();
                listOfValuesWithTheSameName.append(value);
                data[name] = listOfValuesWithTheSameName;
            } else {
                QVariantList listOfValuesWithTheSameName;
                listOfValuesWithTheSameName.append(existingData);
                listOfValuesWithTheSameName.append(value);
                data[name] = listOfValuesWithTheSameName;
            }
        } else {
            data[name] = value;
        }
    }


    /* --------------------------------------------------------------------------------
     * Return an object's properties and children as a QVariantMap.
     *
     * Property name/value pairs:
     *   data[name] = QVariant(value)
     *
     * Child objects with unique objectNames:
     *   data[objectName] = QVariantMap(child serialization)
     *
     * Child objects and properties sharing the same objectName/property name:
     *   data[objectName] = QVariantList([QVariantMap(child serialization), QVariant(property value), ...])
     *
     * !!! The objectName property is reserved for map keys, and not itself included in the map.
     *
     * !!! If includeReadOnlyProperties is true, include all readable properties,
     *     otherwise include only properties that are both readable and writable.
     *
     * !!! Child map keys are either the child's objectName property if it exists (nonempty),
     *     or else the child's meta className.
     * -------------------------------------------------------------------------------- */
    static QVariantMap serialize(const QObject *object, bool includeReadOnlyProperties = false) {
        QVariantMap data;

        // Properties.
        const QMetaObject *metaObject = object->metaObject();
        for(int i = 0; i < metaObject->propertyCount(); ++i) {
            QMetaProperty metaProperty = metaObject->property(i);
            if(metaProperty.isReadable() && (includeReadOnlyProperties || metaProperty.isWritable())) {
                QString propertyName = QString::fromLatin1(metaProperty.name());
                if(propertyName != "objectName") {
                    QVariant propertyValue = object->property(propertyName.toUtf8().constData());
                    data[propertyName] = propertyValue;
                }
            }
        }

        // Children.
        for(QObjectList::const_iterator i = object->children().constBegin(); i != object->children().constEnd(); ++i) {
            QObject *child = *i;
            QString childName = (child->objectName().size() ? child->objectName() : child->metaObject()->className());
            addMappedData(data, childName, serialize(child, includeReadOnlyProperties));
        }

        return data;
    }


    /* --------------------------------------------------------------------------------
     * Set the input object's properties and children to the input QVariantMap data.
     *
     * See serialize(...) for a description of the mapped data.
     *
     * If the data map contains new properties, set them dynamically.
     * If the data map contains new child objects, attempt to create them dynamically using
     *   the input factory. In this case, factory must have a registered creator for the
     *   object's map key. If not, the child data is ignored.
     * -------------------------------------------------------------------------------- */
    static void deserialize(QObject *object, const QVariantMap &data,
                            const ObjectFactory &factory = ObjectFactory()) {
        if(!object)
            return;
        for(QVariantMap::const_iterator i = data.constBegin(); i != data.constEnd(); ++i) {
            if(i.value().type() == QVariant::Map) {
                // Child.
                // 1. Deserialize into existing child whose objectName matches the map key.
                // 2. Attempt to create a new child using the map key and supplied factory.
                const QString &childName = i.key();
                const QVariantMap &childData = i.value().toMap();
                QObject *child = object->findChild<QObject*>(childName);
                if(child)
                    deserialize(child, childData, factory);
                else if(factory.hasCreator(childName)) {
                    QObject *child = factory.create(childName);
                    child->setParent(object);
                    child->setObjectName(childName);
                    deserialize(child, childData, factory);
                }
            } else if(i.value().type() == QVariant::List) {
                // List of children (may also contain properties with the same name as children).
                const QString &childName = i.key();
                const QVariantList &childDataList = i.value().toList();
                QObjectList existingChildList = object->findChildren<QObject*>(childName);
                int count = 0;
                for(QVariantList::const_iterator j = childDataList.constBegin(); j != childDataList.constEnd(); ++j) {
                    if(j->type() == QVariant::Map) {
                        // Child.
                        // 1. Deserialize into existing child whose objectName matches the map key.
                        //    Since there are multiple matching children, we keep track of those already used.
                        // 2. Attempt to create a new child using the map key and supplied factory.
                        const QVariantMap &childData = j->toMap();
                        if(count < existingChildList.size()) {
                            deserialize(existingChildList[count], childData, factory);
                            ++count;
                        } else if(factory.hasCreator(childName)) {
                            QObject *child = factory.create(childName);
                            child->setParent(object);
                            child->setObjectName(childName);
                            deserialize(child, childData, factory);
                        }
                    } else {
                        // Property.
                        // Deserialize named property.
                        const QString &propertyName = childName;
                        const QVariant &propertyValue = *j;
                        object->setProperty(propertyName.toUtf8().constData(), propertyValue);
                    }
                }
            } else {
                // Property.
                // Deserialize named property.
                const QString &propertyName = i.key();
                const QVariant &propertyValue = i.value();
                object->setProperty(propertyName.toUtf8().constData(), propertyValue);
            }
        }
    }

};

#endif
