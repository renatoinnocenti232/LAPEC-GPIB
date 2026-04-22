#include "jsonschema.h"

bool JsonSchemaValidator::validateCommand(const QJsonObject& obj, QString& error) {
    if (!obj.contains("command") || !obj["command"].isString()) {
        error = "Campo 'command' ausente ou inválido.";
        return false;
    }
    QString cmd = obj["command"].toString();
    if (cmd == "write" || cmd == "query" || cmd == "read" || cmd == "batch") {
        if (!obj.contains("address") || !obj["address"].isDouble()) {
            error = "Campo 'address' (número) obrigatório.";
            return false;
        }
        int addr = obj["address"].toInt();
        if (addr < 0 || addr > 30) {
            error = "Endereço GPIB deve estar entre 0 e 30.";
            return false;
        }
        // board é opcional, mas se presente deve ser número
        if (obj.contains("board") && !obj["board"].isDouble()) {
            error = "Campo 'board' deve ser número.";
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
        if (cmd == "batch" && (!obj.contains("commands") || !obj["commands"].isArray())) {
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