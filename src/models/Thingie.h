#ifndef THINGIE_H
#define THINGIE_H

#include <QObject>
#include <QColor>

class Thingie : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int id READ id CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
    explicit Thingie(int id, const QString& name, QObject* parent = nullptr);
    explicit Thingie(const QString& name, QObject* parent = nullptr);

    int id() const;
    const QString &name() const;
    const QColor &color() const;

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

    QColor randomColor();
    QString randomHexString(unsigned int length);
};

#endif