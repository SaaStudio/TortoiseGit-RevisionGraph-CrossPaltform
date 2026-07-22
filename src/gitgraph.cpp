#include "gitgraph.h"

#include <QProcess>
#include <QSet>

#include <algorithm>

namespace {
QByteArray runGit(const QString& directory, const QStringList& arguments, QString& error, bool allowExitOne = false)
{
    QProcess process;
    process.setWorkingDirectory(directory);
    process.start(QStringLiteral("git"), arguments);
    if (!process.waitForStarted(5000)) {
        error = QStringLiteral("Не удалось запустить git: %1").arg(process.errorString());
        return {};
    }
    if (!process.waitForFinished(120000)) {
        process.kill();
        error = QStringLiteral("Git не завершил команду за 120 секунд");
        return {};
    }
    if (process.exitCode() != 0 && !(allowExitOne && process.exitCode() == 1)) {
        error = QString::fromUtf8(process.readAllStandardError()).trimmed();
        if (error.isEmpty())
            error = QStringLiteral("Git завершился с кодом %1").arg(process.exitCode());
        return {};
    }
    return process.readAllStandardOutput();
}
}

GitGraphResult GitGraphLoader::load(const QString& path, bool showBranchingsMerges, bool showAllTags)
{
    GitGraphResult result;
    QString error;
    result.repositoryRoot = QString::fromUtf8(runGit(path,
        {QStringLiteral("rev-parse"), QStringLiteral("--show-toplevel")}, error)).trimmed();
    if (!error.isEmpty()) {
        result.error = error;
        return result;
    }

    result.currentBranch = QString::fromUtf8(runGit(result.repositoryRoot,
        {QStringLiteral("symbolic-ref"), QStringLiteral("--quiet"), QStringLiteral("--short"), QStringLiteral("HEAD")}, error, true)).trimmed();
    error.clear();
    result.headHash = QString::fromUtf8(runGit(result.repositoryRoot,
        {QStringLiteral("rev-parse"), QStringLiteral("--verify"), QStringLiteral("HEAD")}, error, true)).trimmed();
    error.clear();

    // Exact data source used by TortoiseGit RevisionGraph:
    // LOG_INFO_SIMPILFY_BY_DECORATION | LOG_INFO_ALL_BRANCH and --parents.
    QStringList logArguments = {QStringLiteral("log"), QStringLiteral("-z"), QStringLiteral("--parents"),
        QStringLiteral("--simplify-by-decoration")};
    if (showBranchingsMerges)
        logArguments.append(QStringLiteral("--sparse"));
    logArguments << QStringLiteral("--all") << QStringLiteral("--pretty=format:%H %P");
    const QByteArray log = runGit(result.repositoryRoot, logArguments, error, true);
    if (!error.isEmpty()) {
        result.error = error;
        return result;
    }
    const QList<QByteArray> records = log.split('\0');
    for (const QByteArray& raw : records) {
        const QList<QByteArray> fields = raw.trimmed().split(' ');
        if (fields.isEmpty() || fields.front().isEmpty())
            continue;
        GitCommit commit;
        commit.hash = QString::fromLatin1(fields.front());
        for (int i = 1; i < fields.size(); ++i) {
            if (!fields[i].isEmpty())
                commit.parents.append(QString::fromLatin1(fields[i]));
        }
        result.commits.append(std::move(commit));
    }

    // Exact equivalent of CGit::GetMapHashToFriendName() in CLI mode.
    const QByteArray refs = runGit(result.repositoryRoot,
        {QStringLiteral("show-ref"), QStringLiteral("-d")}, error, true);
    if (!error.isEmpty()) {
        result.error = error;
        return result;
    }
    for (const QByteArray& line : refs.split('\n')) {
        const int separator = line.indexOf(' ');
        if (separator <= 0)
            continue;
        const QString hash = QString::fromLatin1(line.left(separator));
        result.refsByHash[hash].append(QString::fromUtf8(line.mid(separator + 1)).trimmed());
    }
    for (auto it = result.refsByHash.begin(); it != result.refsByHash.end(); ++it)
        it.value().sort();

    // Exact rewrite from CRevisionGraphWnd::FetchRevisionData(). Linear,
    // unlabeled commits are removed only for these two display modes.
    if (!showAllTags || showBranchingsMerges) {
        QHash<QString, QStringList> childMap;
        QHash<QString, int> indexByHash;
        for (int i = 0; i < result.commits.size(); ++i) {
            indexByHash.insert(result.commits[i].hash, i);
            for (const QString& parent : result.commits[i].parents)
                childMap[parent].append(result.commits[i].hash);
        }

        QSet<QString> skipList;
        for (int i = 0; i < result.commits.size(); ++i) {
            GitCommit& commit = result.commits[i];
            const QStringList names = result.refsByHash.value(commit.hash);
            if (!names.isEmpty()) {
                if (showAllTags)
                    continue;
                bool haveNonTagName = false;
                for (const QString& name : names) {
                    if (!name.startsWith(QStringLiteral("refs/tags/"))) {
                        haveNonTagName = true;
                        break;
                    }
                }
                if (haveNonTagName)
                    continue;
            }
            if (commit.parents.size() != 1)
                continue;
            const QStringList children = childMap.value(commit.hash);
            if (children.size() != 1 || !indexByHash.contains(children.front()))
                continue;
            GitCommit& child = result.commits[indexByHash.value(children.front())];
            if (child.parents.size() != 1)
                continue;

            skipList.insert(commit.hash);
            const QString oldParent = commit.parents.front();
            child.parents[0] = oldParent;
            QStringList& parentChildren = childMap[oldParent];
            const int position = parentChildren.indexOf(commit.hash);
            if (position >= 0)
                parentChildren[position] = child.hash;
            childMap.remove(commit.hash);
        }
        result.commits.erase(std::remove_if(result.commits.begin(), result.commits.end(),
            [&skipList](const GitCommit& commit) { return skipList.contains(commit.hash); }), result.commits.end());
    }
    return result;
}
