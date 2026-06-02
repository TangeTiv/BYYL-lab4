#include "DotRenderer.h"
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <iostream>

DotRenderer::DotRenderer(QObject *parent)
    : QObject(parent)
    , process_(nullptr)
    , dotAvailable_(false)
    , currentTag_(0)
    , running_(false)
{
    dotAvailable_ = checkGraphviz();
}

DotRenderer::~DotRenderer() {
    if (process_) {
        process_->kill();
        process_->waitForFinished(3000);
    }
}

// ── 检测 Graphviz 是否可用 ──
bool DotRenderer::checkGraphviz() {
    // 1) 优先：程序目录下的 graphviz/dot.exe
    QString bundledPath = QCoreApplication::applicationDirPath() + "/graphviz/dot.exe";
    if (QFileInfo::exists(bundledPath)) {
        dotPath_ = bundledPath;
        std::cout << "[DotRenderer] Using bundled dot: " << dotPath_.toStdString() << std::endl;
        return true;
    }

    // 2) 在 PATH 中查找
    QString pathDot = QStandardPaths::findExecutable("dot");
    if (!pathDot.isEmpty()) {
        dotPath_ = pathDot;
        std::cout << "[DotRenderer] Found dot in PATH: " << dotPath_.toStdString() << std::endl;
        return true;
    }

    // 3) Windows 常见安装路径
#ifdef Q_OS_WIN
    QStringList candidates = {
        "C:/Program Files/Graphviz/bin/dot.exe",
        "C:/Program Files (x86)/Graphviz/bin/dot.exe",
        QDir::homePath() + "/scoop/apps/graphviz/current/bin/dot.exe"
    };
    for (const auto& p : candidates) {
        if (QFileInfo::exists(p)) {
            dotPath_ = p;
            std::cout << "[DotRenderer] Found dot at: " << dotPath_.toStdString() << std::endl;
            return true;
        }
    }
#endif

    std::cout << "[DotRenderer] Graphviz 'dot' not found." << std::endl;
    return false;
}

bool DotRenderer::isGraphvizAvailable() const {
    return dotAvailable_;
}

// ── 异步渲染 ──
void DotRenderer::render(const std::string &dotString, int tag) {
    if (!dotAvailable_) {
        emit renderError("Graphviz (dot) 未安装。请安装 Graphviz 并确保 dot 在 PATH 中。", tag);
        return;
    }

    // 内容未变则跳过
    if (dotString == lastDotString_ && !svgData_.isEmpty()) {
        emit svgReady(svgData_, tag);
        return;
    }

    // 终止正在进行的渲染
    if (process_ && running_) {
        process_->kill();
        process_->waitForFinished(2000);
    }

    if (!process_) {
        process_ = new QProcess(this);
        connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &DotRenderer::onProcessFinished);
    }

    lastDotString_ = dotString;
    currentTag_ = tag;
    running_ = true;

    QStringList args;
    args << "-Tsvg";

    process_->start(dotPath_, args);
    if (!process_->waitForStarted(5000)) {
        running_ = false;
        emit renderError("无法启动 dot 进程。请确认 Graphviz 已正确安装。", tag);
        return;
    }

    // 将 DOT 内容写入 stdin
    QByteArray dotBytes(dotString.c_str(), (int)dotString.size());
    process_->write(dotBytes);
    process_->closeWriteChannel();
}

// ── dot 进程结束时回调 ──
void DotRenderer::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    running_ = false;

    if (exitStatus == QProcess::CrashExit || exitCode != 0) {
        QString errMsg = QString::fromUtf8(process_->readAllStandardError());
        if (errMsg.isEmpty())
            errMsg = QString("dot 进程异常退出 (exit code %1)").arg(exitCode);
        emit renderError(errMsg, currentTag_);
        return;
    }

    svgData_ = process_->readAllStandardOutput();
    emit svgReady(svgData_, currentTag_);
}

// ── 同步渲染（备用） ──
QByteArray DotRenderer::renderSync(const std::string &dotString) {
    QProcess proc;
    QStringList args;
    args << "-Tsvg";

    // 同步版也优先用 bundled
    QString dotExe = "dot";
    {
        QString bundled = QCoreApplication::applicationDirPath() + "/graphviz/dot.exe";
        if (QFileInfo::exists(bundled))
            dotExe = bundled;
    }
    proc.start(dotExe, args);
    if (!proc.waitForStarted(5000))
        return QByteArray();

    QByteArray dotBytes(dotString.c_str(), (int)dotString.size());
    proc.write(dotBytes);
    proc.closeWriteChannel();

    if (!proc.waitForFinished(30000))
        return QByteArray();

    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
        return QByteArray();

    return proc.readAllStandardOutput();
}
