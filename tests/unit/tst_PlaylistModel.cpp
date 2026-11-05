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
    void remoteTitles()
    {
        PlaylistModel m;
        m.append({QUrl("http://h.com/v/My%20Clip.mp4?t=1")});
        const auto title = [&] { return m.data(m.index(0), PlaylistModel::TitleRole).toString(); };
        QCOMPARE(title(), QString("My Clip.mp4")); // last path segment, no file-existence checks
        m.setCurrentTitle("ignored"); // no current entry yet
        QCOMPARE(title(), QString("My Clip.mp4"));
        m.setCurrentIndex(0);
        m.setCurrentTitle("Real Title");
        QCOMPARE(title(), QString("Real Title"));
        QVERIFY(m.data(m.index(0), PlaylistModel::PathRole).toString().startsWith("http://h.com/v/"));
    }
    void m3uRoundTrip()
    {
        PlaylistModel m;
        m.append(urls({"a.mp4", "b.mp4"}));
        const QList<QUrl> back = PlaylistModel::parseM3u(m.toM3u(), QDir("/nowhere"));
        QCOMPARE(back.size(), 2);
        QCOMPARE(back.at(1).toLocalFile(), QStringLiteral("/tmp/b.mp4"));
    }
    void m3uRelativeAndComments()
    {
        const QList<QUrl> u = PlaylistModel::parseM3u(
            QStringLiteral("#EXTM3U\n#EXTINF:1,x\nsub/c.mkv\n\nhttps://h/x.mp4\r\n"), QDir("/media"));
        QCOMPARE(u.size(), 2);
        QCOMPARE(u.at(0).toLocalFile(), QStringLiteral("/media/sub/c.mkv"));
        QCOMPARE(u.at(1).toString(), QStringLiteral("https://h/x.mp4"));
    }
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
    void sequentialNextRepeat()
    {
        PlaylistModel m;
        m.append(urls({"a", "b", "c"}));
        m.setCurrentIndex(1);
        QCOMPARE(m.nextIndex(false, 0, true), 2);
        m.setCurrentIndex(2);
        QCOMPARE(m.nextIndex(false, 0, true), -1);
        QCOMPARE(m.nextIndex(false, 1, true), 0);
        QCOMPARE(m.nextIndex(false, 2, true), 2);  // repeat one: EOF replays
        QCOMPARE(m.nextIndex(false, 2, false), -1); // manual next ignores repeat one
        QCOMPARE(m.previousIndex(false), 1);
        m.setCurrentIndex(0);
        QCOMPARE(m.previousIndex(false), -1);
    }
    void shuffleVisitsAllOnceThenStops()
    {
        PlaylistModel m;
        m.append(urls({"a", "b", "c", "d", "e"}));
        for (int round = 0; round < 20; ++round) {
            PlaylistModel s;
            s.append(urls({"a", "b", "c", "d", "e"}));
            s.setCurrentIndex(round % 5);
            QSet<int> seen{s.currentIndex()};
            for (int k = 0; k < 4; ++k) {
                const int n = s.nextIndex(true, 0, true);
                QVERIFY(n >= 0 && !seen.contains(n));
                seen.insert(n);
                s.setCurrentIndex(n);
            }
            QCOMPARE(s.nextIndex(true, 0, true), -1);
        }
    }
    void shuffleRepeatAllNoImmediateRepeatAndHistory()
    {
        PlaylistModel m;
        m.append(urls({"a", "b", "c"}));
        m.setCurrentIndex(0);
        QList<int> path{0};
        for (int k = 0; k < 12; ++k) {
            const int n = m.nextIndex(true, 1, true);
            QVERIFY(n >= 0 && n != m.currentIndex());
            m.setCurrentIndex(n);
            path << n;
        }
        // previous walks the history backwards
        QCOMPARE(m.previousIndex(true), path.at(path.size() - 2));
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
