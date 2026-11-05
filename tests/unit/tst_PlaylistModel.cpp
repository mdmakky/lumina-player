#include <QtTest>

#include "core/PlaylistModel.h"

class TestPlaylistModel : public QObject
{
    Q_OBJECT

    static QVariantList urls(std::initializer_list<const char *> names)
    {
        QVariantList l;
        for (auto n : names)
            l << QUrl::fromLocalFile(QString("/tmp/%1").arg(n));
        return l;
    }
    static QString title(const PlaylistModel &m, int r)
    {
        return m.data(m.index(r), PlaylistModel::TitleRole).toString();
    }

private slots:
    void appendAndRoles()
    {
        PlaylistModel m;
        m.append(urls({"a.mp4", "b.mp4"}));
        QCOMPARE(m.rowCount(), 2);
        QCOMPARE(title(m, 1), QStringLiteral("b.mp4"));
        QCOMPARE(m.data(m.index(0), PlaylistModel::PathRole).toString(), QStringLiteral("/tmp/a.mp4"));
        QCOMPARE(m.currentIndex(), -1);
    }
    void currentTracking()
    {
        PlaylistModel m;
        m.append(urls({"a", "b", "c", "d"}));
        m.setCurrentIndex(2);
        QVERIFY(m.data(m.index(2), PlaylistModel::CurrentRole).toBool());
        m.remove(0); // before current -> shifts down
        QCOMPARE(m.currentIndex(), 1);
        QCOMPARE(title(m, 1), QStringLiteral("c"));
        m.move(2, 0); // d jumps before current -> current shifts up
        QCOMPARE(m.currentIndex(), 2);
        QCOMPARE(title(m, 2), QStringLiteral("c"));
        m.move(2, 0); // move current itself
        QCOMPARE(m.currentIndex(), 0);
        QCOMPARE(title(m, 0), QStringLiteral("c"));
        m.move(0, 2); // and back past others
        QCOMPARE(m.currentIndex(), 2);
        m.remove(2); // remove current
        QCOMPARE(m.currentIndex(), -1);
    }
    void clearResets()
    {
        PlaylistModel m;
        m.append(urls({"a", "b"}));
        m.setCurrentIndex(1);
        m.clear();
        QCOMPARE(m.rowCount(), 0);
        QCOMPARE(m.currentIndex(), -1);
    }
    void rejectsBadIndices()
    {
        PlaylistModel m;
        m.append(urls({"a"}));
        m.setCurrentIndex(5);
        m.remove(3);
        m.move(0, 4);
        QCOMPARE(m.currentIndex(), -1);
        QCOMPARE(m.rowCount(), 1);
    }
};

QTEST_MAIN(TestPlaylistModel)
#include "tst_PlaylistModel.moc"
