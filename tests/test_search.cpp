#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include "core/command.h"
#include "core/exportservice.h"
#include "core/searchitem.h"
#include "core/searchmodel.h"
#include "engine/commandbuilder.h"
#include "engine/packageparser.h"

// Os testes exercitam o codigo de producao (PackageParser, CommandBuilder,
// ExportService). A versao anterior reimplementava o parsing dentro do proprio
// teste, entao passava mesmo que o parser real quebrasse.
class TestSearch : public QObject
{
    Q_OBJECT

private slots:
    // core
    void testSearchItemGettersSetters();
    void testCommandToStringQuoting();
    void testCommandEnvironment();
    void testSearchModelAddItems();
    void testSearchModelClear();
    void testSearchModelRoles();

    // comandos
    void testPacmanInstallAvoidsPartialUpgrade();
    void testUninstallCommands();
    void testFlatpakCommands();

    // parsing
    void testSplitQuery();
    void testPacmanParsing();
    void testAurParsing();
    void testFlatpakParsing();
    void testFlatpakParsingKeepsEmptyDescription();
    void testFlatpakParsingNoMatches();
    void testInfoParsing();
    void testPacmanUpdatesParsing();
    void testFlatpakUpdatesParsing();

    // relevancia
    void testComputeRelevance();
    void testFilterByRelevance();

    // exportacao
    void testExportCsvEscapesQuotes();
    void testExportJsonRoundTrip();
    void testExportTextIsNotTruncated();

private:
    // Helper fixo para nao depender do que estiver instalado na maquina.
    CommandBuilder m_commands{QStringLiteral("paru")};
};

// --- core -----------------------------------------------------------------

void TestSearch::testSearchItemGettersSetters()
{
    SearchItem item("chromium", "90.0-1", "aur", "[AUR] chromium (90.0-1)",
                    Command("paru", {"-S", "chromium"}));

    QCOMPARE(item.name(), QString("chromium"));
    QCOMPARE(item.version(), QString("90.0-1"));
    QCOMPARE(item.repository(), QString("aur"));
    QCOMPARE(item.displayName(), QString("[AUR] chromium (90.0-1)"));
    QCOMPARE(item.installCommandText(), QString("paru -S chromium"));
    QCOMPARE(item.repositoryType(), SearchItem::Pacman);
    QVERIFY(item.description().isEmpty());
    QVERIFY(!item.isInstalled());

    item.setRepositoryType(SearchItem::AUR);
    item.setDescription("Navegador");
    item.setIsInstalled(true);

    QCOMPARE(item.repositoryType(), SearchItem::AUR);
    QCOMPARE(item.description(), QString("Navegador"));
    QVERIFY(item.isInstalled());
}

void TestSearch::testCommandToStringQuoting()
{
    QVERIFY(Command().isEmpty());
    QVERIFY(Command().toString().isEmpty());

    const Command simple("pacman", {"-S", "firefox"});
    QCOMPARE(simple.toString(), QString("pacman -S firefox"));
    QCOMPARE(simple.program(), QString("pacman"));
    QCOMPARE(simple.arguments().size(), 2);

    // Argumentos com espaco nunca podem virar dois argumentos.
    const Command spaced("flatpak", {"install", "-y", "meu remote"});
    QCOMPARE(spaced.toString(), QString("flatpak install -y 'meu remote'"));
}

void TestSearch::testCommandEnvironment()
{
    const Command cmd("paru", {"-S", "pkg"},
                      {{"SUDO_ASKPASS", "/run/user/1000/askpass.sh"}});

    QCOMPARE(cmd.toString(),
             QString("SUDO_ASKPASS=/run/user/1000/askpass.sh paru -S pkg"));
    QCOMPARE(cmd.environment().value("SUDO_ASKPASS"),
             QString("/run/user/1000/askpass.sh"));
    // O ambiente nao vira argumento do programa.
    QCOMPARE(cmd.program(), QString("paru"));
    QCOMPARE(cmd.arguments(), QStringList({"-S", "pkg"}));
}

void TestSearch::testSearchModelAddItems()
{
    SearchModel model;
    QCOMPARE(model.rowCount(), 0);

    model.addItem(SearchItem("pkg1", "1.0", "extra", "pkg1 (1.0)", Command()));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.itemAt(0).name(), QString("pkg1"));

    model.addItems({SearchItem("pkg2", "2.0", "aur", "pkg2 (2.0)", Command()),
                    SearchItem("pkg3", "3.0", "aur", "pkg3 (3.0)", Command())});
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.allItems().size(), 3);

    // Indice invalido devolve um item vazio em vez de estourar.
    QVERIFY(model.itemAt(99).name().isEmpty());
    QVERIFY(model.itemAt(-1).name().isEmpty());
}

void TestSearch::testSearchModelClear()
{
    SearchModel model;
    model.addItem(SearchItem("a", "1", "extra", "a (1)", Command()));
    model.addItem(SearchItem("b", "2", "aur", "b (2)", Command()));
    QCOMPARE(model.rowCount(), 2);

    model.clear();
    QCOMPARE(model.rowCount(), 0);
}

void TestSearch::testSearchModelRoles()
{
    SearchModel model;
    SearchItem item("testpkg", "3.0-1", "aur", "[AUR] testpkg (3.0-1)",
                    Command("paru", {"-S", "testpkg"}));
    item.setRepositoryType(SearchItem::AUR);
    item.setDescription("A test package");
    item.setIsInstalled(true);
    model.addItem(item);

    const QModelIndex idx = model.index(0, 0);
    QCOMPARE(model.data(idx, SearchModel::NameRole).toString(), QString("testpkg"));
    QCOMPARE(model.data(idx, SearchModel::VersionRole).toString(), QString("3.0-1"));
    QCOMPARE(model.data(idx, SearchModel::RepositoryRole).toString(), QString("aur"));
    QCOMPARE(model.data(idx, SearchModel::DisplayNameRole).toString(),
             QString("[AUR] testpkg (3.0-1)"));
    QCOMPARE(model.data(idx, SearchModel::InstallCommandRole).toString(),
             QString("paru -S testpkg"));
    QCOMPARE(model.data(idx, SearchModel::RepositoryTypeRole).toInt(),
             static_cast<int>(SearchItem::AUR));
    QCOMPARE(model.data(idx, SearchModel::DescriptionRole).toString(),
             QString("A test package"));
    QVERIFY(model.data(idx, SearchModel::IsInstalledRole).toBool());
    QVERIFY(!model.data(idx, SearchModel::RepoBadgeColorRole).toString().isEmpty());
}

// --- comandos -------------------------------------------------------------

void TestSearch::testPacmanInstallAvoidsPartialUpgrade()
{
    const Command cmd = m_commands.installPacman({"firefox"});

    QCOMPARE(cmd.program(), QString("pkexec"));
    QCOMPARE(cmd.arguments(), QStringList({"pacman", "-S", "--noconfirm", "firefox"}));
    // -Sy + instalacao pontual e partial upgrade: nao pode voltar.
    QVERIFY(!cmd.arguments().contains("-Sy"));
    QVERIFY(!cmd.arguments().contains("-Syu"));

    const Command many = m_commands.installPacman({"firefox", "vlc"});
    QCOMPARE(many.arguments().last(), QString("vlc"));
    QCOMPARE(many.arguments().size(), 5);

    QVERIFY(m_commands.installPacman({}).isEmpty());
}

void TestSearch::testUninstallCommands()
{
    const Command pacman = m_commands.uninstall("firefox", SearchItem::Pacman);
    QCOMPARE(pacman.program(), QString("pkexec"));
    QCOMPARE(pacman.arguments(), QStringList({"pacman", "-Rns", "--noconfirm", "firefox"}));

    const Command aur = m_commands.uninstall("yay-bin", SearchItem::AUR);
    QCOMPARE(aur.arguments(), QStringList({"pacman", "-Rns", "--noconfirm", "yay-bin"}));

    const Command flatpak = m_commands.uninstall("org.mozilla.firefox", SearchItem::Flatpak);
    QCOMPARE(flatpak.program(), QString("flatpak"));
    QCOMPARE(flatpak.arguments(), QStringList({"uninstall", "-y", "org.mozilla.firefox"}));

    QVERIFY(m_commands.uninstall("", SearchItem::Pacman).isEmpty());
}

void TestSearch::testFlatpakCommands()
{
    const Command install = m_commands.installFlatpak("flathub", {"org.gimp.GIMP"});
    QCOMPARE(install.program(), QString("flatpak"));
    QCOMPARE(install.arguments(),
             QStringList({"install", "-y", "flathub", "org.gimp.GIMP"}));

    const Command update = m_commands.updateFlatpak({"org.gimp.GIMP"});
    QCOMPARE(update.arguments(), QStringList({"update", "-y", "org.gimp.GIMP"}));
}

// --- parsing --------------------------------------------------------------

void TestSearch::testSplitQuery()
{
    QCOMPARE(PackageParser::splitQuery("gimp"), QStringList({"gimp"}));
    QCOMPARE(PackageParser::splitQuery(" gimp , inkscape "),
             QStringList({"gimp", "inkscape"}));
    // Termos vazios nao podem virar argumento do pacman.
    QCOMPARE(PackageParser::splitQuery("gimp, ,inkscape"),
             QStringList({"gimp", "inkscape"}));
    QVERIFY(PackageParser::splitQuery("   ").isEmpty());
    QVERIFY(PackageParser::splitQuery("").isEmpty());
}

void TestSearch::testPacmanParsing()
{
    const QString output = "extra/firefox 100.0-1\n"
                           "    Mozilla Firefox\n"
                           "core-testing/linux 6.0-1\n"
                           "    Linux kernel\n";

    const QList<SearchItem> items = PackageParser::parsePacman(output, m_commands);
    QCOMPARE(items.size(), 2);

    QCOMPARE(items[0].name(), QString("firefox"));
    QCOMPARE(items[0].version(), QString("100.0-1"));
    QCOMPARE(items[0].repository(), QString("extra"));
    QCOMPARE(items[0].description(), QString("Mozilla Firefox"));
    QCOMPARE(items[0].repositoryType(), SearchItem::Pacman);
    QCOMPARE(items[0].displayName(), QString("[Repo: extra] firefox (100.0-1)"));
    QCOMPARE(items[0].installCommand().arguments().last(), QString("firefox"));

    QCOMPARE(items[1].name(), QString("linux"));
    QCOMPARE(items[1].repository(), QString("core-testing"));
}

void TestSearch::testAurParsing()
{
    const QString output = "aur/firefox-nightly 100.0a1-1 (+120 0.55)\n"
                           "    Firefox Nightly\n"
                           "aur/google-chrome 90.0-1\n"
                           "    Google Chrome\n";

    const QList<SearchItem> items = PackageParser::parseAur(output, m_commands);
    QCOMPARE(items.size(), 2);

    QCOMPARE(items[0].name(), QString("firefox-nightly"));
    QCOMPARE(items[0].version(), QString("100.0a1-1"));
    QCOMPARE(items[0].repository(), QString("aur"));
    QCOMPARE(items[0].description(), QString("Firefox Nightly"));
    QCOMPARE(items[0].repositoryType(), SearchItem::AUR);
    QCOMPARE(items[0].installCommand().program(), QString("paru"));
    QCOMPARE(items[1].name(), QString("google-chrome"));
}

void TestSearch::testFlatpakParsing()
{
    const QString output =
        "Name\tDescription\tApplication ID\tVersion\tBranch\tRemotes\n"
        "Firefox\tWeb Browser\torg.mozilla.firefox\t100.0\tstable\tflathub\n"
        "Spotify\tMusic\tcom.spotify.Client\t1.2.0\tstable\tflathub\n";

    const QList<SearchItem> items = PackageParser::parseFlatpak(output, m_commands);
    QCOMPARE(items.size(), 2);

    QCOMPARE(items[0].name(), QString("org.mozilla.firefox"));
    QCOMPARE(items[0].version(), QString("100.0"));
    QCOMPARE(items[0].repository(), QString("flathub"));
    QCOMPARE(items[0].description(), QString("Web Browser"));
    QCOMPARE(items[0].repositoryType(), SearchItem::Flatpak);
    QCOMPARE(items[0].displayName(), QString("[Flatpak] Firefox (100.0)"));
    QCOMPARE(items[1].name(), QString("com.spotify.Client"));
}

void TestSearch::testFlatpakParsingKeepsEmptyDescription()
{
    // Descricao vazia deslocava as colunas e o pacote sumia do resultado.
    const QString output =
        "Name\tDescription\tApplication ID\tVersion\tBranch\tRemotes\n"
        "Sem Descricao\t\torg.exemplo.App\t1.0\tstable\tflathub\n";

    const QList<SearchItem> items = PackageParser::parseFlatpak(output, m_commands);
    QCOMPARE(items.size(), 1);
    QCOMPARE(items[0].name(), QString("org.exemplo.App"));
    QVERIFY(items[0].description().isEmpty());
    QCOMPARE(items[0].repository(), QString("flathub"));
}

void TestSearch::testFlatpakParsingNoMatches()
{
    QVERIFY(PackageParser::parseFlatpak("No matches found\n", m_commands).isEmpty());
}

void TestSearch::testInfoParsing()
{
    const QString output = "Repository      : extra\n"
                           "Name            : firefox\n"
                           "Version         : 100.0-1\n"
                           "Description     : Standalone web browser\n";

    const SearchItem item = PackageParser::parseInfo(output, SearchItem::Pacman, m_commands);
    QCOMPARE(item.name(), QString("firefox"));
    QCOMPARE(item.version(), QString("100.0-1"));
    QCOMPARE(item.repository(), QString("extra"));
    QCOMPARE(item.description(), QString("Standalone web browser"));
    QCOMPARE(item.relevanceScore(), 100);
    QCOMPARE(item.displayName(), QString("[Repo: extra] firefox (100.0-1)"));

    // Saida vazia nao vira um pacote fantasma.
    QVERIFY(PackageParser::parseInfo("", SearchItem::Pacman, m_commands).name().isEmpty());
}

void TestSearch::testPacmanUpdatesParsing()
{
    const QString output = "firefox 100.0-1 -> 101.0-1\n"
                           "linux 6.0-1 -> 6.1-1\n"
                           "ruido sem seta\n"
                           "igual 1.0 -> 1.0\n";

    const QList<SearchItem> items = PackageParser::parsePacmanUpdates(output, m_commands);
    QCOMPARE(items.size(), 2);
    QCOMPARE(items[0].name(), QString("firefox"));
    QVERIFY(items[0].isInstalled());
    QVERIFY(items[0].version().contains("100.0-1"));
    QVERIFY(items[0].version().contains("101.0-1"));
    QCOMPARE(items[1].name(), QString("linux"));
}

void TestSearch::testFlatpakUpdatesParsing()
{
    const QString output =
        "Firefox\torg.mozilla.firefox\tstable\t101.0\tflathub\n"
        "linha\tinvalida\n";

    const QList<SearchItem> items = PackageParser::parseFlatpakUpdates(output, m_commands);
    QCOMPARE(items.size(), 1);
    QCOMPARE(items[0].name(), QString("org.mozilla.firefox"));
    QCOMPARE(items[0].repositoryType(), SearchItem::Flatpak);
    QCOMPARE(items[0].installCommand().arguments().first(), QString("update"));
}

// --- relevancia -----------------------------------------------------------

void TestSearch::testComputeRelevance()
{
    int score = PackageParser::computeRelevance("cura", "Cura 3D printing slicer", "cura");
    QVERIFY2(score >= 100, QString("Exact name match failed: %1").arg(score).toLatin1());

    score = PackageParser::computeRelevance("cura-engine", "Engine for Cura", "cura");
    QVERIFY2(score >= 80, QString("Prefix hyphen match failed: %1").arg(score).toLatin1());

    score = PackageParser::computeRelevance("procura", "Some tool", "cura");
    QVERIFY2(score >= 40 && score < 65,
             QString("Name substring match failed: %1").arg(score).toLatin1());

    score = PackageParser::computeRelevance("somepkg", "A cura for diseases", "cura");
    QVERIFY2(score >= 20 && score < 40,
             QString("Word in description match failed: %1").arg(score).toLatin1());

    score = PackageParser::computeRelevance("somepkg", "High accuracy tool", "cura");
    QVERIFY2(score < 15,
             QString("Substring in description should be low: %1").arg(score).toLatin1());

    const int multiScore =
        PackageParser::computeRelevance("cura-engine", "Cura slicing engine", "cura engine");
    QVERIFY2(multiScore >= 60,
             QString("Multi-word relevance failed: %1").arg(multiScore).toLatin1());
}

void TestSearch::testFilterByRelevance()
{
    auto makeItem = [](const QString &name, const QString &desc) {
        SearchItem item;
        item.setName(name);
        item.setDescription(desc);
        return item;
    };

    QList<SearchItem> items;
    items << makeItem("cura", "Cura 3D printing slicer");
    items << makeItem("cura-engine", "Engine component for Cura");
    items << makeItem("libcerf", "Self-contained numeric library for error functions");
    items << makeItem("whipper", "Python CD-DA ripper preferring accuracy over speed");
    items << makeItem("ares-emu", "Multi-system emulator focusing on accuracy");

    const QList<SearchItem> filtered = PackageParser::filterByRelevance(items, "cura", 15);
    QCOMPARE(filtered.size(), 2);

    QSet<QString> names;
    for (const auto &item : filtered) names.insert(item.name());
    QVERIFY(names.contains("cura"));
    QVERIFY(names.contains("cura-engine"));
    QVERIFY(!names.contains("whipper"));
    QVERIFY(!names.contains("ares-emu"));
    QVERIFY(!names.contains("libcerf"));

    QVERIFY2(filtered[0].relevanceScore() >= filtered[1].relevanceScore(),
             "Results should be sorted by relevance descending");
}

// --- exportacao -----------------------------------------------------------

void TestSearch::testExportCsvEscapesQuotes()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("out.csv");

    SearchItem item("pkg", "1.0", "extra", "pkg (1.0)", Command("pacman", {"-S", "pkg"}));
    item.setDescription("Editor com \"aspas\" no meio");

    QString error;
    QVERIFY2(ExportService::exportResults({item}, path, &error), qPrintable(error));

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString content = QString::fromUtf8(file.readAll());

    QVERIFY(content.contains("Nome,Versão"));
    // Aspas internas duplicadas mantem o campo intacto (RFC 4180).
    QVERIFY(content.contains("\"Editor com \"\"aspas\"\" no meio\""));
    QCOMPARE(content.count('\n'), 2); // cabecalho + 1 linha
}

void TestSearch::testExportJsonRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("out.json");

    SearchItem item("gimp", "2.10", "extra", "[Repo: extra] gimp (2.10)",
                    Command("pkexec", {"pacman", "-S", "--noconfirm", "gimp"}));
    item.setRepositoryType(SearchItem::Pacman);
    item.setIsInstalled(true);

    QVERIFY(ExportService::exportResults({item}, path));

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QJsonArray arr = QJsonDocument::fromJson(file.readAll()).array();
    QCOMPARE(arr.size(), 1);

    const QJsonObject obj = arr.first().toObject();
    QCOMPARE(obj["name"].toString(), QString("gimp"));
    QCOMPARE(obj["repositoryType"].toString(), QString("Pacman"));
    QCOMPARE(obj["isInstalled"].toBool(), true);
    QCOMPARE(obj["installCommand"].toString(),
             QString("pkexec pacman -S --noconfirm gimp"));
}

void TestSearch::testExportTextIsNotTruncated()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("out.txt");

    // Volume suficiente para encher o buffer do QTextStream: era exatamente ai
    // que o arquivo saia cortado quando o close() vinha antes do flush.
    QList<SearchItem> items;
    for (int i = 0; i < 500; ++i) {
        SearchItem item(QString("pacote-%1").arg(i), "1.0", "extra",
                        QString("pacote-%1 (1.0)").arg(i), Command("pacman", {"-S"}));
        item.setDescription(QString("Descrição bem longa do pacote numero %1").arg(i));
        items << item;
    }

    QVERIFY(ExportService::exportResults(items, path));

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString content = QString::fromUtf8(file.readAll());

    QVERIFY(content.contains("Total: 500 pacotes"));
    QVERIFY2(content.contains("pacote-499"), "o final do arquivo foi truncado");
}

QTEST_GUILESS_MAIN(TestSearch)
#include "test_search.moc"
