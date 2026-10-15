#include <QObject>
#include <QTranslator>
#include <QQmlApplicationEngine>
#include <QGuiApplication>

class Translator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
public:
    explicit Translator(QQmlApplicationEngine *engine, QObject *parent = nullptr)
        : QObject(parent), m_engine(engine) {}

    QString language() const { return m_language; }

    void setLanguage(const QString &lang)
    {
        if (lang == m_language) return;

        qApp->removeTranslator(&m_translator);

        if (lang == "en") {
            if (m_translator.load(":/i18n/en.qm")) qApp->installTranslator(&m_translator);
        }

        m_language = lang;
        m_engine->retranslate();
        emit languageChanged();
    }

signals:
    void languageChanged();

private:
    QQmlApplicationEngine *m_engine;
    QTranslator m_translator;
    QString m_language = "ru";
};