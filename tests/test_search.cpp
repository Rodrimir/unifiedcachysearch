#include <QtTest>
#include "core/searchitem.h"
#include "core/searchmodel.h"
#include "engine/searchengine.h"

class TestSearch : public QObject
{
    Q_OBJECT

private slots:
    void testSearchItemCreation();
    void testSearchItemGettersSetters();
    void testSearchItemDescriptionAndInstalled();
    void testSearchModelAddItems();
    void testSearchModelClear();
    void testSearchModelRoles();
    void testPacmanParsing();
    void testAurParsing();
    void testFlatpakParsing();
    void testComputeRelevance();
    void testFilterByRelevance();
};

void TestSearch::testSearchItemCreation()
{
    SearchItem item("firefox", "100.0-1", "extra",
                    "[Repo: extra] firefox (100.0-1)",
                    "pkexec pacman -S --noconfirm firefox");

    QCOMPARE(item.name(), QString("firefox"));
    QCOMPARE(item.version(), QString("100.0-1"));
    QCOMPARE(item.repository(), QString("extra"));
    QCOMPARE(item.displayName(), QString("[Repo: extra] firefox (100.0-1)"));
    QCOMPARE(item.installCommand(), QString("pkexec pacman -S --noconfirm firefox"));
    QCOMPARE(item.repositoryType(), SearchItem::Pacman);
    QVERIFY(item.description().isEmpty());
    QVERIFY(!item.isInstalled());
}

void TestSearch::testSearchItemGettersSetters()
{
    SearchItem item;
    item.setName("chromium");
    item.setVersion("90.0-1");
    item.setRepository("aur");
    item.setDisplayName("[AUR] chromium (90.0-1)");
    item.setInstallCommand("paru -S chromium");
    item.setRepositoryType(SearchItem::AUR);

    QCOMPARE(item.name(), QString("chromium"));
    QCOMPARE(item.version(), QString("90.0-1"));
    QCOMPARE(item.repository(), QString("aur"));
    QCOMPARE(item.displayName(), QString("[AUR] chromium (90.0-1)"));
    QCOMPARE(item.installCommand(), QString("paru -S chromium"));
    QCOMPARE(item.repositoryType(), SearchItem::AUR);
}

void TestSearch::testSearchItemDescriptionAndInstalled()
{
    SearchItem item;
    item.setName("spotify");
    item.setVersion("1.2.0");
    item.setRepository("flathub");
    item.setDescription("Music streaming service");
    item.setIsInstalled(true);

    QCOMPARE(item.description(), QString("Music streaming service"));
    QVERIFY(item.isInstalled());

    item.setIsInstalled(false);
    QVERIFY(!item.isInstalled());

    QCOMPARE(item.repositoryType(), SearchItem::Pacman);
}

void TestSearch::testSearchModelAddItems()
{
    SearchModel model;

    QCOMPARE(model.rowCount(), 0);

    SearchItem item1("pkg1", "1.0", "extra", "[Repo: extra] pkg1 (1.0)", "pkexec pacman -S --noconfirm pkg1");
    model.addItem(item1);

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.itemAt(0).name(), QString("pkg1"));

    SearchItem item2("pkg2", "2.0", "aur", "[AUR] pkg2 (2.0)", "paru -S pkg2");
    model.addItem(item2);

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.itemAt(1).name(), QString("pkg2"));

    QList<SearchItem> all = model.allItems();
    QCOMPARE(all.size(), 2);
}

void TestSearch::testSearchModelClear()
{
    SearchModel model;
    model.addItem(SearchItem("a", "1", "extra", "a (1)", "cmd"));
    model.addItem(SearchItem("b", "2", "aur", "b (2)", "cmd"));
    QCOMPARE(model.rowCount(), 2);

    model.clear();
    QCOMPARE(model.rowCount(), 0);
}

void TestSearch::testSearchModelRoles()
{
    SearchModel model;
    SearchItem item("testpkg", "3.0-1", "aur",
                    "[AUR] testpkg (3.0-1)", "paru -S testpkg");
    item.setRepositoryType(SearchItem::AUR);
    item.setDescription("A test package");
    item.setIsInstalled(true);
    model.addItem(item);

    QModelIndex idx = model.index(0, 0);

    QCOMPARE(model.data(idx, SearchModel::NameRole).toString(), QString("testpkg"));
    QCOMPARE(model.data(idx, SearchModel::VersionRole).toString(), QString("3.0-1"));
    QCOMPARE(model.data(idx, SearchModel::RepositoryRole).toString(), QString("aur"));
    QCOMPARE(model.data(idx, SearchModel::DisplayNameRole).toString(),
             QString("[AUR] testpkg (3.0-1)"));
    QCOMPARE(model.data(idx, SearchModel::InstallCommandRole).toString(),
             QString("paru -S testpkg"));
    QCOMPARE(model.data(idx, SearchModel::RepositoryTypeRole).toInt(),
             static_cast<int>(SearchItem::AUR));
    QCOMPARE(model.data(idx, SearchModel::DescriptionRole).toString(), QString("A test package"));
    QCOMPARE(model.data(idx, SearchModel::IsInstalledRole).toBool(), true);

    QString color = model.data(idx, SearchModel::RepoBadgeColorRole).toString();
    QVERIFY(!color.isEmpty());
}

void TestSearch::testPacmanParsing()
{
    QString simulatedOutput = "extra/firefox 100.0-1\n"
                              "    Mozilla Firefox\n"
                              "core-testing/linux 6.0-1\n"
                              "    Linux kernel\n";

    QStringList lines = simulatedOutput.split('\n', Qt::SkipEmptyParts);
    QRegularExpression pkgRegex(R"(^([^ /]+)/([^ ]+) +(.+)$)");
    int count = 0;

    for (const auto &line : lines) {
        if (line.startsWith(' '))
            continue;

        auto match = pkgRegex.match(line);
        if (match.hasMatch()) {
            count++;
            QString repo = match.captured(1);
            QString pkgName = match.captured(2);
            QString version = match.captured(3);

            QVERIFY(!repo.isEmpty());
            QVERIFY(!pkgName.isEmpty());
            QVERIFY(!version.isEmpty());
        }
    }

    QCOMPARE(count, 2);
}

void TestSearch::testAurParsing()
{
    QString simulatedOutput = "aur/firefox-nightly 100.0a1-1\n"
                              "    Firefox Nightly\n"
                              "aur/google-chrome 90.0-1\n"
                              "    Google Chrome\n";

    QStringList lines = simulatedOutput.split('\n', Qt::SkipEmptyParts);
    int count = 0;

    for (const auto &line : lines) {
        if (!line.startsWith("aur/"))
            continue;

        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            count++;
            QString pkgName = parts[0].section('/', 1, 1);
            QString version = parts[1];

            QVERIFY(!pkgName.isEmpty());
            QVERIFY(!version.isEmpty());
        }
    }

    QCOMPARE(count, 2);
}

void TestSearch::testFlatpakParsing()
{
    QString simulatedOutput = "Name\tDescription\tID\tVersion\tRemote\n"
                              "Firefox\tWeb Browser\torg.mozilla.firefox\t100.0\tflathub\n"
                              "Spotify\tMusic\tcom.spotify.Client\t1.2.0\tflathub\n";

    QStringList lines = simulatedOutput.split('\n', Qt::SkipEmptyParts);
    int count = 0;

    for (int i = 1; i < lines.size(); ++i) {
        const QString &line = lines[i].trimmed();
        if (line.isEmpty())
            continue;

        QStringList parts = line.split('\t', Qt::SkipEmptyParts);
        if (parts.size() < 2)
            continue;

        QString appId = parts[2];
        if (appId == "Name" || appId == "Application" || !appId.contains('.'))
            continue;

        count++;
        QVERIFY(!appId.isEmpty());
        QVERIFY(appId.contains('.'));
    }

    QCOMPARE(count, 2);
}

void TestSearch::testComputeRelevance()
{
    // Exact name match
    int score = SearchEngine::computeRelevance("cura", "Cura 3D printing slicer", "cura");
    QVERIFY2(score >= 100, QString("Exact name match failed: %1").arg(score).toLatin1());

    // Prefix match (name starts with term + hyphen)
    score = SearchEngine::computeRelevance("cura-engine", "Engine for Cura", "cura");
    QVERIFY2(score >= 80, QString("Prefix hyphen match failed: %1").arg(score).toLatin1());

    // Name substring match (not prefix)
    score = SearchEngine::computeRelevance("procura", "Some tool", "cura");
    QVERIFY2(score >= 40 && score < 65,
             QString("Name substring match failed: %1").arg(score).toLatin1());

    // Word match in description
    score = SearchEngine::computeRelevance("somepkg", "A cura for diseases", "cura");
    QVERIFY2(score >= 20 && score < 40,
             QString("Word in description match failed: %1").arg(score).toLatin1());

    // Substring in description — low score, filtered out
    score = SearchEngine::computeRelevance("somepkg", "High accuracy tool", "cura");
    QVERIFY2(score < 15,
             QString("Substring in description should be low: %1").arg(score).toLatin1());

    // Multi-word: "cura" matches name, "engine" matches name
    int multiScore = SearchEngine::computeRelevance("cura-engine", "Cura slicing engine", "cura engine");
    QVERIFY2(multiScore >= 60,
             QString("Multi-word relevance failed: %1").arg(multiScore).toLatin1());
}

void TestSearch::testFilterByRelevance()
{
    QList<SearchItem> items;

    auto makeItem = [](const QString &name, const QString &desc) {
        SearchItem item;
        item.setName(name);
        item.setDescription(desc);
        return item;
    };

    items << makeItem("cura", "Cura 3D printing slicer");
    items << makeItem("cura-engine", "Engine component for Cura");
    items << makeItem("libcerf", "Self-contained numeric library for error functions");

    // "accuracy" contains "cura" substring, this would match pacman -Ss but should be filtered
    items << makeItem("whipper", "Python CD-DA ripper preferring accuracy over speed");
    items << makeItem("ares-emu", "Multi-system emulator focusing on accuracy");

    QList<SearchItem> filtered = SearchEngine::filterByRelevance(items, "cura", 15);

    QVERIFY2(filtered.size() >= 2,
             QString("Expected at least 2 results (cura, cura-engine), got %1")
                 .arg(filtered.size()).toLatin1());

    // cura and cura-engine should both be in results
    QSet<QString> names;
    for (const auto &i : filtered) names.insert(i.name());
    QVERIFY2(names.contains("cura"),
             "cura should be in filtered results");
    QVERIFY2(names.contains("cura-engine"),
             "cura-engine should be in filtered results");

    // whipper (accuracy) should NOT be in results
    QVERIFY2(!names.contains("whipper"),
             "whipper (accuracy) should be filtered out");
    QVERIFY2(!names.contains("ares-emu"),
             "ares-emu (accuracy) should be filtered out");
    QVERIFY2(!names.contains("libcerf"),
             "libcerf should be filtered out");

    // Results should be sorted by relevance (cura first, then cura-engine)
    QVERIFY2(filtered[0].relevanceScore() >= filtered[1].relevanceScore(),
             "Results should be sorted by relevance descending");
}

QTEST_MAIN(TestSearch)
#include "test_search.moc"
