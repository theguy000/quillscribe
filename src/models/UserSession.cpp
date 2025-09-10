#include "UserSession.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

UserSession::UserSession()
    : m_id(generateUuid())
    , m_name("")
    , m_startTime(QDateTime::currentDateTime())
    , m_endTime(QDateTime())
    , m_recordingCount(0)
    , m_totalDuration(0)
    , m_tags(QStringList())
    , m_notes("")
    , m_status(SessionStatus::Active)
{
}

UserSession::UserSession(const QString& name)
    : m_id(generateUuid())
    , m_name(name)
    , m_startTime(QDateTime::currentDateTime())
    , m_endTime(QDateTime())
    , m_recordingCount(0)
    , m_totalDuration(0)
    , m_tags(QStringList())
    , m_notes("")
    , m_status(SessionStatus::Active)
{
}

UserSession::UserSession(const QJsonObject& json)
    : UserSession()
{
    fromJson(json);
}

UserSession::UserSession(const UserSession& other)
    : m_id(other.m_id)
    , m_name(other.m_name)
    , m_startTime(other.m_startTime)
    , m_endTime(other.m_endTime)
    , m_recordingCount(other.m_recordingCount)
    , m_totalDuration(other.m_totalDuration)
    , m_tags(other.m_tags)
    , m_notes(other.m_notes)
    , m_status(other.m_status)
{
}

UserSession& UserSession::operator=(const UserSession& other) {
    if (this != &other) {
        m_id = other.m_id;
        m_name = other.m_name;
        m_startTime = other.m_startTime;
        m_endTime = other.m_endTime;
        m_recordingCount = other.m_recordingCount;
        m_totalDuration = other.m_totalDuration;
        m_tags = other.m_tags;
        m_notes = other.m_notes;
        m_status = other.m_status;
    }
    return *this;
}

QString UserSession::getId() const {
    return m_id;
}

QJsonObject UserSession::toJson() const {
    QJsonObject json;
    json["id"] = m_id;
    json["name"] = m_name;
    json["startTime"] = m_startTime.toString(Qt::ISODate);
    json["endTime"] = m_endTime.isValid() ? m_endTime.toString(Qt::ISODate) : QString();
    json["recordingCount"] = m_recordingCount;
    json["totalDuration"] = static_cast<qint64>(m_totalDuration);
    
    QJsonArray tagsArray;
    for (const QString& tag : m_tags) {
        tagsArray.append(tag);
    }
    json["tags"] = tagsArray;
    
    json["notes"] = m_notes;
    json["status"] = sessionStatusToString(m_status);
    return json;
}

bool UserSession::fromJson(const QJsonObject& json) {
    if (!json.contains("id") || !isValidUuid(json["id"].toString())) {
        return false;
    }
    
    m_id = json["id"].toString();
    m_name = json.value("name").toString();
    
    QString startTimeStr = json.value("startTime").toString();
    m_startTime = QDateTime::fromString(startTimeStr, Qt::ISODate);
    if (!m_startTime.isValid()) {
        m_startTime = QDateTime::currentDateTime();
    }
    
    QString endTimeStr = json.value("endTime").toString();
    if (!endTimeStr.isEmpty()) {
        m_endTime = QDateTime::fromString(endTimeStr, Qt::ISODate);
    }
    
    m_recordingCount = json.value("recordingCount").toInt(0);
    m_totalDuration = json.value("totalDuration").toVariant().toLongLong();
    
    QJsonArray tagsArray = json.value("tags").toArray();
    m_tags.clear();
    for (const QJsonValue& tagValue : tagsArray) {
        m_tags.append(tagValue.toString());
    }
    
    m_notes = json.value("notes").toString();
    
    QString statusStr = json.value("status").toString("Active");
    m_status = sessionStatusFromString(statusStr);
    
    return true;
}

bool UserSession::isValid() const {
    if (!isValidUuid(m_id)) {
        return false;
    }
    
    if (!validateName()) {
        return false;
    }
    
    if (!validateDateTimes()) {
        return false;
    }
    
    if (!validateCounts()) {
        return false;
    }
    
    return true;
}

// Implementation of remaining methods would continue here...
// Due to space constraints, showing abbreviated implementation

bool UserSession::validateName() const {
    QString trimmedName = m_name.trimmed();
    return !trimmedName.isEmpty() && trimmedName.length() <= 255;
}

bool UserSession::validateDateTimes() const {
    if (!m_startTime.isValid()) {
        return false;
    }
    
    if (m_endTime.isValid() && m_endTime <= m_startTime) {
        return false;
    }
    
    return true;
}

bool UserSession::validateCounts() const {
    if (m_recordingCount < 0 || m_totalDuration < 0) {
        return false;
    }
    
    if (m_recordingCount > 0 && m_totalDuration <= 0) {
        return false;
    }
    
    return true;
}

bool UserSession::operator==(const UserSession& other) const {
    return m_id == other.m_id;
}

bool UserSession::operator!=(const UserSession& other) const {
    return !(*this == other);
}