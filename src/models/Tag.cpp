#include <random>

#include "Tag.h"

Tag::Tag(int id, const QString& name, QObject* parent)
    : QObject(parent), _id(id), _name("#" + name) {}

Tag::Tag(const QString& name, QObject* parent)
    : Tag(-1, name, parent) {}

int Tag::id() const { return _id; }

const QString &Tag::name() const { return _name; }

void Tag::setName(const QString &newName)
{
    if (_name == newName) return;
    _name = newName;
    emit nameChanged(_name);
}

const QColor &Tag::color() const { return _color; }

void Tag::setColor(const QColor &newColor)
{
    if (_color == newColor) return;
    _color = newColor;
    emit colorChanged(_color);
}

QString Tag::randomHexString(unsigned int length)
{
    QString result;
    static std::mt19937 generator {std::random_device {}()};
    std::string hex_characters = "0123456789abcdef";
    std::uniform_int_distribution<int> dist(0, hex_characters.length() - 1);
    for (unsigned int i = 0; i < length; i++)
        result += hex_characters[dist(generator)];
    return result;
}

QColor Tag::randomColor()
{
    QString result = "#";
    result.append(randomHexString(6));
    return QColor(result);
}