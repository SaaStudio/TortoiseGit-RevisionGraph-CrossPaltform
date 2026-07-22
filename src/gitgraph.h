#pragma once

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

struct GitCommit
{
    QString hash;
    QStringList parents;
};

struct GitGraphResult
{
    QVector<GitCommit> commits;
    QHash<QString, QStringList> refsByHash;
    QString currentBranch;
    QString headHash;
    QString repositoryRoot;
    QString error;
};

class GitGraphLoader
{
public:
    static GitGraphResult load(const QString& path, bool showBranchingsMerges, bool showAllTags);
};
