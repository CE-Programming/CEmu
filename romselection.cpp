#include "romselection.h"
#include "ui_romselection.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include "core/flash.h"

std::string romImagePath;

RomSelection::RomSelection(QWidget *p) : QDialog(p), ui(new Ui::RomSelection)
{
    ui->setupUi(this);
    setWindowModality(Qt::ApplicationModal);
    setWindowFlags(Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint );

    connect(ui->rompath, SIGNAL(textChanged(QString)), this, SLOT(checkInput(QString)));
}

bool fileExists(const QString &path) {
    QFileInfo checkFile(path);
    return (checkFile.exists() && checkFile.isFile());
}

RomSelection::~RomSelection()
{
    delete ui;
}

void RomSelection::checkInput(const QString &path)
{
    // see if the file exists -- if so, change the continue ability
    ui->next->setEnabled(fileExists(path));
}

void RomSelection::on_create_sel_clicked()
{
    this->ui->rompath->setEnabled(false);
    this->ui->browse->setEnabled(false);
    this->ui->next->setEnabled(true);
    this->ui->next->setText(QString("Continue"));
}

void RomSelection::on_cancel_clicked()
{
    romImagePath.clear();
    close();
}

void RomSelection::on_browse_sel_clicked()
{
    ui->rompath->setEnabled(true);
    ui->browse->setEnabled(true);
    ui->next->setText(QString("Finish"));
    checkInput(ui->rompath->text());     // make sure the text is still valid
}

void RomSelection::on_browse_clicked()
{
  ui->rompath->setText(
      QFileDialog::getOpenFileName(this, tr("Open ROM file"),"",
      tr("Known Types (*.rom *.sav);;ROM Image (*.rom);;Saved Image (*.sav);;All Files (*.*)")));
}

void RomSelection::on_next_clicked()
{
    if(ui->browse_sel->isChecked()) {
        if(flash_open(ui->rompath->text().toLatin1()) == 0) {
            QMessageBox::critical(this, trUtf8("Invalid ROM image"), trUtf8("You have selected an invalid ROM image."));
            return;
        }
        romImagePath = ui->rompath->text().toLatin1().toStdString();
        close();
    }
}
