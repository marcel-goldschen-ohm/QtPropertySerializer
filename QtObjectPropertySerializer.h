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

#include <functional>

#include <QByteArray>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariant>
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
        typedef std::function<QObject*()> ObjectCreatorFunction;
        typedef QMap<QByteArray, ObjectCreatorFunction> ObjectCreatorMap;
        
    public:
        void registerCreator(const QByteArray &className, ObjectCreatorFunction creator) { _objectCreatorMap[className] = creator; }
        bool hasCreator(const QByteArray &className) const { return _objectCreatorMap.contains(className); }
        ObjectCreatorFunction getCreator(const QByteArray &className) const { return _objectCreatorMap.value(className); }
        QList<QByteArray> creators() const { return _objectCreatorMap.keys(); }
        QObject* create(const QByteArray &className) const { return _objectCreatorMap.contains(className) ? _objectCreatorMap.value(className)() : 0; }
        
        // For convenience. e.g. call ObjectFactory::registerCreator("MyClass", ObjectFactory::defaultCreator<MyClass>);
        template <class T>
        static QObject* defaultCreator() { return new T(); }
        
    private:
        ObjectCreatorMap _objectCreatorMap;
    };
    
    /* --------------------------------------------------------------------------------
     * Serialize QObject --> QVariantMap
     * -------------------------------------------------------------------------------- */
    QVariantMap serialize(const QObject *object, int childDepth = -1, bool includeReadOnlyProperties = true, bool includeObjectName = true);
    
    // Helper function for serialize().
    void addMappedData(QVariantMap &data, const QByteArray &key, const QVariant &value);
    
    /* --------------------------------------------------------------------------------
     * Deserialize QVariantMap --> QObject
     * -------------------------------------------------------------------------------- */
    void deserialize(QObject *object, const QVariantMap &data, ObjectFactory *factory = 0);
    
    /* --------------------------------------------------------------------------------
     * Read/Write from/to JSON file.
     * -------------------------------------------------------------------------------- */
    bool readJson(QObject *object, const QString &filePath, ObjectFactory *factory = 0);
    bool writeJson(QObject *object, const QString &filePath, int childDepth = -1, bool includeReadOnlyProperties = true, bool includeObjectName = true);
    
} // QtObjectPropertySerializer

#endif
