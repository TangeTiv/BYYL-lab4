#ifndef DOTRENDERER_H
#define DOTRENDERER_H

#include <QObject>
#include <QProcess>
#include <QByteArray>
#include <QString>
#include <string>

// 封装 Graphviz dot 命令，将 DOT 字符串异步渲染为 SVG
class DotRenderer : public QObject {
    Q_OBJECT

public:
    explicit DotRenderer(QObject *parent = nullptr);
    ~DotRenderer() override;

    // 异步渲染 DOT→SVG，tag 用于标识调用者（如 0=LR0, 1=LR1, 2=LALR1）
    void render(const std::string &dotString, int tag = 0);

    // 同步渲染（阻塞调用线程，备用）
    static QByteArray renderSync(const std::string &dotString);

    // 系统是否安装了可用的 Graphviz
    bool isGraphvizAvailable() const;

signals:
    void svgReady(const QByteArray &svgData, int tag);
    void renderError(const QString &errorMessage, int tag);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    bool checkGraphviz();

    QProcess *process_;
    QString dotPath_;       // dot.exe 的完整路径
    bool dotAvailable_;
    int currentTag_;
    std::string lastDotString_;
    QByteArray svgData_;
    bool running_;
};

#endif // DOTRENDERER_H
