/*
  Copyright (c) 2026 Jayachandran Dharuman
  This file is part of CANgaroo.
*/

#include "ConditionalLoggingManager.h"
#include <core/Backend.h>
#include <core/CanMessage.h>
#include <core/CanDbMessage.h>
#include <core/CanDbSignal.h>
#include <QDateTime>

ConditionalLoggingManager::ConditionalLoggingManager(Backend &backend, QObject *parent)
    : QObject(parent), _backend(backend), _enabled(false), _conditionMet(false), _useAndLogic(true), _textStream(nullptr)
{
}

ConditionalLoggingManager::~ConditionalLoggingManager()
{
    setEnabled(false);
}

void ConditionalLoggingManager::setEnabled(bool enabled)
{
    if (_enabled == enabled) return;
    _enabled = enabled;

    if (!_enabled) {
        if (_logFile.isOpen()) {
            _logFile.close();
        }
        delete _textStream;
        _textStream = nullptr;
        _conditionMet = false;
        emit conditionChanged(false);
    }
}

void ConditionalLoggingManager::reset()
{
    setEnabled(false);
    _conditions.clear();
    _logSignals.clear();
    _signalValues.clear();
    _logFilePath.clear();
}

void ConditionalLoggingManager::setConditions(const QList<LoggingCondition> &conditions, bool useAndLogic)
{
    _conditions = conditions;
    _useAndLogic = useAndLogic;
}

void ConditionalLoggingManager::setLogSignals(const QList<CanDbSignal*> &signalList)
{
    _logSignals = signalList;
}

void ConditionalLoggingManager::setLogFilePath(const QString &path)
{
    _logFilePath = path;
}

void ConditionalLoggingManager::processMessage(const CanMessage &msg)
{
    if (!_enabled) return;

    CanDbMessage *dbmsg = _backend.findDbMessage(msg);
    if (!dbmsg) return;

    bool relevantUpdate = false;
    foreach (CanDbSignal *signal, dbmsg->getSignals()) {
        if (signal->isPresentInMessage(msg)) {
            double value = signal->extractPhysicalFromMessage(msg);
            _signalValues[signal] = value;
            relevantUpdate = true;
        }
    }

    if (relevantUpdate) {
        evaluate();
        if (_conditionMet && _textStream) {
            writeDataRow(msg.getFloatTimestamp());
        }
    }
}

void ConditionalLoggingManager::evaluate()
{
    if (_conditions.isEmpty()) {
        if (_conditionMet) {
            _conditionMet = false;
            emit conditionChanged(false);
        }
        return;
    }

    bool result = _useAndLogic;

    for (const auto &cond : _conditions) {
        if (!_signalValues.contains(cond.signal)) {
            if (_useAndLogic) {
                result = false;
                break;
            }
            continue;
        }

        double val = _signalValues[cond.signal];
        bool condMet = false;

        switch (cond.op) {
            case ConditionOperator::Greater:      condMet = (val > cond.threshold); break;
            case ConditionOperator::Less:         condMet = (val < cond.threshold); break;
            case ConditionOperator::Equal:        condMet = (val == cond.threshold); break;
            case ConditionOperator::GreaterEqual: condMet = (val >= cond.threshold); break;
            case ConditionOperator::LessEqual:    condMet = (val <= cond.threshold); break;
            case ConditionOperator::NotEqual:     condMet = (val != cond.threshold); break;
        }

        if (_useAndLogic) {
            result &= condMet;
            if (!result) break;
        } else {
            result |= condMet;
            if (result) break;
        }
    }

    if (result != _conditionMet) {
        _conditionMet = result;
        emit conditionChanged(_conditionMet);

        if (_conditionMet) {
            // Start logging to file if path is provided
            if (!_logFilePath.isEmpty()) {
                _logFile.setFileName(_logFilePath);
                if (_logFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
                    _textStream = new QTextStream(&_logFile);
                    if (_logFile.size() == 0) {
                        writeHeader();
                    }
                }
            }
        } else {
            // Stop logging to file
            if (_logFile.isOpen()) {
                _logFile.close();
            }
            delete _textStream;
            _textStream = nullptr;
        }
    }
}

void ConditionalLoggingManager::writeHeader()
{
    if (!_textStream) return;
    *_textStream << "Timestamp";
    for (CanDbSignal *sig : _logSignals) {
        *_textStream << "," << sig->name();
        if (!sig->getUnit().isEmpty()) {
            *_textStream << " [" << sig->getUnit() << "]";
        }
    }
    *_textStream << "\n";
}

void ConditionalLoggingManager::writeDataRow(double timestamp)
{
    if (!_textStream) return;
    *_textStream << QString::number(timestamp, 'f', 6);
    for (CanDbSignal *sig : _logSignals) {
        *_textStream << ",";
        if (_signalValues.contains(sig)) {
            *_textStream << _signalValues[sig];
        } else {
            *_textStream << "NaN";
        }
    }
    *_textStream << "\n";
}
