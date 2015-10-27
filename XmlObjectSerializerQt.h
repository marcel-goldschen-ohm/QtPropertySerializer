/* --------------------------------------------------------------------------------
 * XmlObjectSerializerQt
 *
 * Simple XML serialization for arbitrary objects derived from QObject.
 * Serializes both object properties AND child objects.
 *
 * Uses ObjectSerializerQt for serializing objects to/from QVariantMaps.
 *
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#ifndef __XmlObjectSerializerQt_H__
#define __XmlObjectSerializerQt_H__

#include <QDomDocument>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include "ObjectSerializerQt.h"
#ifdef DEBUG
#include <iostream>
#include <QDebug>
#endif


/* --------------------------------------------------------------------------------
 * Simple XML serialization of objects derived from QObject.
 * Serializes both object properties AND child objects.
 * Uses ObjectSerializerQt for serializing objects to/from QVariantMaps.
 * -------------------------------------------------------------------------------- */
class XmlObjectSerializerQt {
public:
    typedef ObjectSerializerQt::ObjectFactory ObjectFactory;


    /* --------------------------------------------------------------------------------
     * Append an object's serialized data as XML children of root.
     * -------------------------------------------------------------------------------- */
    static void appendXml(QDomDocument &doc, QDomElement &root, const QVariantMap &data,
                          const QStringList &attributes = QStringList(),
                          bool allPropertiesAreAttributes = false,
                          bool skipEmptyProperties = true) {
        for(QVariantMap::const_iterator i = data.constBegin(); i != data.constEnd(); ++i) {
            if(i.value().type() == QVariant::Map) {
                // Child node.
                const QString &childName = i.key();
                const QVariantMap &childData = i.value().toMap();
                QDomElement child = doc.createElement(childName);
                root.appendChild(child);
                appendXml(doc, child, childData, attributes, allPropertiesAreAttributes, skipEmptyProperties);
            } else if(i.value().type() == QVariant::List) {
                // List of child or property nodes.
                const QVariantList &childList = i.value().toList();
                for(QVariantList::const_iterator j = childList.constBegin(); j != childList.constEnd(); ++j) {
                    if(j->type() == QVariant::Map) {
                        // Child node.
                        const QString &childName = i.key();
                        const QVariantMap &childData = j->toMap();
                        QDomElement child = doc.createElement(childName);
                        root.appendChild(child);
                        appendXml(doc, child, childData, attributes, allPropertiesAreAttributes, skipEmptyProperties);
                    } else {
                        // Property node.
                        const QString &propertyName = i.key();
                        const QVariant &propertyValue = *j;
                        if(propertyValue.canConvert(QVariant::String)) {
                            QString propertyValueStr = propertyValue.toString();
                            if(propertyValueStr.size() || !skipEmptyProperties) {
                                if(allPropertiesAreAttributes || attributes.contains(propertyName)) {
                                    root.setAttribute(propertyName, propertyValueStr); }
                                else {
                                    QDomElement child = doc.createElement(propertyName);
                                    QDomText text = doc.createTextNode(propertyValueStr);
                                    child.appendChild(text);
                                    root.appendChild(child);
                                }
                            }
                        }
                    }
                }
            } else {
                // Property node.
                const QString &propertyName = i.key();
                const QVariant &propertyValue = i.value();
                if(propertyValue.canConvert(QVariant::String)) {
                    QString propertyValueStr = propertyValue.toString();
                    if(propertyValueStr.size() || !skipEmptyProperties) {
                        if(allPropertiesAreAttributes || attributes.contains(propertyName)) {
                            root.setAttribute(propertyName, propertyValueStr); }
                        else {
                            QDomElement child = doc.createElement(propertyName);
                            QDomText text = doc.createTextNode(propertyValueStr);
                            child.appendChild(text);
                            root.appendChild(child);
                        }
                    }
                }
            }
        }
    }


    /* --------------------------------------------------------------------------------
     * Get QVariantMap data for XML children of root.
     * -------------------------------------------------------------------------------- */
    static QVariantMap parseXml(const QDomElement &root,
                                const ObjectFactory &factory = ObjectFactory()) {
        QVariantMap data;

        // Node attributes (properties).
        QDomNamedNodeMap attrNodes = root.attributes();
        for(int i = 0; i < attrNodes.size(); ++i) {
            QDomAttr attr = attrNodes.item(i).toAttr();
            data[attr.name()] = QVariant(attr.value());
        }

        // Child nodes (may be either properties or child objects).
        const QDomNodeList childNodes = root.childNodes();
        for(int i = 0; i < childNodes.size(); ++i) {
            const QDomNode &childNode = childNodes.at(i);
            if(childNode.isElement()) {
                const QDomElement &child = childNode.toElement();
                if((child.firstChild() == child.lastChild()) && child.firstChild().isText()) {
                    // Property.
                    const QString &propertyName = child.tagName();
                    QString propertyValueStr = child.firstChild().toText().data();
                    ObjectSerializerQt::addMappedData(data, propertyName, QVariant(propertyValueStr));
                } else {
                    // Child.
                    const QString &childName = child.tagName();
                    ObjectSerializerQt::addMappedData(data, childName, parseXml(child, factory));
                }
            }
        }

        return data;
    }


    /* --------------------------------------------------------------------------------
     * Save XML serilization of object to fileName.
     *
     * 1. Adds a root node for the object to doc (tag is either the objectName property if it exists,
     *    or else the meta className - same as for serialization of child objects).
     * 2. Appends the object's serialized data to the root node.
     * 3. Writes XML to fileName.
     * -------------------------------------------------------------------------------- */
    static void saveXml(const QObject *object, const QString &fileName,
                        const QStringList &attributes = QStringList(),
                        bool allPropertiesAreAttributes = false,
                        bool skipEmptyProperties = true) {
        QFile file(fileName);
        if(!file.open(QFile::Text | QFile::WriteOnly))
            return;
        QDomDocument doc;
        const QMetaObject *metaObject = object->metaObject();
        QString tag = object->objectName().size() ? object->objectName() : metaObject->className();
        QDomElement root = doc.createElement(tag);
        doc.appendChild(root);
        QVariantMap data = ObjectSerializerQt::serialize(object);
        appendXml(doc, root, data, attributes, allPropertiesAreAttributes, skipEmptyProperties);
        QTextStream out(&file);
        out << doc.toString();
        file.close();
    }


    /* --------------------------------------------------------------------------------
     * Load XML representation of object from fileName.
     * -------------------------------------------------------------------------------- */
    static void loadXml(QObject *object, const QString &fileName,
                        const ObjectFactory &factory = ObjectFactory()) {
        if(!object)
            return;
        QFile file(fileName);
        if(!file.open(QFile::Text | QFile::ReadOnly))
            return;
        QDomDocument doc;
        doc.setContent(&file);
        file.close();
        const QDomElement &root = doc.firstChildElement();
        QVariantMap data = parseXml(root, factory);
        ObjectSerializerQt::deserialize(object, data, factory);
    }


    /* --------------------------------------------------------------------------------
     * Create new object from XML representation in fileName.
     * -------------------------------------------------------------------------------- */
    static QObject* loadXml(const QString &fileName,
                            const ObjectFactory &factory = ObjectFactory()) {
        QFile file(fileName);
        if(!file.open(QFile::Text | QFile::ReadOnly))
            return 0;
        QDomDocument doc;
        doc.setContent(&file);
        file.close();
        const QDomElement &root = doc.firstChildElement();
        if(!factory.hasCreator(root.tagName()))
            return 0;
        QObject *object = factory.create(root.tagName());
        if(!object)
            return 0;
        QVariantMap data = parseXml(root, factory);
        ObjectSerializerQt::deserialize(object, data, factory);
        return object;
    }

};

#endif
