/*
  Copyright (c) 2026 Jayachandran Dharuman
  This file is part of CANgaroo.
*/

#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QFile>
#include <QTextStream>
#include <QSet>
#include <core/CanMessage.h>
#include <core/CanDbSignal.h>

class Backend;
class CanDbMessage;

enum class ConditionOperator {
    Greater,
    Less,
    Equal,
    GreaterEqual,
    LessEqual,
    NotEqual
};

struct LoggingCondition {
    CanDbSignal *signal;
    ConditionOperator op;
    double threshold;
};

class ConditionalLoggingManager : public QObject
{
    Q_OBJECT

public:
    explicit ConditionalLoggingManager(Backend &backend, QObject *parent = nullptr);
    ~ConditionalLoggingManager();

    void setConditions(const QList<LoggingCondition> &conditions, bool useAndLogic);
    void setLogSignals(const QList<CanDbSignal*> &signalList);
    const QList<CanDbSignal*>& getLogSignals() const { return _logSignals; }
    void setLogFilePath(const QString &path);

    const QList<LoggingCondition>& getConditions() const { return _conditions; }
    bool useAndLogic() const { return _useAndLogic; }
    QString getLogFilePath() const { return _logFilePath; }

    bool isConditionMet() const { return _conditionMet; }
    void setEnabled(bool enabled);
    bool isEnabled() const { return _enabled; }
    void reset();

signals:
    void conditionChanged(bool met);

public slots:
    void processMessage(const CanMessage &msg);

private:
    void evaluate();
    void writeHeader();
    void writeDataRow(double timestamp);

    Backend &_backend;
    bool _enabled;
    bool _conditionMet;
    bool _useAndLogic;
    QList<LoggingCondition> _conditions;
    QList<CanDbSignal*> _logSignals;
    QMap<CanDbSignal*, double> _signalValues;

    QString _logFilePath;
    QFile _logFile;
    QTextStream *_textStream;
};
