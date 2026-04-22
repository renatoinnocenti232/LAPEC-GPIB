#ifndef JSONSCHEMA_H
#define JSONSCHEMA_H

#include <QJsonObject>
#include <QString>

class JsonSchemaValidator {
public:
    static bool validateCommand(const QJsonObject& obj, QString& error);
    static QJsonObject createErrorResponse(const QString& error);
};

#endif