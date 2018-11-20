/* --------------------------------------------------------------------------------
 * Tools for serializing properties in a QObject tree.
 * - Serialize/Deserialize to/from a QVariantMap.
 * - Read/Write from/to a JSON file.
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#ifndef __QtPropertySerializer_H__
#define __QtPropertySerializer_H__

#include <functional>

#include <QByteArray>
#include <QMap>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#ifdef DEBUG
#include <iostream>
#include <QDebug>
#endif

namespace QtPropertySerializer
{
    /* --------------------------------------------------------------------------------
     * Object factory for dynamic object creation during deserialization.
     * -------------------------------------------------------------------------------- */
    class ObjectFactory
    {
    public:
        typedef std::function<QObject*()> ObjectCreatorFunction;
        typedef QMap<QByteArray, ObjectCreatorFunction> ObjectCreatorMap;
        
        // Map of (key,creator) pairs.
        ObjectCreatorMap creators;
        
    public:
        // These functions are not absolutely necessary since the creators map is publicly accessible,
        // but I've kept them here for backwards compatibility and convenience.
        void registerCreator(const QByteArray &key, ObjectCreatorFunction creator) { creators[key] = creator; }
        bool hasCreator(const QByteArray &key) const { return creators.contains(key); }
        ObjectCreatorFunction getCreator(const QByteArray &key) const { return creators.value(key); }
        QList<QByteArray> creatorKeys() const { return creators.keys(); }
        QObject* create(const QByteArray &key) const { return creators.contains(key) ? creators.value(key)() : 0; }
        
        // For convenience. e.g. call ObjectFactory::registerCreator("MyClass", ObjectFactory::defaultCreator<MyClass>);
        template <class T>
        static QObject* defaultCreator() { return new T(); }
        template <class T>
        static QObject* defaultChildCreator(QObject *parent) { return new T(parent); }
        
        // Default creators based on className for convenience.
        template <class T>
        void registerClass() { creators[T::staticMetaObject.className()] = defaultCreator<T>; }
        template <class T>
        void registerChildClass(QObject *parent) { creators[T::staticMetaObject.className()] = std::bind(defaultChildCreator<T>, parent); }
    };
    
    /* --------------------------------------------------------------------------------
     * Serialize QObject --> QVariantMap
     * -------------------------------------------------------------------------------- */
    QVariantMap serialize(const QObject *object, int childDepth = -1, bool includeReadOnlyProperties = true);
    
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
    
} // QtPropertySerializer

#endif
