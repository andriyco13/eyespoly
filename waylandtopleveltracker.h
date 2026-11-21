#ifndef WAYLANDTOPLEVELTRACKER_H
#define WAYLANDTOPLEVELTRACKER_H

#include <QObject>
#include <QString>

class WaylandToplevelTracker : public QObject
{
    Q_OBJECT

public:
    explicit WaylandToplevelTracker(QObject *parent = nullptr);
    ~WaylandToplevelTracker() override;

    bool isAvailable() const;
    QString activeWindowInfo() const;

private:
    class Private;
    Private *d = nullptr;
};

#endif // WAYLANDTOPLEVELTRACKER_H
