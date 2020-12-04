#ifndef CRISTALL_H
#define CRISTALL_H

#include <QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QString>

#include <Frame.h>

QT_BEGIN_NAMESPACE
namespace Ui { class Cristall; }
QT_END_NAMESPACE

class Cristall : public QMainWindow
{
    Q_OBJECT

public:
    Cristall(QWidget *parent = nullptr);
    ~Cristall();

private slots:
    void slotChangeAngle();
    void on_goStepX_clicked();
    void on_goStepY_clicked();
    void on_goStepZ_clicked();
    void on_goScaleX_clicked();
    void on_goScaleY_clicked();
    void on_goScaleZ_clicked();
    void on_goX_clicked();
    void on_goY_clicked();
    void on_goZ_clicked();
    void on_goReflectX_clicked();
    void on_goReflectY_clicked();
    void on_goReflectZ_clicked();

    void on_pushButton_clicked();

    void on_comboBox_activated(int index);

private:
    Ui::Cristall *ui;
    Frame *canvas;
};
#endif // CRISTALL_H
