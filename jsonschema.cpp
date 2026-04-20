#include "jsonschema.h"

bool JsonSchemaValidator::validateCommand(const QJsonObject& obj, QString& error) {
    // Esquema básico: deve conter "command" (string)
    if (!obj.contains("command") || !obj["command"].isString()) {
        error = "Campo 'command' ausente ou inválido.";
        return false;
    }
    QString cmd = obj["command"].toString();
    if (cmd == "write" || cmd == "query" || cmd == "read") {
        if (!obj.contains("address") || !obj["address"].isDouble()) {
            error = "Campo 'address' (número) obrigatório.";
            return false;
        }
        if (cmd == "write" && (!obj.contains("data") || !obj["data"].isString())) {
            error = "Campo 'data' (string) obrigatório para 'write'.";
            return false;
        }
        if (cmd == "query" && (!obj.contains("query") || !obj["query"].isString())) {
            error = "Campo 'query' (string) obrigatório para 'query'.";
            return false;
        }
    } else if (cmd == "batch") {
        if (!obj.contains("address") || !obj["address"].isDouble()) {
            error = "Campo 'address' obrigatório.";
            return false;
        }
        if (!obj.contains("commands") || !obj["commands"].isArray()) {
            error = "Campo 'commands' (array) obrigatório para 'batch'.";
            return false;
        }
    } else {
        error = "Comando desconhecido: " + cmd;
        return false;
    }
    return true;
}

QJsonObject JsonSchemaValidator::createErrorResponse(const QString& error) {
    QJsonObject resp;
    resp["status"] = "error";
    resp["error"] = error;
    return resp;
}