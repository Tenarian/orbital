#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QList>
#include <QMap>

#include "orbital/envvarschema.h"
#include "orbital/types.h"

namespace Orbital {

class EnvVarModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        KeyRole = Qt::UserRole + 1,
        ValueRole,
        SourceRole,
        StateRole,
        CategoryRole,
        GroupRole,
        LockedRole,
        ErrorRole
    };

    explicit EnvVarModel(QObject *parent = nullptr);
    ~EnvVarModel() override;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // QML-friendly accessors
    Q_INVOKABLE void load();
    Q_INVOKABLE void reload();
    Q_INVOKABLE void setValue(const QString &key, const QString &value);
    Q_INVOKABLE void addCustom(const QString &key, const QString &value);
    Q_INVOKABLE void removeCustom(const QString &key);
    Q_INVOKABLE QList<EnvVarEntry> entries() const;
    Q_INVOKABLE QList<EnvVarEntry> entriesForCategory(const QString &categoryId) const;
    Q_INVOKABLE QList<EnvVarEntry> allEntries() const;
    Q_INVOKABLE bool hasKey(const QString &key) const;
    Q_INVOKABLE QString categoryIdForKey(const QString &key) const;

    int count() const;

public slots:
    void onSchemaLoaded();
    void onSchemaError(const QString &error);

private:
    EnvVarSchemaLoader m_loader;
    QList<EnvVarEntry> m_entries;
    QMap<QString, QString> m_customVars;  // key -> value
};

} // namespace Orbital
