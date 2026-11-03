#ifndef Tag_H
#define Tag_H

#include <QObject>
#include <QColor>

class Tag : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int id READ id CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
    explicit Tag(int id, const QString& name, QObject* parent = nullptr);
    explicit Tag(const QString& name, QObject* parent = nullptr);

    int id() const;
    const QString &name() const;
    const QColor &color() const;

    static QColor randomColor();

public slots:
    void setName(const QString &newName);
    void setColor(const QColor &newColor);

signals:
    void nameChanged(const QString &name);
    void colorChanged(const QColor &color);

private:
    int _id = -1;
    QString _name;
    QColor _color = randomColor();

    static QString randomHexString(unsigned int length);
};

#endif