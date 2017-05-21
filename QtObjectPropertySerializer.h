/* --------------------------------------------------------------------------------
 * Tools for serializing properties in a QObject tree.
 * - Serialize/Deserialize to/from a QVariantMap.
 * - Read/Write from/to a JSON file.
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#ifndef __QtObjectPropertySerializer_H__
#define __QtObjectPropertySerializer_H__

#include <QByteArray>
#include <QFile>
#include <QJsonDocument>
#include <QMap>
#include <QMetaObject>
#include <QMetaProperty>
#include <QObject>
#include <QString>
#include <QTextStream>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#ifdef DEBUG
#include <iostream>
#include <QDebug>
#endif

namespace QtObjectPropertySerializer
{
    /* --------------------------------------------------------------------------------
     * Object factory for dynamic object creation during deserialization.
     * -------------------------------------------------------------------------------- */
    class ObjectFactory
    {
    public:
        typedef QObject* (*ObjectCreatorFuncPtr)();
        typedef QMap<QByteArray, ObjectCreatorFuncPtr> ObjectCreatorMap;
        
    public:
        void registerCreator(const QByteArray &className, ObjectCreatorFuncPtr creator) { _objectCreatorMap[className] = creator; }
        bool hasCreator(const QByteArray &className) const { return _objectCreatorMap.contains(className); }
        ObjectCreatorFuncPtr getCreator(const QByteArray &className) const { return _objectCreatorMap.value(className); }
        QList<QByteArray> creators() const { return _objectCreatorMap.keys(); }
        QObject* create(const QByteArray &className) const { return _objectCreatorMap.contains(className) ? (*_objectCreatorMap.value(className))() : 0; }
        
        // For convenience. e.g. call ObjectFactory::registerCreator("MyClass", ObjectFactory::defaultCreator<MyClass>);
        template <class T>
        static QObject* defaultCreator() { return new T(); }
        
    private:
        ObjectCreatorMap _objectCreatorMap;
    };
    
    /* --------------------------------------------------------------------------------
     * Helper function for serialize().
     * -------------------------------------------------------------------------------- */
    void addMappedData(QVariantMap &data, const QByteArray &key, const QVariant &value)
    {
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
    
    /* --------------------------------------------------------------------------------
     * Serialize QObject --> QVariantMap
     * -------------------------------------------------------------------------------- */
    QVariantMap serialize(const QObject *object, int childDepth = -1, bool includeReadOnlyProperties = true, bool includeObjectName = true)
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
                if(includeObjectName || (propertyName != "objectName")) {
                    const QVariant propertyValue = object->property(propertyName.constData());
                    addMappedData(data, propertyName, propertyValue);
                }
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
    
    /* --------------------------------------------------------------------------------
     * Deserialize QVariantMap --> QObject
     * -------------------------------------------------------------------------------- */
    void deserialize(QObject *object, const QVariantMap &data, ObjectFactory *factory = 0)
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
                    QObject *child = 0;
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
                            QObject *child = 0;
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
    
    /* --------------------------------------------------------------------------------
     * Read/Write from/to JSON file.
     * -------------------------------------------------------------------------------- */
    bool readJson(QObject *object, const QString &filePath, ObjectFactory *factory = 0)
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
    
    bool writeJson(QObject *object, const QString &filePath, int childDepth = -1, bool includeReadOnlyProperties = true, bool includeObjectName = true)
    {
        QFile file(filePath);
        if(!file.open(QIODevice::Text | QIODevice::WriteOnly))
            return false;
        QTextStream out(&file);
        QVariantMap data = serialize(object, childDepth, includeReadOnlyProperties, includeObjectName);
        out << QJsonDocument::fromVariant(data).toJson(QJsonDocument::Indented);
        file.close();
        return true;
    }
    
} // QtObjectPropertySerializer

#endif
