/* --------------------------------------------------------------------------------
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#include "QtPropertySerializer.h"

#include <QFile>
#include <QJsonDocument>
#include <QMetaObject>
#include <QMetaProperty>
#include <QTextStream>
#include <QVariantList>

namespace QtPropertySerializer
{
    QVariantMap serialize(const QObject *object, int childDepth, bool includeReadOnlyProperties)
    {
        QVariantMap data;
        if(!object)
            return data;
        // Properties.
        const QMetaObject *metaObject = object->metaObject();
        int propertyCount = metaObject->propertyCount();
        for(int i = 0; i < propertyCount; ++i) {
            const QMetaProperty metaProperty = metaObject->property(i);
            if(metaProperty.isReadable() && (includeReadOnlyProperties || metaProperty.isWritable())) {
                const QByteArray propertyName = QByteArray(metaProperty.name());
                const QVariant propertyValue = object->property(propertyName.constData());
                addMappedData(data, propertyName, propertyValue);
            }
        }
        foreach(const QByteArray &propertyName, object->dynamicPropertyNames()) {
            const QVariant propertyValue = object->property(propertyName.constData());
            addMappedData(data, propertyName, propertyValue);
        }
        // Children.
        if(childDepth == -1 || childDepth > 0) {
            if(childDepth > 0)
                --childDepth;
            foreach(QObject *child, object->children()) {
                const QByteArray className(child->metaObject()->className());
                addMappedData(data, className, serialize(child, childDepth, includeReadOnlyProperties));
            }
        }
        return data;
    }
    
    QVariantList serialize(const QList<QObject*> objects, int childDepth, bool includeReadOnlyProperties)
    {
        QVariantList data;
        for(QObject *object : objects) {
            data.append(serialize(object, childDepth, includeReadOnlyProperties));
        }
        return data;
    }
    
    void addMappedData(QVariantMap &data, const QByteArray &key, const QVariant &value)
    {
        if(value.canConvert<QObject*>()) {
            // Handle QObject* values. !!! This will be deserialized as a child object!
            QObject *object = qvariant_cast<QObject*>(value);
            addMappedData(data, key, serialize(object));
        } else if(value.canConvert<QList<QObject*> >()) {
            // Handle QList<QObject*> values. !!! These will be deserialized as child objects!
            QList<QObject*> objects = qvariant_cast<QList<QObject*> >(value);
            QVariantList values;
            for(QObject *object : objects) {
                values.append(serialize(object));
            }
            addMappedData(data, key, values);
        } else {
            // Handle all other values (i.e. QVariant, QVariantList, QVariantMap, ...).
            if(data.contains(key)) {
                // If data already contains key, make sure key's value is a list and append the input value.
                QVariant &existingData = data[key];
                if(existingData.type() == QVariant::List) {
                    QVariantList values = existingData.toList();
                    values.append(value);
                    data[key] = values;
                } else {
                    QVariantList values;
                    values.append(existingData);
                    values.append(value);
                    data[key] = values;
                }
            } else {
                data[key] = value;
            }
        }
    }
    
    void deserialize(QObject *object, const QVariantMap &data, ObjectFactory *factory)
    {
        if(!object)
            return;
        for(QVariantMap::const_iterator i = data.constBegin(); i != data.constEnd(); ++i) {
            if(i.value().type() == QVariant::Map) {
                // Child object.
                QByteArray className = i.key().toUtf8();
                const QVariantMap &childData = i.value().toMap();
                bool childFound = false;
                if(childData.contains("objectName")) {
                    // If objectName is specified for the child, find the first existing child with matching objectName and className.
                    QObjectList children = object->findChildren<QObject*>(childData.value("objectName").toString());
                    foreach(QObject *child, children) {
                        if(className == QByteArray(child->metaObject()->className())) {
                            deserialize(child, childData, factory);
                            childFound = true;
                            break;
                        }
                    }
                } else {
                    // If objectName is NOT specified for the child, find the first existing child with matching className.
                    foreach(QObject *child, object->children()) {
                        if(className == QByteArray(child->metaObject()->className())) {
                            deserialize(child, childData, factory);
                            childFound = true;
                            break;
                        }
                    }
                }
                // If we still have not found an existing child, attempt to create one dynamically.
                if(!childFound) {
                    QObject *child = NULL;
                    if(className == QByteArray("QObject"))
                        child = new QObject;
                    else if(factory && factory->hasCreator(className))
                        child = factory->create(className);
                    if(child) {
                        child->setParent(object);
                        deserialize(child, childData, factory);
                    }
                }
            } else if(i.value().type() == QVariant::List) {
                // List of child objects and/or properties.
                QByteArray className = i.key().toUtf8();
                const QVariantList &childDataList = i.value().toList();
                // Keep track of existing children that have been deserialized.
                QObjectList existingChildrenWithClassNameAndObjectName;
                QObjectList existingChildrenWithClassName;
                foreach(QObject *child, object->children()) {
                    if(className == QByteArray(child->metaObject()->className())) {
                        if(!child->objectName().isEmpty())
                            existingChildrenWithClassNameAndObjectName.append(child);
                        else
                            existingChildrenWithClassName.append(child);
                    }
                }
                for(QVariantList::const_iterator j = childDataList.constBegin(); j != childDataList.constEnd(); ++j) {
                    if(j->type() == QVariant::Map) {
                        // Child object.
                        const QVariantMap &childData = j->toMap();
                        bool childFound = false;
                        if(childData.contains("objectName")) {
                            // If objectName is specified for the child, find the first existing child with matching objectName and className.
                            foreach(QObject *child, existingChildrenWithClassNameAndObjectName) {
                                if(child->objectName() == childData.value("objectName").toString()) {
                                    deserialize(child, childData, factory);
                                    existingChildrenWithClassNameAndObjectName.removeOne(child);
                                    childFound = true;
                                    break;
                                }
                            }
                        }
                        if(!childFound) {
                            // If objectName is NOT specified for the child or we could NOT find an object with the same name,
                            // find the first existing child with matching className.
                            if(!existingChildrenWithClassName.isEmpty()) {
                                QObject *child = existingChildrenWithClassName.first();
                                deserialize(child, childData, factory);
                                existingChildrenWithClassName.removeOne(child);
                                childFound = true;
                            }
                        }
                        // If we still havent found an existing child, attempt to create one dynamically.
                        if(!childFound) {
                            QObject *child = NULL;
                            if(className == QByteArray("QObject"))
                                child = new QObject;
                            else if(factory && factory->hasCreator(className))
                                child = factory->create(className);
                            if(child) {
                                child->setParent(object);
                                deserialize(child, childData, factory);
                            }
                        }
                    } else {
                        // Property.
                        const QByteArray &propertyName = className;
                        const QVariant &propertyValue = *j;
                        object->setProperty(propertyName.constData(), propertyValue);
                    }
                }
            } else {
                // Property.
                const QByteArray propertyName = i.key().toUtf8();
                const QVariant &propertyValue = i.value();
                object->setProperty(propertyName.constData(), propertyValue);
            }
        }
    }
    
    void deserialize(QList<QObject*> &objects, const QVariantList &data, ObjectFactory *factory, const QByteArray &objectCreatorKey)
    {
        int i = 0;
        for(QVariantList::const_iterator j = data.constBegin(); j != data.constEnd(); ++j) {
            if(j->type() == QVariant::Map) {
                // Objects should be maps.
                QObject *object = i < objects.size() ? objects[i] : NULL;
                if(!object && factory) {
                    if(!objectCreatorKey.isEmpty()) {
                        object = factory->create(objectCreatorKey);
                    }
                    if(!object && !objects.isEmpty()) {
                        // Use className of  object in the list as the factory creator key.
                        for(QObject *obj : objects) {
                            if(obj) {
                                object = factory->create(obj->metaObject()->className());
                                break;
                            }
                        }
                    }
                }
                if(object) {
                    deserialize(object, j->toMap(), factory);
                    if(i < objects.size()) {
                        objects[i] = object;
                    } else {
                        objects.append(object);
                    }
                } else {
                    // Failed to deserialize map into object.
                    // Should only happen if we failed to create the object (i.e. mising factory, creator key or preallocated object in list)
                    if(i >= objects.size()) {
                        // Since we can't make new objects, give up.
                        return;
                    }
                }
                ++i;
            }
        }
    }
    
    bool readJson(QObject *object, const QString &filePath, ObjectFactory *factory)
    {
        QFile file(filePath);
        if(!file.open(QIODevice::Text | QIODevice::ReadOnly))
            return false;
        QString buffer = file.readAll();
        file.close();
        QVariantMap data = QJsonDocument::fromJson(buffer.toUtf8()).toVariant().toMap();
        deserialize(object, data, factory);
        return true;
    }
    
    bool writeJson(QObject *object, const QString &filePath, int childDepth, bool includeReadOnlyProperties)
    {
        QFile file(filePath);
        if(!file.open(QIODevice::Text | QIODevice::WriteOnly))
            return false;
        QTextStream out(&file);
        QVariantMap data = serialize(object, childDepth, includeReadOnlyProperties);
        out << QJsonDocument::fromVariant(data).toJson(QJsonDocument::Indented);
        file.close();
        return true;
    }
    
} // QtPropertySerializer
